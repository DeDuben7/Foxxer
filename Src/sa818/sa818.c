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

#include "stm32h7xx_hal.h"
#include "sa818.h"
#include "sa818_uart.h"
#include "gpio.h"

/* Defines -------------------------------------------------------------------*/

#define CRLF "\r\n"

/* Typedefs -------------------------------------------------------------------*/

/* Variables -------------------------------------------------------------------*/

sa818_settings_t sa818_settings;

/* Function prototypes ---------------------------------------------------------*/

static sa818_status_t sa818_send_cmd(const char *cmd, const char *expect, char *resp, int resp_len, int timeout_ms);

/* Functions -------------------------------------------------------------------*/

sa818_status_t sa818_init(void)
{
  // Initialize UART interface (9600 8N1)
  sa818_uart_init();

  // --------------------------------------------------------------------------
  // GPIO and Power control
  // --------------------------------------------------------------------------
  sa818_set_ptt_level(HIGH);     // HIGH = RX mode
  sa818_set_low_power();         // low power mode (since fox-hunt RX only)
  sa818_set_power_on_off(HIGH);  // HIGH = power ON
  HAL_Delay(300);   // allow SA818 to boot fully
  sa818_uart_flush();
  // --------------------------------------------------------------------------
  // Initialize configuration structure with sensible defaults for RX-only use
  // --------------------------------------------------------------------------
  memset(&sa818_settings, 0, sizeof(sa818_settings_t));

  sa818_settings.bandwidth     = 0;          // 12.5 kHz narrowband for ARDF channels
  sa818_settings.tx_frequency  = 144.4500f;  // TX = RX for simple alignment (not transmitting)
  sa818_settings.rx_frequency  = 144.4500f;  // Example fox beacon frequency (2m band)
  strncpy(sa818_settings.tx_subaudio, "0000", sizeof(sa818_settings.tx_subaudio));
  strncpy(sa818_settings.rx_subaudio, "0000", sizeof(sa818_settings.rx_subaudio));
  sa818_settings.squelch       = 4;          // Medium squelch, good balance for weak signals
  sa818_settings.volume        = 5;          // Mid-range volume level
  sa818_settings.pre_de_emph   = 0;          // Normal emphasis
  sa818_settings.highpass      = 0;          // Normal audio response
  sa818_settings.lowpass       = 0;          // Normal audio response
  sa818_settings.tail_tone     = 0;          // Tail tone off (clean RX audio)

  // --------------------------------------------------------------------------
  // Basic module initialization sequence
  // --------------------------------------------------------------------------
  if (sa818_handshake() != SA818_OK) {
    return SA818_ERROR;
  }

  if (sa818_set_group(&sa818_settings) != SA818_OK) {
    return SA818_ERROR;
  }

  if (sa818_set_volume(sa818_settings.volume) != SA818_OK) {
    return SA818_ERROR;
  }

  if (sa818_set_filter(sa818_settings.pre_de_emph,
                       sa818_settings.highpass,
                       sa818_settings.lowpass) != SA818_OK) {
    return SA818_ERROR;
  }

  if (sa818_set_tail(sa818_settings.tail_tone) != SA818_OK) {
    return SA818_ERROR;
  }

  return SA818_OK;
}


// AT+DMOCONNECT
sa818_status_t sa818_handshake(void)
{
    return sa818_send_cmd("AT+DMOCONNECT\r\n", "+DMOCONNECT:0", NULL, 0, 10000);
}

// AT+DMOSETGROUP=BW,TX_F,RX_F,TxSub,SQ,RxSub
sa818_status_t sa818_set_group(const sa818_settings_t *cfg)
{
  char cmd[96];
  snprintf(cmd, sizeof(cmd),
           "AT+DMOSETGROUP=%d,%.4f,%.4f,%s,%d,%s\r\n",
           cfg->bandwidth,
           cfg->tx_frequency,
           cfg->rx_frequency,
           cfg->tx_subaudio,
           cfg->squelch,
           cfg->rx_subaudio);

  return sa818_send_cmd(cmd, "+DMOSETGROUP:0", NULL, 0, 300);
}

// AT+DMOSETVOLUME=X
sa818_status_t sa818_set_volume(uint8_t level)
{
  char cmd[32];
  snprintf(cmd, sizeof(cmd), "AT+DMOSETVOLUME=%d\r\n", level);
  return sa818_send_cmd(cmd, "+DMOSETVOLUME:0", NULL, 0, 300);
}

// AT+SETFILTER=PRE/DE-EMPH,HIGHPASS,LOWPASS
sa818_status_t sa818_set_filter(uint8_t pre_de_emph, uint8_t highpass, uint8_t lowpass)
{
  char cmd[48];
  snprintf(cmd, sizeof(cmd), "AT+SETFILTER=%d,%d,%d\r\n", pre_de_emph, highpass, lowpass);
  return sa818_send_cmd(cmd, "+DMOSETFILTER:0", NULL, 0, 300);
}

// AT+SETTAIL=TAIL
sa818_status_t sa818_set_tail(uint8_t tail_on)
{
  char cmd[32];
  snprintf(cmd, sizeof(cmd), "AT+SETTAIL=%d\r\n", tail_on);
  return sa818_send_cmd(cmd, "+DMOSETTAIL:0", NULL, 0, 300);
}

// S+Rx_F  --> S=0 or S=1
sa818_status_t sa818_scan_frequency(float rx_freq, bool *signal_found)
{
  char cmd[32], resp[32];
  snprintf(cmd, sizeof(cmd), "S+%.4f\r\n", rx_freq);

  if (sa818_send_cmd(cmd, "S=", resp, sizeof(resp), 300) != SA818_OK)
    return SA818_ERROR;

  char *eq = strchr(resp, '=');
  if (!eq) return SA818_ERROR;
  *signal_found = (*(eq + 1) == '0');
  return SA818_OK;
}

// RSSI? --> RSSI=X
sa818_status_t sa818_get_rssi(uint8_t *rssi)
{
  char resp[32];
  if (sa818_send_cmd("RSSI?\r\n", "RSSI=", resp, sizeof(resp), 300) != SA818_OK)
    return SA818_ERROR;

  char *eq = strchr(resp, '=');
  if (!eq) return SA818_ERROR;
  *rssi = (uint8_t)atoi(eq + 1);
  return SA818_OK;
}

// AT+VERSION --> +VERSION:SA818_Vx.x
sa818_status_t sa818_get_version(char *version_str, int max_len)
{
  char resp[64];
  if (sa818_send_cmd("AT+VERSION\r\n", "+VERSION:", resp, sizeof(resp), 300) != SA818_OK) {
    return SA818_ERROR;
  }

  char *ver = strstr(resp, "+VERSION:");

  if (!ver) {
    return SA818_ERROR;
  }

  strncpy(version_str, ver + 9, max_len - 1);
  version_str[max_len - 1] = '\0';

  return SA818_OK;
}

void sa818_set_power_on_off(pin_level_t level) {
  HAL_GPIO_WritePin(SA818_PD_GPIO_Port, SA818_PD_Pin, level ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void sa818_set_ptt_level(pin_level_t level) {
  HAL_GPIO_WritePin(SA818_PTT_GPIO_Port, SA818_PTT_Pin, level ? GPIO_PIN_SET : GPIO_PIN_RESET);
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

static sa818_status_t sa818_send_cmd(const char *cmd, const char *expect, char *resp, int resp_len, int timeout_ms)
{
  char buffer[64] = {0};
  sa818_uart_transmit(cmd);
  int len = sa818_uart_receive(buffer, sizeof(buffer), timeout_ms);

  if (len <= 0) {
    return SA818_ERROR;
  }

  if (resp && resp_len > 0) {
    strncpy(resp, buffer, resp_len - 1);
  }

  if (expect && strstr(buffer, expect)) {
    return SA818_OK;
  }

  return SA818_ERROR;
}
