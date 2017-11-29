/*
 * Copyright (c) 2017 Quantum.inc. All rights reserved.
 * This software is released under the MIT License, see LICENSE.txt.
 */

#ifndef SYSTEM_SYSTEM_H_
#define SYSTEM_SYSTEM_H_

/*
 * Reboot and Boot cause
 */
#define BOOT_CAUSE_SW_RST_BIT	(0x80)
typedef enum BOOT_CAUSE_t{
	BOOT_CAUSE_POR			= 0x00,
	BOOT_CAUSE_UNKNOWN		= 0x40,
	BOOT_CAUSE_SW_RST		= BOOT_CAUSE_SW_RST_BIT,
	BOOT_CAUSE_USB			= BOOT_CAUSE_SW_RST_BIT | 0x01,
	BOOT_CAUSE_UPDATE_READY	= BOOT_CAUSE_SW_RST_BIT | 0x02
} BOOT_CAUSE;

void aiMini4WdRebootForSystem(uint8_t bootloader);
BOOT_CAUSE aiMini4WdGetBootCauseForSystem(void);


#endif /* SYSTEM_SYSTEM_H_ */