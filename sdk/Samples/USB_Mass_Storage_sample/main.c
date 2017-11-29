/*
 * Copyright (c) 2017 Quantum.inc. All rights reserved.
 * This software is released under the MIT License, see LICENSE.txt.
 */

#include <stdint.h>

#include <error.h>
#include <system.h>
#include <hids.h>

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
static int _initialize_user(void);

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
int main(void)
{
	int ret = 0;
	
	ret = aiMini4WdSystemInitialize(AI_SYSTEM_FUNCTION_BASE | AI_SYSTEM_FUNCTION_USB);
	if (ret != AI_OK) {
		
	}
	
	ret = _initialize_user();
	if (ret != AI_OK) {
		
	}

	uint8_t on_off = 0;
	while (1) {
		volatile uint32_t wait = 0x3ffff;
		while (wait--);
		
		if (on_off) {
			aiMini4WdHidsSetLed(AI_MINI_4WD_LED0);
			aiMini4WdHidsClearLed(AI_MINI_4WD_LED1);
		}
		else {
			aiMini4WdHidsClearLed(AI_MINI_4WD_LED0);
			aiMini4WdHidsSetLed(AI_MINI_4WD_LED1);
		}

		aiMini4WdHidsToggleLed(AI_MINI_4WD_LED2);
		
		on_off = 1-on_off;
	}

	return 0;
}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
static int _initialize_user(void)
{
	return AI_OK;
}