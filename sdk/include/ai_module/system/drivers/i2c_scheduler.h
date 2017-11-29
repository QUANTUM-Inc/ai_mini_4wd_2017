/*
 * Copyright (c) 2017 Quantum.inc. All rights reserved.
 * This software is released under the MIT License, see LICENSE.txt.
 */

#ifndef I2C_SCHEDULER_H_
#define I2C_SCHEDULER_H_


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
int8_t initialize_i2c_scheduler(uint32_t scl_clock, uint32_t systemClock);
int8_t i2cSchedulerPushTask(uint8_t address, uint8_t *txbuf, uint8_t txlen, uint8_t *rxbuf, uint8_t rxlen, i2c_done_callback cb);


#endif /* I2C_SCHEDULER_H_ */