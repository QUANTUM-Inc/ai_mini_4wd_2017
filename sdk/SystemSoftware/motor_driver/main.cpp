/*
 * Copyright (c) 2017 Quantum.inc. All rights reserved.
 * This software is released under the MIT License, see LICENSE.txt.
 */

#include <avr/io.h>
#include <avr/interrupt.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <utils.h>
#include <tinyX4A_timer.h>
#include <tinyX4A_i2c.h>
#include <tinyX4A_adc.h>

#ifdef __cplusplus
}
#endif

#include "motor_driver.h"

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
#define CTRL1_Hi()							(PORTB |=  (1 << PORTB0))
#define CTRL1_Lo()							(PORTB &= ~(1 << PORTB0))
#define CTRL1_Stat()						(PINB &    (1 << PORTB0))

#define PWM1_Hi()							(PORTB |=  (1 << PORTB2))
#define PWM1_Lo()							(PORTB &= ~(1 << PORTB2))
#define PWM1_Stat()							(PINB &    (1 << PORTB2))

#define CTRL2_Hi()							(PORTB |=  (1 << PORTB1))
#define CTRL2_Lo()							(PORTB &= ~(1 << PORTB1))
#define CTRL2_Stat()						(PINB &    (1 << PORTB1))

#define PWM2_Hi()							(PORTA |=  (1 << PORTA7))
#define PWM2_Lo()							(PORTA &= ~(1 << PORTA7))
#define PWM2_Stat()							(PINA &    (1 << PORTA7))

/*--------------------------------------------------------------------------*/
#define FET_CW_HI_SIDE_ON()					(CTRL1_Hi())
#define FET_CW_HI_SIDE_OFF()				(CTRL1_Lo())
#define FET_CW_LO_SIDE_ON()					(PWM1_Lo())
#define FET_CW_LO_SIDE_OFF()				(PWM1_Hi())

#define FET_CCW_HI_SIDE_ON()				(CTRL2_Hi())
#define FET_CCW_HI_SIDE_OFF()				(CTRL2_Lo())
#define FET_CCW_LO_SIDE_ON()				(PWM2_Lo())
#define FET_CCW_LO_SIDE_OFF()				(PWM2_Hi())


#define DRIVER_OFF()						{FET_CCW_HI_SIDE_OFF();	FET_CCW_LO_SIDE_OFF();	FET_CW_HI_SIDE_OFF();	FET_CW_LO_SIDE_OFF();}
#define DRIVER_BREAK()						{FET_CCW_HI_SIDE_ON();	FET_CCW_LO_SIDE_OFF();	FET_CW_HI_SIDE_ON();	FET_CW_LO_SIDE_OFF();}
#define DRIVER_SET_CW()						{FET_CCW_HI_SIDE_OFF();	FET_CCW_LO_SIDE_OFF();	FET_CW_HI_SIDE_ON();	FET_CW_LO_SIDE_OFF();}
#define DRIVER_SET_CCW()					{FET_CW_HI_SIDE_OFF();	FET_CW_LO_SIDE_OFF();	FET_CCW_HI_SIDE_ON();	FET_CCW_LO_SIDE_OFF();}



/*--------------------------------------------------------------------------*/
#define MCU_CLK								(8000000)

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
static void _initialize_gpio(void); 

static uint8_t _timer1cb(void);
static void _i2cStartCb(uint8_t addr);
static uint8_t _i2cTx(void);
static void _i2cRx(uint8_t data);


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
static uint16_t sCurrentBatteryVoltage = 0;

static uint8_t sCurrentMode        = DRIVER_MODE_STOP;
static uint8_t sCurrentPwmDuty     = 0;

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
int main(void)
{
	_initialize_gpio();

	initializeTimer1(MCU_CLK, 10, _timer1cb);
	initializeTimer0FastPwm(MCU_CLK, 0);

	i2cInitAsSlave(_i2cStartCb, _i2cTx, _i2cRx, 0x60);
	adcInit(ADC_REF_1_1V, 0x01);

	sei();

    while (1) 
    {
		uint16_t val = 0;
		adcRead(ADC_CH0, &val);
	
		uint32_t battery_mv = (((uint32_t)val * 1100) * 109) / 10240; //1.09”{‚µ‚½‚©‚Á‚½c
		
		sCurrentBatteryVoltage = (uint16_t)battery_mv;
    }
}


/*--------------------------------------------------------------------------*/
static void _initialize_gpio(void)
{
	//J ‰Šúó‘Ô‚Íˆê’USTOP
	FET_CW_HI_SIDE_OFF();	FET_CW_LO_SIDE_OFF();
	FET_CCW_HI_SIDE_OFF();	FET_CCW_LO_SIDE_OFF();
	

	DDRA = (1 << PORTA7) | (0 << PORTA6) | (0 << PORTA5) | (0 << PORTA4) | (0 << PORTA3) | (0 << PORTA2) | (0 << PORTA1) | (0 << PORTA0);
	DDRB = (0 << PORTB3) | (1 << PORTB2) | (1 << PORTB1) | (1 << PORTB0);

	return;
}

/*--------------------------------------------------------------------------*/
static uint8_t _timer1cb(void)
{
	return 0;
}

/*--------------------------------------------------------------------------*/
static uint8_t  sWritePointer = 0;
static uint8_t  sReadPointer  = 0;
static uint16_t sBatteryVoltage = 0;
static uint8_t sMode  =0;
static void _i2cStartCb(uint8_t addr)
{
	sReadPointer  = 0;
	sWritePointer = 0;
	sBatteryVoltage = sCurrentBatteryVoltage;
}

/*--------------------------------------------------------------------------*/
static uint8_t _i2cTx(void)
{
	volatile uint8_t data = 0;
	if (sReadPointer == 0) {
		data = (sBatteryVoltage >> 8) & 0xff;
	}
	else {
		data = sBatteryVoltage & 0xff;
	}

	sReadPointer++;
	
	return data;
}

/*--------------------------------------------------------------------------*/
static void _i2cRx(uint8_t data)
{
	if (sWritePointer == 0) {
		sMode = data;
	}
	else if (sWritePointer == 1) {
		if(sMode == DRIVER_MODE_STOP) {
			DRIVER_OFF();
			timer0_enableFastPwm(TIMER0_FAST_PWM_DISABLE);
			timer0_setFastPwmDuty(0);
			DRIVER_BREAK();
		}
		else if (sMode == DRIVER_MODE_CW) {
			if (sMode != sCurrentMode) {
				DRIVER_OFF();
			}

			if (sCurrentPwmDuty != data) {
				timer0_setFastPwmDuty(data);
			}

			if (sMode != sCurrentMode) {
				timer0_enableFastPwm(TIMER0_FAST_PWM_OC0A_I);
				DRIVER_SET_CW();
			}
		}
		else if (sMode == DRIVER_MODE_CCW) {
			if (sMode != sCurrentMode) {
				DRIVER_OFF();
			}

			if (sCurrentPwmDuty != data) {
				timer0_setFastPwmDuty(data);
			}

			if (sMode != sCurrentMode) {
				timer0_enableFastPwm(TIMER0_FAST_PWM_OC0B_I);
				DRIVER_SET_CCW();
			}
		}
		else if (sMode == DRIVER_MODE_FREE) {
			DRIVER_OFF();
			timer0_enableFastPwm(TIMER0_FAST_PWM_DISABLE);
			timer0_setFastPwmDuty(0);
		}

		sCurrentMode    = sMode;
		sCurrentPwmDuty = data;
	}
	else {
		
	}

	sWritePointer++;
}

