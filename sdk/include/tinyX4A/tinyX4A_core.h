/*
 * Copyright (c) 2017 Quantum.inc. All rights reserved.
 * This software is released under the MIT License, see LICENSE.txt.
 */


#ifndef TINYX4A_CORE_H_
#define TINYX4A_CORE_H_

#include <stdint.h>

#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>


#define MCUCR_SLEEP_MODE_MASK						(0x03 << SM0)
#define MCUCR_SLEEP_ENABLE							(0x01 << SE)

#define TINY_X4A_CORE_SLEEP_ENTER_IDLE() {\
	MCUCR = (MCUCR & ~MCUCR_SLEEP_MODE_MASK) | ((0x00) << SM0);\
	MCUCR |=  MCUCR_SLEEP_ENABLE; \
	__asm__ __volatile__ ( "sleep" "\n\t" :: );\
	MCUCR &= ~MCUCR_SLEEP_ENABLE; \
}

#define TINY_X4A_CORE_SLEEP_ENTER_ADC_LOW_NOISE()  {\
	MCUCR = (MCUCR & ~MCUCR_SLEEP_MODE_MASK) | ((0x01) << SM0);\
	MCUCR |=  MCUCR_SLEEP_ENABLE; \
	__asm__ __volatile__ ( "sleep" "\n\t" :: );\
	MCUCR &= ~MCUCR_SLEEP_ENABLE; \
}

#define TINY_X4A_CORE_SLEEP_ENTER_POWER_DOWN() {\
	MCUCR = (MCUCR & ~MCUCR_SLEEP_MODE_MASK) | ((0x02) << SM0);\
	MCUCR |=  MCUCR_SLEEP_ENABLE; \
	__asm__ __volatile__ ( "sleep" "\n\t" :: );\
	MCUCR &= ~MCUCR_SLEEP_ENABLE; \
}


#endif /* TINYX4A_CORE_H_ */