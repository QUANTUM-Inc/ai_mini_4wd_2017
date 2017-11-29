/*
 * Copyright (c) 2017 Quantum.inc. All rights reserved.
 * This software is released under the MIT License, see LICENSE.txt.
 */

#ifndef TIMER_DRIVER_H_
#define TIMER_DRIVER_H_


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
#define TIMER_OK								(0)
#define TIMER_CALLBACK_IS_ALREADY_REGISTERD		(-1)
#define TIMER_CANNOT_CONFIGURATE				(-2)

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
typedef void (*timer_callback)(void);

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
uint8_t initialize_tcc0_as_timerMilSec(uint32_t periodInMiliSec, uint32_t peripheralClock);
uint8_t initialize_tcc0_as_timerMicroSec(uint32_t periodInMicroSec, uint32_t peripheralClock);
uint8_t tcc0_timer_registerCallback(timer_callback cb);
uint32_t tcc0_timer_getCurrentCount(void);

uint8_t initialize_tcc1_as_timerMilSec(uint32_t periodInMiliSec, uint32_t peripheralClock);
uint8_t initialize_tcc1_as_timerMicroSec(uint32_t periodInMicroSec, uint32_t peripheralClock);
uint8_t tcc1_timer_registerCallback(timer_callback cb);
uint32_t tcc1_timer_getCurrentCount(void);


#endif /* TIMER_DRIVER_H_ */