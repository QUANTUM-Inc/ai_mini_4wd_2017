/*
 * Copyright (c) 2017 Quantum.inc. All rights reserved.
 * This software is released under the MIT License, see LICENSE.txt.
 */

#ifndef TINYX4A_TIMER_H_
#define TINYX4A_TIMER_H_

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
#define ERROR_TIMER_OUT_OF_RANGE		(-1)

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
typedef uint8_t (*timer_callback)(void);

typedef enum TIMER0_FAST_PWM_OUTPUT_PIN_t
{
	TIMER0_FAST_PWM_OC0A	= 0x01,
	TIMER0_FAST_PWM_OC0B	= 0x10,
	TIMER0_FAST_PWM_OC0A_I	= 0x03,
	TIMER0_FAST_PWM_OC0B_I	= 0x30,
	TIMER0_FAST_PWM_DISABLE	= 0x00
} TIMER0_FAST_PWM_OUTPUT_PIN;


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
uint8_t initializeTimer0(uint32_t sysClkInHz, uint32_t intervalInHz, timer_callback cb);
uint8_t initializeTimer0MicroSec(uint32_t sysClkInHz, uint32_t intervalInMicroSec, timer_callback cb);
uint8_t initializeTimer0FastPwm(uint32_t sysClkInHz, uint8_t initialDuty);
uint8_t timer0_enableFastPwm(TIMER0_FAST_PWM_OUTPUT_PIN pin_mode);
uint8_t timer0_setFastPwmDuty(uint8_t duty);

uint8_t initializeTimer1(uint32_t sysClkInHz, uint32_t intervalInMiliSec, timer_callback cb);

uint8_t enableTimer0(uint8_t enable);
uint8_t enableTimer1(uint8_t enable);


#endif /* TINYX5A_TIMER_H_ */