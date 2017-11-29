/*
 * motor_driver.h
 *
 * Created: 2017/08/17 15:01:11
 *  Author: aks
 */ 


#ifndef MOTOR_DRIVER_H_
#define MOTOR_DRIVER_H_

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
typedef enum AiMini4WdMotorDriverMode_t {
	cAiMini4WdModeFoward = 0,
	cAiMini4WdModeBack   = 1,
	cAiMini4WdModeBreak  = 2,
	cAiMini4WdModeFree   = 3
} AiMini4WdMotorDriverMode;

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
int8_t aiMini4WdMotorDriverSetDuty(AiMini4WdMotorDriverMode direction, uint8_t duty);
int8_t aiMini4WdMotorDriverGetCurrentSettings(AiMini4WdMotorDriverMode *mode, uint8_t *duty);
int8_t aiMini4WdMotorDriverBreak(void);
int8_t aiMini4WdMotorDriverNeutral(void);
int8_t aiMini4WdMotorDriverGetBatteryLevel(uint16_t *battery_mv);

#endif /* MOTOR_DRIVER_H_ */