/*
 * Copyright (c) 2017 Quantum.inc. All rights reserved.
 * This software is released under the MIT License, see LICENSE.txt.
 */

#ifndef TINYX4A_INTERRUPT_H_
#define TINYX4A_INTERRUPT_H_

#include <stdint.h>

/*---------------------------------------------------------------------------*/
#define ERROR_INTERRUPT_INVALID_CONFIGURATION				(-11)

/*---------------------------------------------------------------------------*/
typedef enum InterruptPort_t{
	INTERRUPT_PORT_PCINT0  = 0,	//J PA0
	INTERRUPT_PORT_PCINT1  = 1,	//J PA1
	INTERRUPT_PORT_PCINT2  = 2,	//J PA2
	INTERRUPT_PORT_PCINT3  = 3,	//J PA3
	INTERRUPT_PORT_PCINT4  = 4,	//J PA4
	INTERRUPT_PORT_PCINT5  = 5,	//J PA5
	INTERRUPT_PORT_PCINT6  = 6,	//J PA6
	INTERRUPT_PORT_PCINT7  = 7,	//J PA7
	INTERRUPT_PORT_PCINT8  = 8,	//J PB0
	INTERRUPT_PORT_PCINT9  = 9,	//J PB1
	INTERRUPT_PORT_PCINT10 = 10,//J PB2
	INTERRUPT_PORT_PCINT11 = 11,//J PB3
	INTERRUPT_PORT_INT0    = 12,//J PB2
	INTERRUPT_PORT_MAX     = 13	//J PA7
} InterruptPort;

typedef enum InterruptType_t{
	INTERRUPT_TYPE_PCINT,				//J For PCINTx
	INTERRUPT_TYPE_INT_LOW_LEVEL,		//J For INT0
	INTERRUPT_TYPE_INT_CHANGE_LOGIC,	//J For INT0
	INTERRUPT_TYPE_INT_FALLING_EDGE,	//J For INT0
	INTERRUPT_TYPE_INT_RISING_EDGE		//J For INT0
} InterruptType;

typedef void (*InterruptCallback)(uint8_t);

/*---------------------------------------------------------------------------*/
int interruptRegister(InterruptPort port, InterruptType type, InterruptCallback cb);
int interruptUnregister(InterruptPort port);

#endif /* TINYX4A_INTERRUPT_H_ */