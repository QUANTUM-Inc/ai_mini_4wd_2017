/*
 * Copyright (c) 2017 Quantum.inc. All rights reserved.
 * This software is released under the MIT License, see LICENSE.txt.
 */

#ifndef PWM_DRIVER_H_
#define PWM_DRIVER_H_

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
#define PWM_OK				(0)


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
#define TCCxIO_REMAP		(1)
#define TCCxIO_NOT_REMAP	(0)

/*---------------------------------------------------------------------------*/
typedef enum {
	TCCxA	= (0),
	TCCxB	= (1),
	TCCxC	= (2),
	TCCxD	= (3)
} PWM_CH;	


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
uint16_t initialize_tcc0_as_pwm(uint32_t carrerFreqInHz, uint32_t peripheralClock, PWM_CH ch);

void tcc0_pwm_start(void);
void tcc0_pwm_stop(void);
void tcc0_pwm_enable(PWM_CH ch, uint8_t enable);

void tcc0_pwm_setDuty(uint16_t duty, PWM_CH idx);
uint16_t tcc0_pwm_getDuty(PWM_CH idx);

#endif /* PWM_DRIVER_H_ */