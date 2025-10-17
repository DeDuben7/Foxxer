/**
 ******************************************************************************
 * @file      spi.c
 * @brief     SPI4 driver with DMA queue and GPIO control for LCD display
 * @version   2.0
 * @author    R. van Renswoude
 * @date      2025
 ******************************************************************************
 */

#include <string.h>

#include "stm32h7xx_hal.h"
#include "spi.h"
#include "led.h"
#include "gpio.h"

/* -------------------------------------------------------------------------- */
/* Private Configuration                                                      */
/* -------------------------------------------------------------------------- */

#define SPI_DMA_QUEUE_SIZE   800

typedef struct {
    uint8_t data[SPI_DMA_MAX_PAYLOAD];
    uint16_t length;
    bool rs_data;
    bool release_cs;
} spi_dma_cmd_t;

/* -------------------------------------------------------------------------- */
/* Private Variables                                                          */
/* -------------------------------------------------------------------------- */

SPI_HandleTypeDef display_spi_handle;
DMA_HandleTypeDef hdma_spi4_tx;

static spi_dma_cmd_t spi_queue[SPI_DMA_QUEUE_SIZE];
static volatile uint8_t spi_q_head = 0;
static volatile uint8_t spi_q_tail = 0;
static volatile bool spi_dma_active = false;

/* -------------------------------------------------------------------------- */
/* Internal Functions                                                         */
/* -------------------------------------------------------------------------- */

static void start_next_spi_dma(void);

/* -------------------------------------------------------------------------- */
/* Public API                                                                 */
/* -------------------------------------------------------------------------- */

void display_spi_init(void)
{
    /* SPI4 configuration */
    display_spi_handle.Instance = SPI4;
    display_spi_handle.Init.Mode = SPI_MODE_MASTER;
    display_spi_handle.Init.Direction = SPI_DIRECTION_1LINE;
    display_spi_handle.Init.DataSize = SPI_DATASIZE_8BIT;
    display_spi_handle.Init.CLKPolarity = SPI_POLARITY_LOW;
    display_spi_handle.Init.CLKPhase = SPI_PHASE_1EDGE;
    display_spi_handle.Init.NSS = SPI_NSS_SOFT;
    display_spi_handle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
    display_spi_handle.Init.FirstBit = SPI_FIRSTBIT_MSB;
    display_spi_handle.Init.TIMode = SPI_TIMODE_DISABLE;
    display_spi_handle.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    display_spi_handle.Init.CRCPolynomial = 0;
    display_spi_handle.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
    display_spi_handle.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_ENABLE;
    display_spi_handle.Init.IOSwap = SPI_IO_SWAP_DISABLE;

    if (HAL_SPI_Init(&display_spi_handle) != HAL_OK)
    {
        // Error_Handler();
    }
}

/* Blocking SPI transmit */
uint32_t display_spi_transmit(const uint8_t *data, uint16_t size, uint32_t timeout)
{
    return HAL_SPI_Transmit(&display_spi_handle, (uint8_t *)data, size, timeout);
}

/* Blocking SPI receive */
uint32_t display_spi_receive(uint8_t *data, uint16_t size, uint32_t timeout)
{
    return HAL_SPI_Receive(&display_spi_handle, data, size, timeout);
}

void display_spi_rs_set(void) {
  HAL_GPIO_WritePin(LCD_WR_RS_GPIO_Port, LCD_WR_RS_Pin, GPIO_PIN_SET);
}

void display_spi_rs_reset(void) {
  HAL_GPIO_WritePin(LCD_WR_RS_GPIO_Port, LCD_WR_RS_Pin, GPIO_PIN_RESET);
}

void display_spi_cs_set(void) {
  HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
}

void display_spi_cs_reset(void) {
  HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);
}

/* -------------------------------------------------------------------------- */
/* DMA Queue Interface                                                        */
/* -------------------------------------------------------------------------- */

void display_spi_enqueue(const uint8_t *data, uint16_t size, bool rs_data, bool release_cs)
{
    if (data == NULL || size == 0 || size > SPI_DMA_MAX_PAYLOAD)
        return;

    // --- Try to combine with previous entry if possible ---
    if (spi_q_head != spi_q_tail)  // queue not empty
    {
        uint8_t last_index = (spi_q_head + SPI_DMA_QUEUE_SIZE - 1) % SPI_DMA_QUEUE_SIZE;
        spi_dma_cmd_t *last = &spi_queue[last_index];

        // Check if same mode and enough room to append
        if (last->rs_data == rs_data &&
            last->release_cs == release_cs &&
            (last->length + size) <= SPI_DMA_MAX_PAYLOAD)
        {
            memcpy(&last->data[last->length], data, size);
            last->length += size;
            return; // successfully merged, done
        }
    }

    // --- Normal path: push new queue entry ---
    uint8_t next = (spi_q_head + 1) % SPI_DMA_QUEUE_SIZE;

    // Handle full queue: flush one entry (blocking)
    if (next == spi_q_tail)
        display_spi_flush_blocking();

    memcpy(spi_queue[spi_q_head].data, data, size);
    spi_queue[spi_q_head].length     = size;
    spi_queue[spi_q_head].rs_data    = rs_data;
    spi_queue[spi_q_head].release_cs = release_cs;
    spi_q_head = next;

    // Start next DMA transfer if idle
    if (!spi_dma_active)
        start_next_spi_dma();
}


/* Wait until SPI is idle and flush one pending transfer */
void display_spi_flush_blocking(void)
{
    while (spi_dma_active) { }

    if (spi_q_head == spi_q_tail)
        return;

    spi_dma_cmd_t *cmd = &spi_queue[spi_q_tail];

    /* GPIO setup */
    if (cmd->rs_data)
      display_spi_rs_set();
    else
      display_spi_rs_reset();

    display_spi_cs_reset();

    HAL_StatusTypeDef status = HAL_SPI_Transmit(&display_spi_handle, cmd->data, cmd->length, HAL_MAX_DELAY);

    if (cmd->release_cs)
       display_spi_cs_set();

    if (status != HAL_OK)
        led_set_interval(LED_ERROR_INTERVAL);

    spi_q_tail = (spi_q_tail + 1) % SPI_DMA_QUEUE_SIZE;
}

/* Check if SPI TX is busy */
bool display_spi_is_tx_busy(void)
{
    return spi_dma_active;
}

/* -------------------------------------------------------------------------- */
/* Internal DMA Scheduling                                                    */
/* -------------------------------------------------------------------------- */

static void start_next_spi_dma(void)
{
    if (spi_dma_active || spi_q_head == spi_q_tail)
        return;

    spi_dma_cmd_t *cmd = &spi_queue[spi_q_tail];

    if (cmd->rs_data)
      display_spi_rs_set();
    else
      display_spi_rs_reset();

    display_spi_cs_reset();

    spi_dma_active = true;

    if (HAL_SPI_Transmit_DMA(&display_spi_handle, cmd->data, cmd->length) != HAL_OK)
    {
        spi_dma_active = false;
        display_spi_cs_set();
        led_set_interval(LED_ERROR_INTERVAL);
    }
}

/* -------------------------------------------------------------------------- */
/* HAL Callbacks                                                              */
/* -------------------------------------------------------------------------- */

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI4)
    {
        spi_dma_cmd_t *cmd = &spi_queue[spi_q_tail];

        if (cmd->release_cs)
          display_spi_cs_set();

        spi_q_tail = (spi_q_tail + 1) % SPI_DMA_QUEUE_SIZE;
        spi_dma_active = false;

        /* Start next queued command */
        start_next_spi_dma();
    }
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
  if (hspi->Instance == SPI4)
  {
    spi_dma_active = false;
    display_spi_cs_set();
    led_set_interval(LED_ERROR_INTERVAL);
  }
}

/* -------------------------------------------------------------------------- */
/* MSP Initialization / DeInitialization                                      */
/* -------------------------------------------------------------------------- */

void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

    if (hspi->Instance == SPI4)
    {
        /* SPI4 Clock Source */
        PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SPI4;
        PeriphClkInitStruct.Spi45ClockSelection = RCC_SPI45CLKSOURCE_D2PCLK1;
        HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);

        /* Peripheral clock enable */
        __HAL_RCC_SPI4_CLK_ENABLE();
        __HAL_RCC_GPIOE_CLK_ENABLE();

        /* SPI4 GPIO Configuration: PE12 -> SCK, PE14 -> MOSI */
        GPIO_InitStruct.Pin = GPIO_PIN_12 | GPIO_PIN_14;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF5_SPI4;
        HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

        /* SPI4 TX DMA Init */
        hdma_spi4_tx.Instance = DMA1_Stream4;
        hdma_spi4_tx.Init.Request = DMA_REQUEST_SPI4_TX;
        hdma_spi4_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
        hdma_spi4_tx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_spi4_tx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_spi4_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_spi4_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_spi4_tx.Init.Mode = DMA_NORMAL;
        hdma_spi4_tx.Init.Priority = DMA_PRIORITY_LOW;
        hdma_spi4_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;

        if (HAL_DMA_Init(&hdma_spi4_tx) != HAL_OK)
        {
            // Error_Handler();
        }

        __HAL_LINKDMA(hspi, hdmatx, hdma_spi4_tx);

        HAL_NVIC_SetPriority(SPI4_IRQn, 1, 0);
        HAL_NVIC_EnableIRQ(SPI4_IRQn);
    }
}

void HAL_SPI_MspDeInit(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI4)
    {
        __HAL_RCC_SPI4_CLK_DISABLE();
        HAL_GPIO_DeInit(GPIOE, GPIO_PIN_12 | GPIO_PIN_14);
        HAL_DMA_DeInit(&hdma_spi4_tx);
        HAL_NVIC_DisableIRQ(SPI4_IRQn);
    }
}
