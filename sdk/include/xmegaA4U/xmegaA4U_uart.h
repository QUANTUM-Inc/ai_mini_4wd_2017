/*
 * Copyright (c) 2017 Quantum.inc. All rights reserved.
 * This software is released under the MIT License, see LICENSE.txt.
 */

#ifndef UART_DRIVER_H_
#define UART_DRIVER_H_

#include <xmegaA4U_gpio.h>

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
#define UART_OK								(0)
#define UART_BUFFER_IS_NULL					(-1)
#define UART_INVALID_BAUDRATE				(-2)
#define UART_BUFFER_FULL					(-3)
#define UART_NOT_ENOUGH_BUFFER				(-4)
#define UART_BUFFER_EMPTY					(-5)
#define UART_INVALID_UART_TYPE				(-6)
#define UART_IS_ALREADY_INITIALIZED			(-7)

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
#define USART0_ON_PORTC						(0)
#define USART1_ON_PORTC						(1)
#define USART0_ON_PORTD						(2)
#define USART1_ON_PORTD						(3)
#define USART0_ON_PORTE						(4)

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
uint8_t initialize_uart(uint8_t usartType, uint32_t baudrate, uint32_t peripheralClock);

uint8_t uart_tx(uint8_t *dat, uint32_t len);
uint8_t uart_txNonBuffer(uint8_t *dat, uint32_t len);

uint8_t uart_rx(uint8_t *dat, uint32_t *len);
uint8_t uart_rx_n(uint8_t *dat, uint32_t rlen, uint32_t *len);
uint32_t uart_get_rxlen(void);
uint8_t uart_rxBlocking(uint8_t *dat, uint32_t rlen);

/*---------------------------------------------------------------------------*/
uint8_t initialize_uart_spi(uint8_t usartType, uint8_t spi_mode, GPIO_PORT ss_port, GPIO_PIN ss_pin, uint32_t spi_clk, uint32_t peripheralClock);
uint8_t uart_spi_txrx(uint8_t *buf, const uint32_t len);
uint8_t uart_spi_txrx_wo_ss(uint8_t *buf, const uint32_t len);
void uart_spi_ss(uint8_t ss_out);


#endif /* UART_DRIVER_H_ */