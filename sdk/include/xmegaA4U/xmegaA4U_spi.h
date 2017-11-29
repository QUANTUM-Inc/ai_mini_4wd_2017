/*
 * Copyright (c) 2017 Quantum.inc. All rights reserved.
 * This software is released under the MIT License, see LICENSE.txt.
 */

#ifndef SPI_DRIVER_H_
#define SPI_DRIVER_H_

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
#define SPI_OK					(0)
#define SPI_INVALID_CLK			(-1)
#define SPI_INVALID_TYPE		(-2)
#define SPI_INVALID_MODE		(-3)
#define SPI_INVALID_ORDER		(-4)
#define SPI_INVALID_PORT		(-5)

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
typedef enum {
	SPI_DATA_ORDER_MSB_FIRST	= 0,
	SPI_DATA_ORDER_LSB_FIRST	= 1
} SPI_DATA_ORDER;

/*---------------------------------------------------------------------------*/
typedef enum {
	SPI_TRANSFER_MODE_0 = 0,	/* On Idle SCK Level = L, Rising Edge = sample, Falling Edge = setup  */
	SPI_TRANSFER_MODE_1 = 1,	/* On Idle SCK Level = L, Rising Edge = setup,  Falling Edge = sample */
	SPI_TRANSFER_MODE_2 = 2,	/* On Idle SCK Level = H, Rising Edge = setup,  Falling Edge = sample */
	SPI_TRANSFER_MODE_3 = 3		/* On Idle SCK Level = H, Rising Edge = sample, Falling Edge = setup  */
} SPI_TRANSFER_MODE;

/*---------------------------------------------------------------------------*/
typedef enum {
	SPI_CLK_DIV_2,
	SPI_CLK_DIV_4,
	SPI_CLK_DIV_8,
	SPI_CLK_DIV_16,
	SPI_CLK_DIV_32,
	SPI_CLK_DIV_64,
	SPI_CLK_DIV_128	
} SPI_CLK_SELECT;

/*---------------------------------------------------------------------------*/
typedef enum {
	SPI_TYPE_4WIRE = 0,
	SPI_TYPE_3WIRE = 1
} SPI_TYPE;


/*---------------------------------------------------------------------------*/
#define SPI_ON_PORTC				(0)
#define SPI_ON_PORTD				(1)

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
uint8_t initialize_spi_master(uint8_t spi_port, SPI_TYPE type, SPI_TRANSFER_MODE mode, SPI_DATA_ORDER order, SPI_CLK_SELECT clk);
uint8_t spi_tx(uint8_t spi_port, uint8_t data);
uint8_t spi_rx(uint8_t spi_port);
uint8_t spi_txrx(uint8_t spi_port, uint8_t *array, uint32_t len);

uint8_t spi_txrx_pio(uint8_t port, uint8_t data);

#endif /* SPI_DRIVER_H_ */