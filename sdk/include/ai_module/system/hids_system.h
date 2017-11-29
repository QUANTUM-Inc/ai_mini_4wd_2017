/*
 * Copyright (c) 2017 Quantum.inc. All rights reserved.
 * This software is released under the MIT License, see LICENSE.txt.
 */

#ifndef HIDS_SYSTEM_H_
#define HIDS_SYSTEM_H_

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
#define AI_MINI_4WD_LED0				(0x01)
#define AI_MINI_4WD_LED1				(0x02)
#define AI_MINI_4WD_LED2				(0x04)
#define AI_MINI_4WD_STATUS_LED			(0x08)
#define AI_MINI_4WD_CHG_LED				(0x10)

/*--------------------------------------------------------------------------*/
void aiMini4WdHidsSetSystemLed(uint8_t val);
void aiMini4WdHidsClearSystemLed(uint8_t val);
void aiMini4WdHidsToggleSystemLed(uint8_t val);

void aiMini4WdHidsUpdate(void);

#endif /* HIDS_SYSTEM_H_ */