#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "stm32h7xx_hal.h"
#include "sa818.h"
#include "sa818_uart.h"
#include "gpio.h"
#include "test_tone.h"

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------
#define CRLF "\r\n"
#define SA818_RSSI_POLL_INTERVAL_MS   1000
#define SA818_CMD_TIMEOUT_MS          300
// #define DEBUG_SA818  // Uncomment for debug prints

// ---------------------------------------------------------------------------
// Internal command state
// ---------------------------------------------------------------------------
typedef enum {
    SA818_CMD_IDLE = 0,
    SA818_CMD_TX,
    SA818_CMD_RX_WAIT,
    SA818_CMD_PROCESS
} sa818_cmd_state_t;

typedef enum {
    SA818_SCHED_OK = 0,
    SA818_SCHED_BUSY,
    SA818_SCHED_ERROR
} sa818_sched_status_t;

typedef struct {
    char cmd[96];
    char expect[32];
    uint32_t timeout_ms;
    bool active;
} sa818_cmd_request_t;

static sa818_cmd_state_t sa818_state = SA818_CMD_IDLE;
static sa818_cmd_request_t sa818_pending;  // single queued command

static uint8_t sa818_rxbuf[128];
static uint32_t sa818_deadline = 0;
static uint32_t last_rssi_poll = 0;

// ---------------------------------------------------------------------------
// Settings
// ---------------------------------------------------------------------------
sa818_settings_t sa818_settings;

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

// Non-blocking uart helpers
static sa818_status_t sa818_handshake_dma(void);
static sa818_status_t sa818_set_group_dma(const sa818_settings_t *cfg);
static sa818_status_t sa818_set_volume_dma(uint8_t level);
static sa818_status_t sa818_set_filter_dma(uint8_t pre_de_emph, uint8_t highpass, uint8_t lowpass);
static sa818_status_t sa818_set_tail_dma(uint8_t tail_on);
static sa818_status_t sa818_scan_frequency_dma(float rx_freq, bool *signal_found);
static sa818_status_t sa818_get_rssi_dma(uint8_t *rssi);
static sa818_status_t sa818_get_version_dma(char *version_str, int max_len);
static sa818_sched_status_t sa818_schedule_cmd(const char *cmd, const char *expect, uint32_t timeout_ms);
static void sa818_start_cmd(const char *cmd, const char *expect, uint32_t timeout_ms);
static void sa818_process_response(void);
static bool sa818_is_command_active(void);

// Blocking uart helpers
static sa818_status_t sa818_handshake_blocking(void);
static sa818_status_t sa818_set_group_blocking(const sa818_settings_t *cfg);
static sa818_status_t sa818_set_volume_blocking(uint8_t level);
static sa818_status_t sa818_set_filter_blocking(uint8_t pre_de_emph, uint8_t highpass, uint8_t lowpass);
static sa818_status_t sa818_set_tail_blocking(uint8_t tail_on);
static sa818_status_t sa818_scan_frequency_blocking(float rx_freq, bool *signal_found);
static sa818_status_t sa818_get_rssi_blocking(uint8_t *rssi);
static sa818_status_t sa818_get_version_blocking(char *version_str, int max_len);
static sa818_status_t sa818_send_cmd_blocking(const char *cmd, const char *expect, char *resp, int resp_len, int timeout_ms);

// GPIO helpers
static void sa818_set_power_on_off(pin_level_t level);
static void sa818_set_ptt_level(pin_level_t level);
static void sa818_set_low_power(void);
static void sa818_set_high_power(void);

// ---------------------------------------------------------------------------
// Init
// ---------------------------------------------------------------------------
sa818_status_t sa818_init(void)
{
    sa818_uart_init();
    sa818_set_ptt_level(HIGH); // HIGH = RX mode
    sa818_set_low_power();
    sa818_set_power_on_off(HIGH); // HIGH = power ON
    HAL_Delay(300); // allow SA818 to boot fully, less might be fine as well
    sa818_uart_flush();

    memset(&sa818_settings, 0, sizeof(sa818_settings_t));

    sa818_settings.bandwidth     = 0;
    sa818_settings.tx_frequency  = 144.4500f;
    sa818_settings.rx_frequency  = 144.4500f;
    strncpy(sa818_settings.tx_subaudio, "0000", sizeof(sa818_settings.tx_subaudio));
    strncpy(sa818_settings.rx_subaudio, "0000", sizeof(sa818_settings.rx_subaudio));
    sa818_settings.squelch       = 4;
    sa818_settings.volume        = 5;
    sa818_settings.pre_de_emph   = 0;
    sa818_settings.highpass      = 0;
    sa818_settings.lowpass       = 0;
    sa818_settings.tail_tone     = 0;
    sa818_settings.mode          = SA818_MODE_RX;
    sa818_settings.power         = SA818_POWER_LOW;

    sa818_state = SA818_CMD_IDLE;
    sa818_pending.active = false;
    last_rssi_poll = HAL_GetTick();

	if (sa818_handshake_blocking() != SA818_OK) {
		return SA818_ERROR;
	}

	if (sa818_set_group_blocking(&sa818_settings) != SA818_OK) {
		return SA818_ERROR;
	}

	if (sa818_set_volume_blocking(sa818_settings.volume) != SA818_OK) {
		return SA818_ERROR;
	}

	if (sa818_set_filter_blocking(sa818_settings.pre_de_emph,
					   sa818_settings.highpass,
					   sa818_settings.lowpass) != SA818_OK) {
		return SA818_ERROR;
	}

	if (sa818_set_tail_blocking(sa818_settings.tail_tone) != SA818_OK) {
		return SA818_ERROR;
	}

	return SA818_OK;
}

// ---------------------------------------------------------------------------
// Task (non-blocking, handles all SA818 commands including RSSI)
// ---------------------------------------------------------------------------
void sa818_task(void)
{
    uint32_t now = HAL_GetTick();

    switch (sa818_state)
    {
    case SA818_CMD_IDLE:
        // If we have a queued command, start it
        if (sa818_pending.active) {
            sa818_start_cmd(sa818_pending.cmd, sa818_pending.expect, sa818_pending.timeout_ms);
            sa818_pending.active = false;
            break;
        }

        // Otherwise handle periodic RSSI polling
        if (sa818_settings.mode == SA818_MODE_RX &&
            now - last_rssi_poll >= SA818_RSSI_POLL_INTERVAL_MS) {
            sa818_start_cmd("RSSI?\r\n", "RSSI=", SA818_CMD_TIMEOUT_MS);
            last_rssi_poll = now;
        }
        break;

    case SA818_CMD_TX:
        if (sa818_uart_tx_done()) {
            sa818_uart_rx_dma(sa818_rxbuf, sizeof(sa818_rxbuf));
            sa818_state = SA818_CMD_RX_WAIT;
        }
        break;

    case SA818_CMD_RX_WAIT:
        if (sa818_uart_rx_done()) {
            sa818_state = SA818_CMD_PROCESS;
        } else if (now > sa818_deadline) {
            sa818_uart_abort_rx();
            sa818_state = SA818_CMD_IDLE;
        }
        break;

    case SA818_CMD_PROCESS:
        sa818_process_response();
        sa818_state = SA818_CMD_IDLE;
        break;
    }
}

const sa818_settings_t* sa818_get_settings(void) {
    return &sa818_settings;
}

void sa818_set_bandwidth(uint8_t bw)
{
    sa818_settings.bandwidth = bw ? 1 : 0;
    sa818_set_group_dma(&sa818_settings);
}

void sa818_set_tx_frequency(float freq)
{
    sa818_settings.tx_frequency = freq;
    // Update both TX/RX frequencies (they share same group config)
    sa818_set_group_dma(&sa818_settings);
}

void sa818_set_rx_frequency(float freq)
{
    sa818_settings.rx_frequency = freq;
    sa818_set_group_dma(&sa818_settings);
}

void sa818_set_tx_subaudio(const char *code)
{
    if (code) {
        strncpy(sa818_settings.tx_subaudio, code, sizeof(sa818_settings.tx_subaudio) - 1);
        sa818_settings.tx_subaudio[sizeof(sa818_settings.tx_subaudio) - 1] = '\0';
    }
    sa818_set_group_dma(&sa818_settings);
}

void sa818_set_rx_subaudio(const char *code)
{
    if (code) {
        strncpy(sa818_settings.rx_subaudio, code, sizeof(sa818_settings.rx_subaudio) - 1);
        sa818_settings.rx_subaudio[sizeof(sa818_settings.rx_subaudio) - 1] = '\0';
    }
    sa818_set_group_dma(&sa818_settings);
}

void sa818_set_squelch(uint8_t sq)
{
    if (sq > 8) sq = 8;
    sa818_settings.squelch = sq;
    sa818_set_group_dma(&sa818_settings);
}

void sa818_set_volume_level(uint8_t vol)
{
    if (vol < 1) vol = 1;
    if (vol > 8) vol = 8;
    sa818_settings.volume = vol;
    sa818_set_volume_dma(vol);
}

void sa818_set_pre_de_emph(uint8_t value)
{
    sa818_settings.pre_de_emph = value ? 1 : 0;
    sa818_set_filter_dma(sa818_settings.pre_de_emph,
                         sa818_settings.highpass,
                         sa818_settings.lowpass);
}

void sa818_set_highpass(uint8_t value)
{
    sa818_settings.highpass = value ? 1 : 0;
    sa818_set_filter_dma(sa818_settings.pre_de_emph,
                         sa818_settings.highpass,
                         sa818_settings.lowpass);
}

void sa818_set_lowpass(uint8_t value)
{
    sa818_settings.lowpass = value ? 1 : 0;
    sa818_set_filter_dma(sa818_settings.pre_de_emph,
                         sa818_settings.highpass,
                         sa818_settings.lowpass);
}

void sa818_set_tail_tone(uint8_t value)
{
    sa818_settings.tail_tone = value ? 1 : 0;
    sa818_set_tail_dma(sa818_settings.tail_tone);
}

void sa818_set_mode(sa818_mode_t mode) {
    sa818_settings.mode = mode;
    sa818_set_ptt_level(mode == SA818_MODE_TX ? LOW : HIGH);
    if(mode == SA818_MODE_TX) {
    	testtone_enable(true);
    } else if (mode == SA818_MODE_RX) {
    	testtone_enable(false);
    }
}

void sa818_set_power_level(sa818_power_t power) {
    sa818_settings.power = power;
    if (power == SA818_POWER_HIGH)
        sa818_set_high_power();
    else
        sa818_set_low_power();
}

// ---------------------------------------------------------------------------
// NON-BLOCKING UART HELPERS (internal)
// ---------------------------------------------------------------------------

static sa818_status_t sa818_handshake_dma(void)
{
    sa818_sched_status_t result =
        sa818_schedule_cmd("AT+DMOCONNECT\r\n", "+DMOCONNECT:0", 1000);

    return (result == SA818_SCHED_OK) ? SA818_OK :
           (result == SA818_SCHED_BUSY ? SA818_TIMEOUT : SA818_ERROR);
}

static sa818_status_t sa818_set_group_dma(const sa818_settings_t *cfg)
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

    sa818_sched_status_t result = sa818_schedule_cmd(cmd, "+DMOSETGROUP:0", SA818_CMD_TIMEOUT_MS);
    return (result == SA818_SCHED_OK) ? SA818_OK :
           (result == SA818_SCHED_BUSY ? SA818_TIMEOUT : SA818_ERROR);
}

static sa818_status_t sa818_set_volume_dma(uint8_t level)
{
    if (level < 1) level = 1;
    if (level > 8) level = 8;
    char cmd[32];
    snprintf(cmd, sizeof(cmd), "AT+DMOSETVOLUME=%d\r\n", level);
    sa818_sched_status_t result = sa818_schedule_cmd(cmd, "+DMOSETVOLUME:0", SA818_CMD_TIMEOUT_MS);
    return (result == SA818_SCHED_OK) ? SA818_OK :
           (result == SA818_SCHED_BUSY ? SA818_TIMEOUT : SA818_ERROR);
}

static sa818_status_t sa818_set_filter_dma(uint8_t pre_de_emph, uint8_t highpass, uint8_t lowpass)
{
    char cmd[48];
    snprintf(cmd, sizeof(cmd), "AT+SETFILTER=%d,%d,%d\r\n",
             pre_de_emph, highpass, lowpass);
    sa818_sched_status_t result = sa818_schedule_cmd(cmd, "+DMOSETFILTER:0", SA818_CMD_TIMEOUT_MS);
    return (result == SA818_SCHED_OK) ? SA818_OK :
           (result == SA818_SCHED_BUSY ? SA818_TIMEOUT : SA818_ERROR);
}

static sa818_status_t sa818_set_tail_dma(uint8_t tail_on)
{
    char cmd[32];
    snprintf(cmd, sizeof(cmd), "AT+SETTAIL=%d\r\n", tail_on);
    sa818_sched_status_t result = sa818_schedule_cmd(cmd, "+DMOSETTAIL:0", SA818_CMD_TIMEOUT_MS);
    return (result == SA818_SCHED_OK) ? SA818_OK :
           (result == SA818_SCHED_BUSY ? SA818_TIMEOUT : SA818_ERROR);
}

static sa818_status_t sa818_scan_frequency_dma(float rx_freq, bool *signal_found)
{
    if (signal_found == NULL)
        return SA818_ERROR;

    char cmd[32];
    snprintf(cmd, sizeof(cmd), "S+%.4f\r\n", rx_freq);
    sa818_sched_status_t result = sa818_schedule_cmd(cmd, "S=", SA818_CMD_TIMEOUT_MS);

    if (result != SA818_SCHED_OK)
        return SA818_ERROR;

    // Response "S=0" means signal found, "S=1" means none
    // Parsing is handled in sa818_process_response
    return SA818_OK;
}

static sa818_status_t sa818_get_rssi_dma(uint8_t *rssi)
{
    if (rssi == NULL)
        return SA818_ERROR;

    sa818_sched_status_t result = sa818_schedule_cmd("RSSI?\r\n", "RSSI=", SA818_CMD_TIMEOUT_MS);
    return (result == SA818_SCHED_OK) ? SA818_OK :
           (result == SA818_SCHED_BUSY ? SA818_TIMEOUT : SA818_ERROR);
}

static sa818_status_t sa818_get_version_dma(char *version_str, int max_len)
{
    if (version_str == NULL || max_len <= 0)
        return SA818_ERROR;

    sa818_sched_status_t result = sa818_schedule_cmd("AT+VERSION\r\n", "+VERSION:", SA818_CMD_TIMEOUT_MS);
    if (result != SA818_SCHED_OK)
        return SA818_ERROR;

    // Version parsing done asynchronously in sa818_process_response
    version_str[0] = '\0';
    return SA818_OK;
}

static sa818_sched_status_t sa818_schedule_cmd(const char *cmd, const char *expect, uint32_t timeout_ms)
{
    if (cmd == NULL || strlen(cmd) == 0) {
    	return SA818_SCHED_ERROR;
    }

    // Reject new command if one is busy or queued
    if (sa818_state != SA818_CMD_IDLE || sa818_pending.active) {
        return SA818_SCHED_BUSY;
    }

    memset(&sa818_pending, 0, sizeof(sa818_pending));
    strncpy(sa818_pending.cmd, cmd, sizeof(sa818_pending.cmd) - 1);
    if (expect) {
    	strncpy(sa818_pending.expect, expect, sizeof(sa818_pending.expect) - 1);
    } else {
    	sa818_pending.expect[0] = '\0';
    }

    sa818_pending.timeout_ms = timeout_ms;
    sa818_pending.active = true;

    // Cancel RSSI if active
    if (sa818_state == SA818_CMD_RX_WAIT || sa818_state == SA818_CMD_TX) {
        sa818_uart_abort_rx();
        sa818_state = SA818_CMD_IDLE;
    }

    return SA818_SCHED_OK;
}

static void sa818_start_cmd(const char *cmd, const char *expect, uint32_t timeout_ms)
{
    memset(sa818_rxbuf, 0, sizeof(sa818_rxbuf));
    sa818_deadline = HAL_GetTick() + timeout_ms;
    sa818_uart_tx_dma(cmd, strlen(cmd));
    sa818_state = SA818_CMD_TX;
}

static void sa818_process_response(void)
{
    int len = sa818_uart_rx_length();
    if (len <= 0) {
        return;
    }

    sa818_rxbuf[len] = 0;

    // --- Parse RSSI ---
    if (strstr((char*)sa818_rxbuf, "RSSI=")) {
        char *eq = strchr((char*)sa818_rxbuf, '=');
        if (eq) {
            sa818_settings.rssi = (uint8_t)atoi(eq + 1);
        }
    }

    // --- Parse S= (scan result) ---
    if (strstr((char*)sa818_rxbuf, "S=")) {
        char *eq = strchr((char*)sa818_rxbuf, '=');
        if (eq) {
            uint8_t s = (uint8_t)atoi(eq + 1);
            sa818_settings.signal_present = (s == 0); // S=0 means signal
        }
    }

    // --- Parse version ---
    if (strstr((char*)sa818_rxbuf, "+VERSION:")) {
        char *ver = strstr((char*)sa818_rxbuf, "+VERSION:");
        if (ver) {
            strncpy(sa818_settings.version, ver + 9, sizeof(sa818_settings.version) - 1);
            sa818_settings.version[sizeof(sa818_settings.version) - 1] = '\0';
        }
    }
}

static bool sa818_is_command_active(void) {
    return (sa818_state != SA818_CMD_IDLE || sa818_pending.active);
}

// ---------------------------------------------------------------------------
// Blocking uart helpers
// ---------------------------------------------------------------------------

// AT+DMOCONNECT
static sa818_status_t sa818_handshake_blocking(void) {
    return sa818_send_cmd_blocking("AT+DMOCONNECT\r\n", "+DMOCONNECT:0", NULL, 0, 10000);
}

// AT+DMOSETGROUP=BW,TX_F,RX_F,TxSub,SQ,RxSub
static sa818_status_t sa818_set_group_blocking(const sa818_settings_t *cfg) {
  char cmd[96];
  snprintf(cmd, sizeof(cmd),
           "AT+DMOSETGROUP=%d,%.4f,%.4f,%s,%d,%s\r\n",
           cfg->bandwidth,
           cfg->tx_frequency,
           cfg->rx_frequency,
           cfg->tx_subaudio,
           cfg->squelch,
           cfg->rx_subaudio);

  return sa818_send_cmd_blocking(cmd, "+DMOSETGROUP:0", NULL, 0, 300);
}

// AT+DMOSETVOLUME=X
static sa818_status_t sa818_set_volume_blocking(uint8_t level) {
  char cmd[32];
  snprintf(cmd, sizeof(cmd), "AT+DMOSETVOLUME=%d\r\n", level);
  return sa818_send_cmd_blocking(cmd, "+DMOSETVOLUME:0", NULL, 0, 300);
}

// AT+SETFILTER=PRE/DE-EMPH,HIGHPASS,LOWPASS
static sa818_status_t sa818_set_filter_blocking(uint8_t pre_de_emph, uint8_t highpass, uint8_t lowpass) {
  char cmd[48];
  snprintf(cmd, sizeof(cmd), "AT+SETFILTER=%d,%d,%d\r\n", pre_de_emph, highpass, lowpass);
  return sa818_send_cmd_blocking(cmd, "+DMOSETFILTER:0", NULL, 0, 300);
}

// AT+SETTAIL=TAIL
static sa818_status_t sa818_set_tail_blocking(uint8_t tail_on) {
  char cmd[32];
  snprintf(cmd, sizeof(cmd), "AT+SETTAIL=%d\r\n", tail_on);
  return sa818_send_cmd_blocking(cmd, "+DMOSETTAIL:0", NULL, 0, 300);
}

// S+Rx_F  --> S=0 or S=1
static sa818_status_t sa818_scan_frequency_blocking(float rx_freq, bool *signal_found) {
  char cmd[32], resp[32];
  snprintf(cmd, sizeof(cmd), "S+%.4f\r\n", rx_freq);

  if (sa818_send_cmd_blocking(cmd, "S=", resp, sizeof(resp), 300) != SA818_OK)
    return SA818_ERROR;

  char *eq = strchr(resp, '=');
  if (!eq) return SA818_ERROR;
  *signal_found = (*(eq + 1) == '0');
  return SA818_OK;
}

// RSSI? --> RSSI=X
static sa818_status_t sa818_get_rssi_blocking(uint8_t *rssi) {
  char resp[32];
  if (sa818_send_cmd_blocking("RSSI?\r\n", "RSSI=", resp, sizeof(resp), 300) != SA818_OK)
    return SA818_ERROR;

  char *eq = strchr(resp, '=');
  if (!eq) return SA818_ERROR;
  *rssi = (uint8_t)atoi(eq + 1);
  return SA818_OK;
}

// AT+VERSION --> +VERSION:SA818_Vx.x
static sa818_status_t sa818_get_version_blocking(char *version_str, int max_len) {
  char resp[64];
  if (sa818_send_cmd_blocking("AT+VERSION\r\n", "+VERSION:", resp, sizeof(resp), 300) != SA818_OK) {
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

static sa818_status_t sa818_send_cmd_blocking(const char *cmd, const char *expect, char *resp, int resp_len, int timeout_ms) {
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

// ---------------------------------------------------------------------------
// GPIO helpers
// ---------------------------------------------------------------------------
static void sa818_set_power_on_off(pin_level_t level) {
    HAL_GPIO_WritePin(SA818_PD_GPIO_Port, SA818_PD_Pin,
                      level ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void sa818_set_ptt_level(pin_level_t level) {
    HAL_GPIO_WritePin(SA818_PTT_GPIO_Port, SA818_PTT_Pin,
                      level ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void sa818_set_low_power(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = SA818_HL_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(SA818_HL_GPIO_Port, &GPIO_InitStruct);
    HAL_GPIO_WritePin(SA818_HL_GPIO_Port, SA818_HL_Pin, GPIO_PIN_RESET);
}

static void sa818_set_high_power(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = SA818_HL_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(SA818_HL_GPIO_Port, &GPIO_InitStruct);
}
