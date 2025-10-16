#ifndef ERROR_WINDOW_H
#define ERROR_WINDOW_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * @brief Display a blocking error message on the LCD.
 *        This halts normal UI updates for the duration.
 * @param message     The message to show (up to ~32 chars).
 * @param delay_ms    How long to show it (default = 1000 ms if 0 passed).
 */
void error_window_show(const char *message, uint32_t delay_ms);

/**
 * @brief Helper macro for showing a standard 1 s blocking error.
 */
#define ERROR_WINDOW_SHOW(msg)  error_window_show((msg), 1000)

#ifdef __cplusplus
}
#endif

#endif // ERROR_WINDOW_H
