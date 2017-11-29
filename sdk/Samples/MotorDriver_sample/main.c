/*
 * Copyright (c) 2017 Quantum.inc. All rights reserved.
 * This software is released under the MIT License, see LICENSE.txt.
 */

#include <stdint.h>

#include <error.h>
#include <system.h>
#include <motor_driver.h>


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
static int _initialize_user(void);

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

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
		volatile uint32_t wait = 0x7ffff;
		while (wait--);

		aiMini4WdMotorDriverSetDuty(cAiMini4WdModeFoward, 255);

		wait = 0x7ffff;
		while (wait--);
		
		aiMini4WdMotorDriverBreak();

		wait = 0x7ffff;
		while (wait--);

		aiMini4WdMotorDriverSetDuty(cAiMini4WdModeBack, 255);
	}	

	return 0;
}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
static int _initialize_user(void)
{
	return AI_OK;
}

