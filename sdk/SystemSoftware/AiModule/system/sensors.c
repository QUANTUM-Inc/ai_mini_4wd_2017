/*
 * Copyright (c) 2017 Quantum.inc. All rights reserved.
 * This software is released under the MIT License, see LICENSE.txt.
 */

#include <stdint.h>
#include <string.h>

#include <sensors.h>
#include <error.h>

#include <system/sensors_system.h>
#include <system/drivers/adns-9800.h>

/*--------------------------------------------------------------------------*/
static AiMini4WdSensorsData   sCurrentSensorsData;

static AiMini4WdSensorsCapturedCb sCapturedCb;

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
int8_t aiMini4WdSensorsRegisterCapturedCallback(AiMini4WdSensorsCapturedCb cb)
{
	sCapturedCb = cb;
	
	return AI_OK;
}

/*--------------------------------------------------------------------------*/
int8_t aiMini4WdSensorsGetCurrentImuData(AiMini4WdSensorsImuData *imu_data)
{
	if (imu_data == NULL) {
		return AI_ERROR_NULL;
	}

	memcpy (imu_data, &sCurrentSensorsData.imu_data, sizeof(AiMini4WdSensorsImuData));

	return AI_OK;	
}


/*--------------------------------------------------------------------------*/
int8_t aiMini4WdSensorsGetCurrentMouseData(AiMini4WdSensorsMouseData *mouse_data)
{
	if (mouse_data == NULL) {
		return AI_ERROR_NULL;
	}

	memcpy (mouse_data, &sCurrentSensorsData.mouse_data, sizeof(AiMini4WdSensorsMouseData));

	return AI_OK;
}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void aiMini4WdSensorsUpdate(AiMini4WdSensorsImuData *data)
{
	if (data != NULL) {
		memcpy (&sCurrentSensorsData.imu_data, data, sizeof(AiMini4WdSensorsImuData));

		//J Mouseからデータを読み出す
		Adn9800MotionData mouse_data;
		adns9800_getValue(&mouse_data);
		sCurrentSensorsData.mouse_data.delta_x = mouse_data.delta_x;
		sCurrentSensorsData.mouse_data.delta_y = mouse_data.delta_y;
		sCurrentSensorsData.mouse_data.motion  = mouse_data.motion;
		sCurrentSensorsData.mouse_data.reliability = mouse_data.surface_quality;

		if (sCapturedCb) {
			sCapturedCb(&sCurrentSensorsData);
		}
	}

	return;
}
