/*
 * system.h
 *
 * Created: 2017/09/05 21:34:16
 *  Author: sazae7
 */ 


#ifndef SYSTEM_H_
#define SYSTEM_H_

#include <fs.h>

/*
 * Initialize AI Module
 */
#define AI_SYSTEM_FUNCTION_BASE					(1 << 0)
#define AI_SYSTEM_FUNCTION_FS					(1 << 1)
#define AI_SYSTEM_FUNCTION_ADNS9800				(1 << 2)
#define AI_SYSTEM_FUNCTION_USB					(1 << 3)

int8_t   aiMini4WdSystemInitialize(uint32_t function_bit_map);


/*
 * Battery Management
 */
uint16_t aiMini4WdSystemGetBatteryVoltage(void);


/*
 * Ext trigger
 */
typedef void (*AiMini4WdSystemExtTriggerCallback)(void);
int8_t   aiMini4WdSystemRegisterExtTriggerCallback(AiMini4WdSystemExtTriggerCallback cb);


/*
 * System Log.
 */
int8_t aiMini4WdSystemChangeLogOutput(FIL *file);
void aiMini4WdSystemLog(const char *format, ...);


/*
 * Reboot and Boot cause
 */
void aiMini4WdReboot(void);

#endif /* SYSTEM_H_ */