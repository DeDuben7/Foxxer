/**
 ******************************************************************************
 * @file      sa818.c
 * @brief     short description
 * @version   version
 * @author    R. van Renswoude
 * @date      2025
 ******************************************************************************
 * @details detailed description
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2025 Ruben van Renswoude.
 * All rights reserved.</center></h2>
 *
 ******************************************************************************
 */
/**
 * @addtogroup  sa818
 * @{
 */


/* Includes -------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "sa818.h"
#include "sa818_uart.h"
#include "gpio.h"

/* Defines -------------------------------------------------------------------*/

#define CRLF "\r\n"

/* Typedefs -------------------------------------------------------------------*/

/* Variables -------------------------------------------------------------------*/

sa818_group_t sa818_config;

/* Function prototypes ---------------------------------------------------------*/

/* Functions -------------------------------------------------------------------*/

static sa818_status_t sa818_send_command(const char *cmd, const char *expect)
{
    char rxbuf[64];
    sa818_uart_transmit(cmd);
    sa818_uart_receive(rxbuf);

    if (expect && strstr(rxbuf, expect))
        return SA818_OK;
    return SA818_ERROR;
}

sa818_status_t sa818_init()
{
  sa818_uart_init();

  sa818_config.bandwidth = 0;
  sa818_config.tx_freq = 144.4500;
  sa818_config.rx_freq = 144.4500;
  memcpy(sa818_config.tx_subaudio, "0000", sizeof(sa818_config.tx_subaudio));
  memcpy(sa818_config.rx_subaudio, "0000", sizeof(sa818_config.rx_subaudio));
  sa818_config.squelch = 4;

  sa818_set_ptt_level(HIGH);    // high = rx, low = tx
  sa818_set_low_power();
  sa818_set_power_on_off(HIGH); // high = on, low = off

  return sa818_handshake();
}

sa818_status_t sa818_handshake()
{
    return sa818_send_command("AT+DMOCONNECT\r\n", "+DMOCONNECT:0");
}

sa818_status_t sa818_set_group()
{
    char cmd[80];
    snprintf(cmd, sizeof(cmd),
             "AT+DMOSETGROUP=%d,%.4f,%.4f,%s,%d,%s\r\n",
             sa818_config.bandwidth,
             sa818_config.tx_freq,
             sa818_config.rx_freq,
             sa818_config.tx_subaudio,
             sa818_config.squelch,
             sa818_config.rx_subaudio);
    return sa818_send_command(cmd, "+DMOSETGROUP:0");
}

sa818_status_t sa818_set_volume(uint8_t vol)
{
    char cmd[32];
    snprintf(cmd, sizeof(cmd), "AT+DMOSETVOLUME=%d\r\n", vol);
    return sa818_send_command(cmd, "+DMOSETVOLUME:0");
}

sa818_status_t sa818_set_filter(uint8_t preemph, uint8_t highpass, uint8_t lowpass)
{
    char cmd[32];
    snprintf(cmd, sizeof(cmd), "AT+SETFILTER=%d,%d,%d\r\n", preemph, highpass, lowpass);
    return sa818_send_command(cmd, "+DMOSETFILTER:0");
}

sa818_status_t SA818_SetTail(uint8_t tail_on)
{
    char cmd[32];
    snprintf(cmd, sizeof(cmd), "AT+SETTAIL=%d\r\n", tail_on);
    return sa818_send_command(cmd, "+DMOSETTAIL:0");
}

sa818_status_t SA818_ReadRSSI(uint8_t *rssi)
{
    char rxbuf[64];
    const char *cmd = "RSSI?\r\n";
    sa818_uart_transmit(cmd);
    sa818_uart_receive(rxbuf);

    char *p = strstr(rxbuf, "RSSI=");
    if (p) {
        *rssi = (uint8_t)atoi(p + 5);
        return SA818_OK;
    }
    return SA818_ERROR;
}

sa818_status_t sa818_read_version(char *version, uint16_t maxlen)
{
    char rxbuf[64];
    const char *cmd = "AT+VERSION\r\n";
    sa818_uart_transmit(cmd);
    sa818_uart_receive(rxbuf);

    char *p = strstr(rxbuf, "+VERSION:");
    if (p) {
        strncpy(version, p + 9, maxlen);
        return SA818_OK;
    }
    return SA818_ERROR;
}

void sa818_set_power_on_off(pin_level_t level) {
  HAL_GPIO_WritePin(SA818_PD_GPIO_Port, SA818_PD_Pin, level ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

void sa818_set_ptt_level(pin_level_t level) {
  HAL_GPIO_WritePin(SA818_PTT_GPIO_Port, SA818_PTT_Pin, level ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

// --- Set SA818 to low power mode (drive pin LOW)
void sa818_set_low_power(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Configure as output, pull-down (optional)
    GPIO_InitStruct.Pin = SA818_HL_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(SA818_HL_GPIO_Port, &GPIO_InitStruct);

    HAL_GPIO_WritePin(SA818_HL_GPIO_Port, SA818_HL_Pin, GPIO_PIN_RESET);
}

// --- Set SA818 to high power mode (float the pin)
void sa818_set_high_power(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Configure as input, no pull — this "disconnects" the MCU
    GPIO_InitStruct.Pin = SA818_HL_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(SA818_HL_GPIO_Port, &GPIO_InitStruct);
}
