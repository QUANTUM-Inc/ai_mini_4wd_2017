/*
 * hids.h
 *
 * Created: 2017/08/15 15:14:49
 *  Author: aks
 */ 


#ifndef HIDS_H_
#define HIDS_H_

#include <stdint.h>

/*--------------------------------------------------------------------------*/
/*J LEDŠÖŒW‚Ì’è‹`                                                            */
/*--------------------------------------------------------------------------*/
#define AI_MINI_4WD_LED0				(0x01)
#define AI_MINI_4WD_LED1				(0x02)
#define AI_MINI_4WD_LED2				(0x04)

/*--------------------------------------------------------------------------*/
void aiMini4WdHidsSetLedValue(uint8_t val);
void aiMini4WdHidsSetLed(uint8_t bitmap);
void aiMini4WdHidsClearLed(uint8_t bitmap);
void aiMini4WdHidsToggleLed(uint8_t bitmap);



/*--------------------------------------------------------------------------*/
/*J SwitchŠÖŒW‚Ì’è‹`                                                         */
/*--------------------------------------------------------------------------*/
typedef enum AiMini4WdHidsSw_t
{
	cAiMini4WdHidsSw0 = 0,
	cAiMini4WdHidsSw1 = 1,
	cAiMini4WdHidsSw2 = 2
} AiMini4WdHidsSw;

typedef enum AiMini4WdHidsSwCbType_t
{
	cAiMini4WdHidsSwCbOnPress,
	cAiMini4WdHidsSwCbOnRepeat,
	cAiMini4WdHidsSwCbOnRelease
} AiMini4WdHidsSwCbType;

typedef void (*AiMini4WdHidsSwCb)(void);

/*--------------------------------------------------------------------------*/
uint8_t aiMini4WdHidsGetSw(AiMini4WdHidsSw sw);
int8_t aiMini4WdHidsRegisterSwCallback(AiMini4WdHidsSwCbType type, AiMini4WdHidsSw sw, AiMini4WdHidsSwCb cb);


#endif /* HIDS_H_ */