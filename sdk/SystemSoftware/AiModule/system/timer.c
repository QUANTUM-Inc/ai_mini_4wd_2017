/*
 * Copyright (c) 2017 Quantum.inc. All rights reserved.
 * This software is released under the MIT License, see LICENSE.txt.
 */

#include <stdint.h>

#include <xmegaA4U_utils.h>

#include <error.h>
#include <timer.h>

#include "system/timer_system.h"


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
static AiMini4WdTimerCb _5msCb = NULL;
static AiMini4WdTimerCb _100msCb = NULL;

static uint8_t sTimerCnt = 0;

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
int8_t aiMini4WdTimerRegister5msCb(AiMini4WdTimerCb cb)
{
	if (_5msCb == NULL) {
		_5msCb = cb;
	}
	else {
		return AI_ERROR_ALREADY_REGISTERED;
	}
	
	return AI_OK;
}

/*--------------------------------------------------------------------------*/
int8_t aiMini4WdTimerRegister100msCb(AiMini4WdTimerCb cb)
{
	if (_100msCb == NULL) {
		_100msCb = cb;
	}
	else {
		return AI_ERROR_ALREADY_REGISTERED;
	}
	
	return AI_OK;
}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void aiMini4WdTimerUpdate(void)
{
	sTimerCnt++;
	
	if (_5msCb != NULL) {
		_5msCb();
	}
	
	if (sTimerCnt >= (100/5)) {
		sTimerCnt = 0;
		if (_100msCb != NULL) {
			_100msCb();
		}
	}
	
	return;
}
