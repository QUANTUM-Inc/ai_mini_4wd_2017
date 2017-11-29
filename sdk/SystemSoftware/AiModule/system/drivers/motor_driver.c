/*
 * Copyright (c) 2017 Quantum.inc. All rights reserved.
 * This software is released under the MIT License, see LICENSE.txt.
 */

#include <stdint.h>

#include <xmegaA4U_utils.h>
#include <xmegaA4U_i2c.h>

#include <error.h>
#include <motor_driver.h>

#include "system/hids_system.h"
#include "system/drivers/i2c_scheduler.h"

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
#define MOTOR_DRIVER_ADDRESS							(0x60)

#define AI_MINI4WD_MOTOR_DRIVER_MODE_BREAK				(0)
#define AI_MINI4WD_MOTOR_DRIVER_MODE_FORWARD			(2)
#define AI_MINI4WD_MOTOR_DRIVER_MODE_BACK				(1)
#define AI_MINI4WD_MOTOR_DRIVER_MODE_FREE				(3)


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
static int8_t _updateMotorDriver(uint8_t mode, uint8_t duty);
static void _i2c_done(uint8_t status);


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
static uint16_t sBatteryLevel_mv = 0;
static volatile uint8_t sCurrentDuty = 0;
static volatile AiMini4WdMotorDriverMode sCurrentMode = cAiMini4WdModeFree;

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
int8_t aiMini4WdMotorDriverSetDuty(AiMini4WdMotorDriverMode direction, uint8_t duty)
{
	uint8_t drive_mode = AI_MINI4WD_MOTOR_DRIVER_MODE_FORWARD;
	if (direction == cAiMini4WdModeFoward) {
		drive_mode = AI_MINI4WD_MOTOR_DRIVER_MODE_FORWARD;
	}
	else if (direction == cAiMini4WdModeBack) {
		drive_mode = AI_MINI4WD_MOTOR_DRIVER_MODE_BACK;
	}
	else {
		return AI_ERROR_INVALID_DRIVE_MODE;
	}

	return _updateMotorDriver(drive_mode, duty);
}

/*--------------------------------------------------------------------------*/
int8_t aiMini4WdMotorDriverBreak(void)
{
	return _updateMotorDriver(AI_MINI4WD_MOTOR_DRIVER_MODE_BREAK, 0);
}

/*--------------------------------------------------------------------------*/
int8_t aiMini4WdMotorDriverNeutral(void)
{
	return _updateMotorDriver(AI_MINI4WD_MOTOR_DRIVER_MODE_FREE, 0);
}

/*--------------------------------------------------------------------------*/
int8_t aiMini4WdMotorDriverGetBatteryLevel(uint16_t *battery_mv)
{
	if (battery_mv == NULL) {
		return AI_ERROR_NULL;
	}
	
	*battery_mv = sBatteryLevel_mv;
	
	return AI_OK;
}

/*--------------------------------------------------------------------------*/
int8_t aiMini4WdMotorDriverGetCurrentSettings(AiMini4WdMotorDriverMode *mode, uint8_t *duty)
{
	if ((mode == NULL) || (duty == NULL)) {
		return AI_ERROR_NULL;
	}
	
	*mode = sCurrentMode;
	*duty = sCurrentDuty;
	
	return AI_OK;
}



/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
static uint8_t sDriveData[2];
static uint8_t sBatteryLevelBuf[2];
static int8_t _updateMotorDriver(uint8_t mode, uint8_t duty)
{
	if (mode == AI_MINI4WD_MOTOR_DRIVER_MODE_BREAK) {
		sCurrentMode = cAiMini4WdModeBreak;
	}
	else if (mode == AI_MINI4WD_MOTOR_DRIVER_MODE_FREE) {
		sCurrentMode = cAiMini4WdModeFree;
	}
	else if (mode == AI_MINI4WD_MOTOR_DRIVER_MODE_FORWARD) {
		sCurrentMode = cAiMini4WdModeFoward;
	}
	else if (mode == AI_MINI4WD_MOTOR_DRIVER_MODE_BACK) {
		sCurrentMode = cAiMini4WdModeBack;
	}
	sCurrentDuty = duty;

	sDriveData[0] = mode;
	sDriveData[1] = duty;

	int8_t ret = i2cSchedulerPushTask(MOTOR_DRIVER_ADDRESS, sDriveData, 2, sBatteryLevelBuf, 2, _i2c_done);

	return ret;	
}

static void _i2c_done(uint8_t status)
{
	if (status == AI_OK) {
		sBatteryLevel_mv = ((uint16_t)sBatteryLevelBuf[0]) << 8 | sBatteryLevelBuf[1];
	}
}


