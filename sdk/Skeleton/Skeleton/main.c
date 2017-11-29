/*
 * Copyright (c) 2017 Quantum.inc. All rights reserved.
 * This software is released under the MIT License, see LICENSE.txt.
 */

#include <stdint.h>

#include <error.h>
#include <system.h>
#include <hids.h>

int main(void)
{
	int ret = 0;
	
	ret = aiMini4WdSystemInitialize(AI_SYSTEM_FUNCTION_BASE);
	if (ret != AI_OK) {
		
	}
	
	while(1) {
		
	}
	
}

