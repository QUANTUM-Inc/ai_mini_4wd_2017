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
static void _sw0_pressed(void);
static void _sw1_pressed(void);
static void _sw2_pressed(void);
static void _sw2_released(void);

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
	//J SW—pCB“o˜^
	aiMini4WdHidsRegisterSwCallback(cAiMini4WdHidsSwCbOnPress,   cAiMini4WdHidsSw0, _sw0_pressed);
	aiMini4WdHidsRegisterSwCallback(cAiMini4WdHidsSwCbOnPress,   cAiMini4WdHidsSw1, _sw1_pressed);
	aiMini4WdHidsRegisterSwCallback(cAiMini4WdHidsSwCbOnPress,   cAiMini4WdHidsSw2, _sw2_pressed);
	aiMini4WdHidsRegisterSwCallback(cAiMini4WdHidsSwCbOnRelease, cAiMini4WdHidsSw2, _sw2_released);

	return AI_OK;
}

/*--------------------------------------------------------------------------*/
static void _sw0_pressed(void)
{
	aiMini4WdHidsToggleLed(AI_MINI_4WD_LED0);
}

/*--------------------------------------------------------------------------*/
static void _sw1_pressed(void)
{
	aiMini4WdHidsToggleLed(AI_MINI_4WD_LED1);
}

/*--------------------------------------------------------------------------*/
static void _sw2_pressed(void)
{
	aiMini4WdHidsSetLed(AI_MINI_4WD_LED2);
}

/*--------------------------------------------------------------------------*/
static void _sw2_released(void)
{
	aiMini4WdHidsClearLed(AI_MINI_4WD_LED2);
}

