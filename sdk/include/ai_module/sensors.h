/*
 * sensors.h
 *
 * Created: 2017/08/15 15:31:52
 *  Author: aks
 */ 


#ifndef SENSORS_H_
#define SENSORS_H_

#include <stdint.h>

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
typedef struct AiMini4WdSensorsImu_t
{
	int16_t pitch;
	int16_t roll;
	int16_t yaw;

	int16_t ax;
	int16_t ay;
	int16_t az;
} AiMini4WdSensorsImuData;

typedef struct AiMini4WdSensorsMouseData_t
{
	int16_t delta_x;
	int16_t delta_y;
	uint8_t reliability;
	uint8_t motion;
} AiMini4WdSensorsMouseData;

typedef struct AiMini4WdSensorsData_t
{
	AiMini4WdSensorsImuData imu_data;
	AiMini4WdSensorsMouseData   mouse_data;
} AiMini4WdSensorsData;

typedef void (*AiMini4WdSensorsCapturedCb)(AiMini4WdSensorsData *sensor_data);


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
#define MOUSE_VAL_TO_MM(c)					((c)  / 128)
#define MM_TO_MOUSE_VAL(mm)					((mm) * 128)

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
int8_t aiMini4WdSensorsRegisterCapturedCallback(AiMini4WdSensorsCapturedCb cb);

int8_t aiMini4WdSensorsGetCurrentImuData(AiMini4WdSensorsImuData *imu_data);
int8_t aiMini4WdSensorsGetCurrentMouseData(AiMini4WdSensorsMouseData *mouse_data);

#endif /* SENSORS_H_ */