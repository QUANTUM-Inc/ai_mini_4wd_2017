/*
 * Copyright (c) 2017 Quantum.inc. All rights reserved.
 * This software is released under the MIT License, see LICENSE.txt.
 */

#ifndef TINYX4A_I2C_H_
#define TINYX4A_I2C_H_

typedef void (*SlaveStartCallback)(uint8_t);
typedef uint8_t (*SlaveTxCallback)(void);
typedef void (*SlaveRxCallback)(uint8_t);


int8_t i2cInitAsSlave(SlaveStartCallback startCallback, SlaveTxCallback txCallback, SlaveRxCallback rxCallback, uint8_t slaveAddr);

#endif /* TINYX4A_I2C_H_ */