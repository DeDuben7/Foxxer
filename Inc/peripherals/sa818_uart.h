/**
 ******************************************************************************
 * @file      uart.h
 * @brief     init and control functions for the uart
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
 * @addtogroup  Peripherals
 * @{
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __UART_H
#define __UART_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes -------------------------------------------------------------------*/

/* Defines -------------------------------------------------------------------*/

/* Typedefs -------------------------------------------------------------------*/

/* Functions -------------------------------------------------------------------*/

extern void sa818_uart_init(void);

extern void sa818_uart_transmit(const char* cmd);
extern size_t sa818_uart_receive(const char* rxbuf, size_t size, uint32_t timeout);
extern void sa818_uart_flush(void);

#ifdef __cplusplus
}
#endif

#endif /* __UART_H */
