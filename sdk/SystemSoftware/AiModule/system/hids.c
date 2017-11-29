/*
 * Copyright (c) 2017 Quantum.inc. All rights reserved.
 * This software is released under the MIT License, see LICENSE.txt.
 */

#include <stdint.h>

#include <xmegaA4U_utils.h>
#include <xmegaA4U_gpio.h>

#include <error.h>

#include <hids.h>
#include <system/hids_system.h>

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
typedef struct AiMini4WdSwContext_t
{
	uint8_t count;
	uint8_t sw_state;

	uint8_t port;
	uint8_t pin;

	AiMini4WdHidsSwCb on_press;
	AiMini4WdHidsSwCb on_repeat;
	AiMini4WdHidsSwCb on_release;
} AiMini4WdSwContext;

static AiMini4WdSwContext sSwCtx[3] =
{
	{0, 0, GPIO_PORTA, GPIO_PIN4, NULL, NULL, NULL},
	{0, 0, GPIO_PORTA, GPIO_PIN5, NULL, NULL, NULL},
	{0, 0, GPIO_PORTA, GPIO_PIN6, NULL, NULL, NULL}
};


#define SW_DELAY_TIME						(20)
#define SW_UPDATE_INTERVAL					(5)

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void aiMini4WdHidsSetLedValue(uint8_t val)
{
	val = val & 0x07;

	if (val & AI_MINI_4WD_LED0) {
		gpio_output(GPIO_PORTC, GPIO_PIN0, 0);
	}
	else {
		gpio_output(GPIO_PORTC, GPIO_PIN0, 1);
	}
	
	if (val & AI_MINI_4WD_LED1) {
		gpio_output(GPIO_PORTC, GPIO_PIN1, 0);
	}
	else {
		gpio_output(GPIO_PORTC, GPIO_PIN1, 1);
	}

	if (val & AI_MINI_4WD_LED2) {
		gpio_output(GPIO_PORTE, GPIO_PIN2, 0);
	}
	else {
		gpio_output(GPIO_PORTE, GPIO_PIN2, 1);
	}

	return;
}

/*--------------------------------------------------------------------------*/
void aiMini4WdHidsSetLed(uint8_t bitmap)
{
	if (bitmap & AI_MINI_4WD_LED0) {
		gpio_output(GPIO_PORTC, GPIO_PIN0, 0);
	}
	
	if (bitmap & AI_MINI_4WD_LED1) {
		gpio_output(GPIO_PORTC, GPIO_PIN1, 0);
	}

	if (bitmap & AI_MINI_4WD_LED2) {
		gpio_output(GPIO_PORTE, GPIO_PIN2, 0);
	}

	return;
}

/*--------------------------------------------------------------------------*/
void aiMini4WdHidsClearLed(uint8_t bitmap)
{
	if (bitmap & AI_MINI_4WD_LED0) {
		gpio_output(GPIO_PORTC, GPIO_PIN0, 1);
	}
	
	if (bitmap & AI_MINI_4WD_LED1) {
		gpio_output(GPIO_PORTC, GPIO_PIN1, 1);
	}

	if (bitmap & AI_MINI_4WD_LED2) {
		gpio_output(GPIO_PORTE, GPIO_PIN2, 1);
	}

	return;
}

/*--------------------------------------------------------------------------*/
void aiMini4WdHidsToggleLed(uint8_t bitmap)
{
	if (bitmap & AI_MINI_4WD_LED0) {
		gpio_output_toggle(GPIO_PORTC, GPIO_PIN0);
	}
	
	if (bitmap & AI_MINI_4WD_LED1) {
		gpio_output_toggle(GPIO_PORTC, GPIO_PIN1);
	}

	if (bitmap & AI_MINI_4WD_LED2) {
		gpio_output_toggle(GPIO_PORTE, GPIO_PIN2);
	}

	return;
}


/*--------------------------------------------------------------------------*/
uint8_t aiMini4WdHidsGetSw(AiMini4WdHidsSw sw)
{
	if ((uint8_t)sw > 2) {
		return 0;
	}

	return sSwCtx[(int)sw].sw_state;
}

/*--------------------------------------------------------------------------*/
int8_t aiMini4WdHidsRegisterSwCallback(AiMini4WdHidsSwCbType type, AiMini4WdHidsSw sw, AiMini4WdHidsSwCb cb)
{
	if ((uint8_t)sw > 2) {
		return AI_ERROR_INVALID_INDEX;
	}

	if (type == cAiMini4WdHidsSwCbOnPress) {
		sSwCtx[(int)sw].on_press = cb;
	}
	else if (type == cAiMini4WdHidsSwCbOnRelease) {
		sSwCtx[(int)sw].on_release = cb;
	}
	else if (type == cAiMini4WdHidsSwCbOnRepeat) {
		sSwCtx[(int)sw].on_repeat = cb;
	}


	return 0;	
}



/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void aiMini4WdHidsSetSystemLed(uint8_t val)
{
	aiMini4WdHidsSetLed(val & 0x07);

	if (val & AI_MINI_4WD_STATUS_LED) {
		gpio_output(GPIO_PORTE, GPIO_PIN3, 0);
	}
	
	if (val & AI_MINI_4WD_CHG_LED) {
		gpio_output(GPIO_PORTD, GPIO_PIN5, 0);
	}
	
	return;	
}

/*--------------------------------------------------------------------------*/
void aiMini4WdHidsClearSystemLed(uint8_t val)
{
	aiMini4WdHidsClearLed(val & 0x07);

	if (val & AI_MINI_4WD_STATUS_LED) {
		gpio_output(GPIO_PORTE, GPIO_PIN3, 1);
	}
	
	if (val & AI_MINI_4WD_CHG_LED) {
		gpio_output(GPIO_PORTD, GPIO_PIN5, 1);
	}
	
	return;
}

/*--------------------------------------------------------------------------*/
void aiMini4WdHidsToggleSystemLed(uint8_t val)
{
	aiMini4WdHidsToggleLed(val & 0x7);	

	if (val & AI_MINI_4WD_STATUS_LED) {
		gpio_output_toggle(GPIO_PORTE, GPIO_PIN3);
	}
	
	if (val & AI_MINI_4WD_CHG_LED) {
		gpio_output_toggle(GPIO_PORTD, GPIO_PIN5);
	}
	
	return;
}

/*--------------------------------------------------------------------------*/
void aiMini4WdHidsUpdate(void)
{
	uint8_t swStat = 0;
	uint8_t i=0;
	
	for (i=0 ; i<3 ; ++i) {	
		(void)gpio_input(sSwCtx[i].port, sSwCtx[i].pin, &swStat);
		if (swStat != sSwCtx[i].sw_state) {
			if (sSwCtx[i].count < SW_DELAY_TIME) {
				sSwCtx[i].count += SW_UPDATE_INTERVAL;
			}
			else {
				sSwCtx[i].sw_state = swStat;
			
				if (sSwCtx[i].sw_state == 0) {
					if (sSwCtx[i].on_press != NULL) {
						sSwCtx[i].on_press();
					}
				}
				else {
					if (sSwCtx[i].on_release != NULL) {
						sSwCtx[i].on_release();
					}
				}
			}
		}
		
		//J SW‚ª‰Ÿ‚³‚ê‚Ä‚¢‚éŠÔ‚ÍRepeat‚ð’@‚­
		if (sSwCtx[i].sw_state == 0) {
			if (sSwCtx[i].on_repeat != NULL) {
				sSwCtx[i].on_repeat();
			}
		}
		
	}
	
	return;	
}

