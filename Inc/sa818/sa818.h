#ifndef __SA818_H
#define __SA818_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

// ---------------------------------------------------------------------------
// Types
// ---------------------------------------------------------------------------
typedef enum {
    SA818_OK = 0,
    SA818_ERROR,
    SA818_TIMEOUT
} sa818_status_t;

typedef enum {
    SA818_MODE_RX = 0,
    SA818_MODE_TX
} sa818_mode_t;

typedef enum {
    SA818_POWER_LOW = 0,
    SA818_POWER_HIGH
} sa818_power_t;

typedef struct {
    uint8_t bandwidth;
    float tx_frequency;
    float rx_frequency;
    char tx_subaudio[5];
    char rx_subaudio[5];
    uint8_t squelch;
    uint8_t volume;
    uint8_t pre_de_emph;
    uint8_t highpass;
    uint8_t lowpass;
    uint8_t tail_tone;
    uint8_t rssi;
    bool signal_present;
    char version[16];
    sa818_mode_t mode;
    sa818_power_t power;
} sa818_settings_t;


sa818_status_t sa818_init(void);
void sa818_task(void);  // periodic task for RSSI updates

const sa818_settings_t* sa818_get_settings(void);

void sa818_set_bandwidth(uint8_t bw);
void sa818_set_tx_frequency(float freq);
void sa818_set_rx_frequency(float freq);
void sa818_set_tx_subaudio(const char *code);
void sa818_set_rx_subaudio(const char *code);
void sa818_set_squelch(uint8_t sq);
void sa818_set_volume_level(uint8_t vol);
void sa818_set_pre_de_emph(uint8_t value);
void sa818_set_highpass(uint8_t value);
void sa818_set_lowpass(uint8_t value);
void sa818_set_tail_tone(uint8_t value);
void sa818_set_mode(sa818_mode_t mode);
void sa818_set_power_level(sa818_power_t power);

#ifdef __cplusplus
}
#endif

#endif /* __SA818_H */
