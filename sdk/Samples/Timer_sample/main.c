/*
 * Copyright (c) 2017 Quantum.inc. All rights reserved.
 * This software is released under the MIT License, see LICENSE.txt.
 */

#include <stdint.h>

#include <error.h>
#include <system.h>
#include <hids.h>
#include <timer.h>

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
static int _initialize_user(void);
static void _100ms_timer(void);


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
int main(void)
{
	int ret = 0;
	
	ret = aiMini4WdSystemInitialize(AI_SYSTEM_FUNCTION_BASE);
	if (ret != AI_OK) {
		
	}
	
	ret = _initialize_user();
	if (ret != AI_OK) {
		
	}
	
	while (1) {
		;
	}	

	return 0;
}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
static int _initialize_user(void)
{
	aiMini4WdTimerRegister100msCb(_100ms_timer);
	
	return AI_OK;
}

/*--------------------------------------------------------------------------*/
static uint8_t cnt = 0;
static void _100ms_timer(void)
{
	cnt++;
	
	aiMini4WdHidsToggleLed(AI_MINI_4WD_LED0);
	
	if ((cnt % 10) == 0) {
		aiMini4WdHidsToggleLed(AI_MINI_4WD_LED1);
	}

	if ((cnt % 20) == 0) {
		aiMini4WdHidsToggleLed(AI_MINI_4WD_LED2);
	}
	
	return;
}

