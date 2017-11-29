/*
 * Copyright (c) 2017 Quantum.inc. All rights reserved.
 * This software is released under the MIT License, see LICENSE.txt.
 */

#include <stdio.h>
#include <string.h>

#include <error.h>
#include <system.h>
#include <hids.h>
#include <sensors.h>
#include <fs.h>
#include <memory_manager.h>

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
typedef struct AksSensorRecode_t
{
	uint8_t  size;						//  1byte
	uint16_t counter;					//  2byte
	AiMini4WdSensorsData sensor_data;	// 18byte
	uint8_t padding[11];				// 11byte
} AksSensorRecode; // 32byte


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
static int _initialize_user(void);
static void _sw0_pressed(void);
static void _sensors_captured_cb(AiMini4WdSensorsData *data);

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
static volatile uint8_t sLogging = 0;
static FIL sLogFile;

static uint16_t sLogRecodeCount = 0;
static uint8_t  sWriteBank = 0;
static uint8_t  sReadBank  = 0;
static uint8_t  sWritePtr  = 0;
static volatile uint8_t  sFlushReq  = 0;

//J 1回の書き込み単位を512Byte刻みにすると効率が良いはず
//static AksSensorRecode sSensorRecodes[4][16]; // 16 x sizeof(AksSensorRecode) = 16 x 32 = 512

static AksSensorRecode **sSensorRecodes = NULL;



/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
int main(void)
{
	int ret = 0;
	
	ret = aiMini4WdSystemInitialize(AI_SYSTEM_FUNCTION_BASE | AI_SYSTEM_FUNCTION_FS | AI_SYSTEM_FUNCTION_ADNS9800);
	if (ret != AI_OK) {
		
	}
	
	ret = _initialize_user();
	if (ret != AI_OK) {
		
	}
	
	while (1) {
		if (sFlushReq) {
			aiMini4WdHidsToggleLed(AI_MINI_4WD_LED0);
			sFlushReq = 0;
			
			uint32_t len = 0;
			ret = f_write(&sLogFile, (void *)sSensorRecodes[sReadBank], sizeof(sSensorRecodes[sReadBank]), &len);
			if (ret != FR_OK) {
				;
			}
		}
	}	

	return 0;
}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
static int _initialize_user(void)
{
	int8_t ret = 0;

	//J Memory Allocate
	sSensorRecodes = (AksSensorRecode **)aiMini4WdAllocateMemory(sizeof (AksSensorRecode *) * 4);
	if (sSensorRecodes == NULL ){
		return AI_ERROR_NOBUF;
	}
	
	sSensorRecodes[0] = (AksSensorRecode *)aiMini4WdAllocateMemory(sizeof(AksSensorRecode) * 16);
	if (sSensorRecodes[0] == NULL ){
		return AI_ERROR_NOBUF;
	}

	sSensorRecodes[1] = (AksSensorRecode *)aiMini4WdAllocateMemory(sizeof(AksSensorRecode) * 16);
	if (sSensorRecodes[1] == NULL ){
		return AI_ERROR_NOBUF;
	}

	sSensorRecodes[2] = (AksSensorRecode *)aiMini4WdAllocateMemory(sizeof(AksSensorRecode) * 16);
	if (sSensorRecodes[2] == NULL ){
		return AI_ERROR_NOBUF;
	}

	sSensorRecodes[3] = (AksSensorRecode *)aiMini4WdAllocateMemory(sizeof(AksSensorRecode) * 16);
	if (sSensorRecodes[3] == NULL ){
		return AI_ERROR_NOBUF;
	}

	//J SW用CB登録
	aiMini4WdHidsRegisterSwCallback(cAiMini4WdHidsSwCbOnPress,  cAiMini4WdHidsSw0, _sw0_pressed);

	//J センサキャプチャーCB
	aiMini4WdSensorsRegisterCapturedCallback(_sensors_captured_cb);
	uint8_t i=0;
	for (i=0 ; i<16 ; ++i) {
		sSensorRecodes[0][i].size = sizeof(AksSensorRecode);
		sSensorRecodes[1][i].size = sizeof(AksSensorRecode);
		sSensorRecodes[2][i].size = sizeof(AksSensorRecode);
		sSensorRecodes[3][i].size = sizeof(AksSensorRecode);
	}

	//J Logファイルの生成 (連番で作る)
	char log_filename[16];
	for (i=0 ; i<10000 ; ++i) {
		sprintf (log_filename, "LOG_%04d.BIN", i);

		ret = f_open(&sLogFile, log_filename,  FA_WRITE | FA_CREATE_NEW);
		aiMini4WdSystemLog("Try f_open(%s). ret = %d\r\n", log_filename, ret);
		if (ret != FR_EXIST) {
			break;
		}
	}


	f_lseek(&sLogFile, 10L *1024L*1024L); //J 10MiBのファイルに拡張しておくとWriteのパフォーマンスは上がるはず
	f_lseek(&sLogFile, 0);
	f_sync(&sLogFile);

	return AI_OK;
}

/*--------------------------------------------------------------------------*/
static void _sw0_pressed(void)
{
	if (sLogging) {
	}
	else {
	}
	
	sLogging = 1-sLogging;

	return;
}

/*--------------------------------------------------------------------------*/
static void _sensors_captured_cb(AiMini4WdSensorsData *data)
{
	aiMini4WdHidsToggleLed(AI_MINI_4WD_LED2);

	if (sLogging) {
		sSensorRecodes[sWriteBank][sWritePtr].counter = sLogRecodeCount;
		memcpy(&sSensorRecodes[sWriteBank][sWritePtr].sensor_data, data, sizeof(AiMini4WdSensorsData));

		sWritePtr++;
		sLogRecodeCount++;
		if (sWritePtr >= 16)	{
			aiMini4WdHidsToggleLed(AI_MINI_4WD_LED1);

			sWritePtr = 0;
			sFlushReq = 1;
			sReadBank = sWriteBank;
			sWriteBank = (sWriteBank + 1 ) & 0x03;
		}
	}
	
	return;	
}