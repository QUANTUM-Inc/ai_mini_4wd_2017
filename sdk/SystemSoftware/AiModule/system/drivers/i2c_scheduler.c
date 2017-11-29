/*
 * Copyright (c) 2017 Quantum.inc. All rights reserved.
 * This software is released under the MIT License, see LICENSE.txt.
 */

#include <stdint.h>
#include <string.h>

#include <xmegaA4U_utils.h>
#include <xmegaA4U_i2c.h>

#include <error.h>

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
#define DESC_SIZE			(8)

typedef struct I2cDescriptor_t
{
	uint8_t address;
	uint8_t *txbuf;
	uint8_t  txlen;
	
	uint8_t *rxbuf;
	uint8_t  rxlen;
	
	i2c_done_callback cb;	
} I2cDescriptor;

typedef struct I2cSchedulerContext_t
{
	I2cDescriptor descs[DESC_SIZE];
	uint8_t wptr;
	uint8_t rptr;
} I2cSchedulerContext;


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
#define REMAINING_SIZE(c)		(((c).wptr - (c).rptr + DESC_SIZE) & (DESC_SIZE-1))

#define BUFFER_ENPTY(c)			((c).wptr == (c).rptr)
#define BUFFER_FULL(c)			(REMAINING_SIZE(c) == (DESC_SIZE-1))

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
static I2cSchedulerContext sCtx;


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
static void _i2c_txrx_done_cb(uint8_t status);
static int8_t _i2c_kick(I2cDescriptor *desc);

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
int8_t initialize_i2c_scheduler(uint32_t scl_clock, uint32_t systemClock)
{
	int8_t ret = initialize_i2c(I2C_ON_PORTE, scl_clock, systemClock, I2C_MASTER, NULL);
	if (ret != AI_OK) {
		;
	}
	
	memset (&sCtx, 0x00, sizeof(sCtx));

	return ret;
}


/*--------------------------------------------------------------------------*/
int8_t i2cSchedulerPushTask(uint8_t address, uint8_t *txbuf, uint8_t txlen, uint8_t *rxbuf, uint8_t rxlen, i2c_done_callback cb)
{
	int8_t ret = 0;
	
	Disable_Int();
	uint8_t kick_driver = 0;
	if (BUFFER_ENPTY(sCtx)) {
		kick_driver = 1;
	}
	
	if (BUFFER_FULL(sCtx)) {
		ret = AI_ERROR_NOBUF;
		goto ENSURE;
	}

	sCtx.descs[sCtx.wptr].address = address;
	sCtx.descs[sCtx.wptr].txbuf   = txbuf;
	sCtx.descs[sCtx.wptr].txlen   = txlen;
	sCtx.descs[sCtx.wptr].rxbuf   = rxbuf;
	sCtx.descs[sCtx.wptr].rxlen   = rxlen;
	sCtx.descs[sCtx.wptr].cb      = cb;
	
	sCtx.wptr = (sCtx.wptr + 1) & (DESC_SIZE-1);
	
	if (kick_driver) {
		ret = _i2c_kick(&sCtx.descs[sCtx.rptr]);
	}

ENSURE:
	Enable_Int();
	
	return ret;
}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
static void _i2c_txrx_done_cb(uint8_t status)
{
	if (sCtx.descs[sCtx.rptr].cb) {
		sCtx.descs[sCtx.rptr].cb(status);
	}
	
	sCtx.rptr = (sCtx.rptr + 1) & (DESC_SIZE - 1);
	
	if (REMAINING_SIZE(sCtx) != 0) {
		(void)_i2c_kick(&sCtx.descs[sCtx.rptr]);
	}

	return;
}

/*--------------------------------------------------------------------------*/
static int8_t _i2c_kick(I2cDescriptor *desc)
{
	int ret = i2c_txRxBytes_w_cb(
							I2C_ON_PORTE,
							desc->address, 
							desc->txbuf, desc->txlen,
							desc->rxbuf, desc->rxlen,
							_i2c_txrx_done_cb
						);
	
	return ret;
}