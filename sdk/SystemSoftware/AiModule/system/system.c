/*
 * Copyright (c) 2017 Quantum.inc. All rights reserved.
 * This software is released under the MIT License, see LICENSE.txt.
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <xmegaA4U_utils.h>
#include <xmegaA4U_core.h>
#include <xmegaA4U_gpio.h>
#include <xmegaA4U_timer.h>
#include <xmegaA4U_adc.h>
#include <xmegaA4U_i2c.h>
#include <xmegaA4U_uart.h>
#include <xmegaA4U_spi.h>

#include "../build_version.h"

#include <error.h>

#include <sensors.h>
#include <motor_driver.h>
#include <system.h>

#include "system/system_system.h"
#include "system/hids_system.h"
#include "system/timer_system.h"
#include "system/memory_manager_system.h"

#include "system/drivers/lsm6ds3h.h"
#include "system/drivers/adns-9800.h"
#include "system/drivers/i2c_scheduler.h"
#include "system/fs/ff.h"

#include "system/usb/mmc.h"
#include "system/usb/usb.h"
#include "system/usb/usb_mass_storage.h"

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
static void _initialize_gpio(void);
static void _lsm6ds3h_int1_cb(void);
static void _lsm6ds3h_int2_cb(void);
static void _timer0_cb(void);
static void _ext_trigger_cb(void);


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
static uint16_t sBatteryVoltage = 0;
static uint8_t sLowBatteryCount = 0;
static FATFS sDrive;
static volatile uint32_t sSystemTickMs = 0;

static uint8_t sExtTriggerIgnoreCounter = 0;
static AiMini4WdSystemExtTriggerCallback sExtTriggerCb = NULL;

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
//J 基準電圧1Vで、12ビット解像度を前提, 10:1 に分圧されたモノ
//J X : 1[V] = Val : 4095
//J X = (1[V] x Val) / 4095
//J E = 11 x X
//J E = 11 x 1[V] x Val/4095
//J E[mV] = 11 x 1000 x val/4095
#define CONVERT_TO_MILI_VOLT(val)	(uint16_t)(((uint32_t)(val)*1000*11)/4095)

#define AI_MINI_4WD_LOW_BATTERY_THRESHOLD					(3400)	// 3.4V = 3400mV


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
int8_t aiMini4WdSystemInitialize(uint32_t function_bit_map)
{
	uint8_t ret = 0;

	//------------------------------------------------------------------------
	if (!(function_bit_map & AI_SYSTEM_FUNCTION_BASE)) {
		return AI_OK;
	}

	//------------------------------------------------------------------------
	//J クロック初期化
	uint32_t systemClock = initialize_clock(CORE_SRC_RC32MHZ, NULL, NULL);

	//J GPIOの初期化
	_initialize_gpio();

	//J UARTの初期化
	ret = initialize_uart(USART0_ON_PORTC, 115200, systemClock);
	if (ret != AI_OK) {
		;
	}

	//------------------------------------------------------------------------
	//J 起動時にSW0が押された状態だった場合にはBoot Loaderモードに入れる
	//J 或いは、Boot Causeが Soft reset の場合にもBoot Loaderモードへ ?
	BOOT_CAUSE boot_cause = aiMini4WdGetBootCauseForSystem();
	if (boot_cause == BOOT_CAUSE_UPDATE_READY) {
		aiMini4WdRebootForSystem(1);
	}

	//J 起動時にSW0が押されていればBootloderモードに入る
	uint8_t sw0 = 0xFF;
	ret = gpio_input(GPIO_PORTA, GPIO_PIN4, &sw0);
	if ((ret == GPIO_OK) && (sw0 == 0)) {
		aiMini4WdRebootForSystem(1);
	}


	//J Sysmemの初期化
	(void)aiMini4WdSystemInitMemoryBlock();
#if 0
	{	
		uint16_t free_size = 0;
		uint16_t allocated_size = 0;
		aiMini4WdSystemLog("\r\nSysmem Test\r\n");

		free_size = aiMini4WdDebugSystemMemoryMap();
		aiMini4WdSystemLog("Free size = %u. Allocated size = %u. Total = %u\r\n\r\n", free_size, allocated_size, free_size + allocated_size);
		
		
		
		void *test1 = aiMini4WdAllocateMemory(500);
		allocated_size += (500 + 7) & ~7;

		void *test2 = aiMini4WdAllocateMemory(8);
		allocated_size += (8 + 7) & ~7;
		
		free_size = aiMini4WdDebugSystemMemoryMap();
		aiMini4WdSystemLog("Free size = %u. Allocated size = %u. Total = %u\r\n\r\n", free_size, allocated_size, free_size + allocated_size);

		aiMini4WdDestroyMemory(test1, 500);
		allocated_size -= (500 + 7) & ~7;

		free_size = aiMini4WdDebugSystemMemoryMap();
		aiMini4WdSystemLog("Free size = %u. Allocated size = %u. Total = %u\r\n\r\n", free_size, allocated_size, free_size + allocated_size);

		void *test3 = aiMini4WdAllocateMemory(1024);
		allocated_size += (1024 + 7) & ~7;

		free_size = aiMini4WdDebugSystemMemoryMap();
		aiMini4WdSystemLog("Free size = %u. Allocated size = %u. Total = %u\r\n\r\n", free_size, allocated_size, free_size + allocated_size);

		aiMini4WdDestroyMemory(test2, 120);
		allocated_size -= (120 + 7) & ~7;

		free_size = aiMini4WdDebugSystemMemoryMap();
		aiMini4WdSystemLog("Free size = %u. Allocated size = %u. Total = %u\r\n\r\n", free_size, allocated_size, free_size + allocated_size);

		void *test4 = aiMini4WdAllocateMemory(200);
		allocated_size += (200 + 7) & ~7;

		free_size = aiMini4WdDebugSystemMemoryMap();
		aiMini4WdSystemLog("Free size = %u. Allocated size = %u. Total = %u\r\n\r\n", free_size, allocated_size, free_size + allocated_size);

		void *test5 = aiMini4WdAllocateMemory(3584);
		allocated_size += (3584 + 7) & ~7;

		free_size = aiMini4WdDebugSystemMemoryMap();
		aiMini4WdSystemLog("Free size = %u. Allocated size = %u. Total = %u\r\n\r\n", free_size, allocated_size, free_size + allocated_size);

		aiMini4WdDestroyMemory(test5, 3584);
		allocated_size -= (3584 + 7) & ~7;

		free_size = aiMini4WdDebugSystemMemoryMap();
		aiMini4WdSystemLog("Free size = %u. Allocated size = %u. Total = %u\r\n\r\n", free_size, allocated_size, free_size + allocated_size);

	}
	while(1);
#endif

	aiMini4WdSystemLog ("\r\n\r\nAI Module Start. ");
	aiMini4WdSystemLog ("Build %08lx [%s %s] CLK = %08lx.\r\n", (uint32_t)AI_MINI_4WD_MODULE_BUILD, __DATE__, __TIME__, systemClock);

	//J USB デバイスのとして初期化する
	if (function_bit_map & AI_SYSTEM_FUNCTION_USB) {
		//J カードの有無
		uint8_t card = mmc_initialize();

		if (card != CT_UNKNOWN)  {
			//J USB接続を行うために再起動してきたケースはUSBデバイスとして頑張る
			if (boot_cause == BOOT_CAUSE_SW_RST) {
				aiMini4WdSystemLog("Activate USB Mass Storage Device\r\n");

				(void)initialize_usb(1);
				(void)usbInstallMassStorageClass();	
				Enable_Int();

				while (1) usb_poll_status();
			}
			//J そうでもない場合にはSOFを受け取ったときに再起動がかかるようにしておく
			else {
				ret = initialize_usb(0);
				usb_set_device_state(USB_DEVICE_STATE_RESET_WHEN_BUS_CONNECTED);
			}
		}
	}

	//J システム基準用タイマー(5ms)起動
	ret = initialize_tcc0_as_timerMilSec(5, systemClock);
	if (ret != AI_OK) {
		;
	}
	else {
		tcc0_timer_registerCallback(_timer0_cb);
	}
	
	//J I2C を起動
	ret = initialize_i2c_scheduler(400000, systemClock);
	if(ret != AI_OK) {
		
	}

	//J USART as SPI Master
	ret = initialize_uart_spi(USART0_ON_PORTD, 3, GPIO_PORTD, GPIO_PIN0, 2000000, systemClock);
	if (ret != UART_OK) {
		
	}

	//J Battery 監視用ADCの起動
	ADC_INIT_OPT adcOpt;
	adcOpt.vrefSelect = VREF_INTERNAL_1V;

	//J 内部1Vを基準電圧として取る
	ret = initialize_adcModule(10000, systemClock, ADC_ONESHOT, 12, &adcOpt);
	if (ret == AI_OK) {
	}


	//------------------------------------------------------------------------
	Enable_Int();

	//------------------------------------------------------------------------
	//J AiMini4Wdベースシステム初期化
	aiMini4WdHidsClearSystemLed(0xff);

	//J マウスモジュールは必要に応じて起動
	if (function_bit_map & AI_SYSTEM_FUNCTION_ADNS9800) {
		//J マウスモジュール起動
		ret = adns9800_prove();
		if (ret != AI_OK) {
			aiMini4WdSystemLog ("adns9800_prove was failed. ret = %d\r\n", ret);
		}
		else {
			uint8_t product_id = 0;
			uint8_t revision_id = 0;
			uint8_t inv_product_id = 0;
			uint8_t srom_id = 0;

			adns9800_getProductId(&product_id);
			adns9800_getInvProductId(&inv_product_id);
			adns9800_getRevisionId(&revision_id);
			adns9800_getSromId(&srom_id);
			aiMini4WdSystemLog ("ADNS9800 [PID.%02x(%02x) rev.%02x SROM.%02x]\r\n", product_id, inv_product_id, revision_id, srom_id);

			if (srom_id != ADNS9800_SROM_VERSION) {
				aiMini4WdHidsSetSystemLed(AI_MINI_4WD_LED0);
			}
		}
	}

	//J IMUの初期化
	ret = lsm6ds3h_probe();
	if (ret != AI_OK) {
		aiMini4WdSystemLog ("lsm6ds3h_probe was failed. ret = %d\r\n", ret);
	}
	else {
		aiMini4WdSystemLog ("IMU(LSM6DS3H) was initialized.\r\n");
	}

	//J 必要に応じてFSを初期化
	if (function_bit_map & AI_SYSTEM_FUNCTION_FS) {
		//J SDカードを動くようにする
		ret = f_mount(&sDrive, "", 0);
		if (ret != FR_OK) {
			aiMini4WdSystemLog ("Failed to f_mount(). ret = %d\r\n", ret);
		}
		else {
			aiMini4WdSystemLog ("File System was initialized.\r\n");
		}
	}


	//------------------------------------------------------------------------
	//J 起動準備完了
	aiMini4WdHidsSetSystemLed(AI_MINI_4WD_STATUS_LED);

	return AI_OK;
}


/*---------------------------------------------------------------------------*/
uint16_t aiMini4WdSystemGetBatteryVoltage(void)
{
	return sBatteryVoltage;
}


/*---------------------------------------------------------------------------*/
int8_t aiMini4WdSystemRegisterExtTriggerCallback(AiMini4WdSystemExtTriggerCallback cb)
{
	sExtTriggerCb = cb;
	
	return AI_OK;
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
static FIL *sSystemLog = NULL;
int8_t aiMini4WdSystemChangeLogOutput(FIL *file)
{
	sSystemLog = file;
	
	return AI_OK;
}

/*---------------------------------------------------------------------------*/
static char line[128];
void aiMini4WdSystemLog(const char *format, ...) {
	va_list ap;
	va_start(ap, format);
	vsnprintf(line, sizeof(line), format, ap);
	va_end(ap);

	if (sSystemLog != NULL) {
		UINT written = 0;
		f_write(sSystemLog, line, strlen(line), &written);
		(void)written;
	}
	else {
		uart_txNonBuffer((uint8_t *)line, strlen(line));
	}

	return;
}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
void aiMini4WdRebootForSystem(uint8_t bootloader)
{
	if (bootloader) {
		//J Boot Loader モードに移行
		asm("jmp	0x20000");
	}
	else {
		//J SW Reset
		CCP = CCP_IOREG_gc;
		RST.CTRL = 1;
		while(1);
	}
	
	//J ここには来ないはず	
	return;
}

/*---------------------------------------------------------------------------*/
BOOT_CAUSE aiMini4WdGetBootCauseForSystem(void)
{
	uint8_t reset_status = RST.STATUS;
	RST.STATUS   = 0;

	uint8_t reboot_cause = 0;

	if (reset_status & RST_SRF_bm) {
		reboot_cause = BOOT_CAUSE_SW_RST;
	}
	else if (reset_status & RST_PORF_bm){
		reboot_cause = BOOT_CAUSE_POR;		
	}
	else {
		reboot_cause = BOOT_CAUSE_UNKNOWN;
	}

	return reboot_cause;
}

/*---------------------------------------------------------------------------*/
void aiMini4WdReboot(void)
{
	//J SW Reset
	RST.CTRL = 1;
	
	//J ここには来ないはず
	return;
}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
static void _initialize_gpio(void)
{
	//J Port A
	initialize_gpio(GPIO_PORTA, GPIO_PIN0, GPIO_IN, GPIO_PUSH_PULL);	// AVCC
	initialize_gpio(GPIO_PORTA, GPIO_PIN1, GPIO_IN, GPIO_PUSH_PULL);	// NC
	initialize_gpio(GPIO_PORTA, GPIO_PIN2, GPIO_IN, GPIO_PUSH_PULL);	// NC
	initialize_gpio(GPIO_PORTA, GPIO_PIN3, GPIO_IN, GPIO_PUSH_PULL);	// NC
	initialize_gpio(GPIO_PORTA, GPIO_PIN4, GPIO_IN, GPIO_PUSH_PULL);	// SW0#
	initialize_gpio(GPIO_PORTA, GPIO_PIN5, GPIO_IN, GPIO_PUSH_PULL);	// SW1#
	initialize_gpio(GPIO_PORTA, GPIO_PIN6, GPIO_IN, GPIO_PUSH_PULL);	// SW2#
	initialize_gpio(GPIO_PORTA, GPIO_PIN7, GPIO_IN, GPIO_PUSH_PULL);	// BATT_SENSE(ADC7)
	
	//J Port B
	initialize_gpio(GPIO_PORTB, GPIO_PIN0, GPIO_IN, GPIO_PUSH_PULL);	// PULSE_IN0
	initialize_gpio(GPIO_PORTB, GPIO_PIN1, GPIO_IN, GPIO_PUSH_PULL);	// PULSE_IN1
	initialize_gpio(GPIO_PORTB, GPIO_PIN2, GPIO_IN, GPIO_PUSH_PULL);	// INT1#
	initialize_gpio(GPIO_PORTB, GPIO_PIN3, GPIO_IN, GPIO_PUSH_PULL);	// INT2#


	gpio_configeInterrupt(GPIO_PORTB, GPIO_PIN1, INT_RISING, _ext_trigger_cb);
	gpio_configeInterrupt(GPIO_PORTB, GPIO_PIN2, INT_RISING, _lsm6ds3h_int1_cb);
	gpio_configeInterrupt(GPIO_PORTB, GPIO_PIN3, INT_RISING, _lsm6ds3h_int2_cb);

	//J Port C
	gpio_output(GPIO_PORTC, GPIO_PIN0, 1);
	gpio_output(GPIO_PORTC, GPIO_PIN1, 1);

	gpio_output(GPIO_PORTC, GPIO_PIN4, 1);
	gpio_output(GPIO_PORTC, GPIO_PIN5, 1);
	gpio_output(GPIO_PORTC, GPIO_PIN7, 0);

	initialize_gpio(GPIO_PORTC, GPIO_PIN0, GPIO_OUT, GPIO_PUSH_PULL);	// LED0
	initialize_gpio(GPIO_PORTC, GPIO_PIN1, GPIO_OUT, GPIO_PUSH_PULL);	// LED1
	initialize_gpio(GPIO_PORTC, GPIO_PIN2, GPIO_IN, GPIO_PUSH_PULL);	// RX
	initialize_gpio(GPIO_PORTC, GPIO_PIN3, GPIO_OUT, GPIO_PUSH_PULL);	// TX
	initialize_gpio(GPIO_PORTC, GPIO_PIN4, GPIO_OUT, GPIO_PUSH_PULL);	// MMC_CS
	initialize_gpio(GPIO_PORTC, GPIO_PIN5, GPIO_OUT, GPIO_PUSH_PULL);	// MMC_MOSI
	initialize_gpio(GPIO_PORTC, GPIO_PIN6, GPIO_IN,  GPIO_PUSH_PULL);	// MMC_MISO
	initialize_gpio(GPIO_PORTC, GPIO_PIN7, GPIO_OUT, GPIO_PUSH_PULL);	// MMC_SCK

	PORTC.PIN6CTRL |= PORT_OPC_PULLUP_gc;


	//J Port D
	gpio_output(GPIO_PORTD, GPIO_PIN5, 1);

	initialize_gpio(GPIO_PORTD, GPIO_PIN0, GPIO_OUT, GPIO_PUSH_PULL);	// MOUSE_SS
	initialize_gpio(GPIO_PORTD, GPIO_PIN1, GPIO_OUT, GPIO_PUSH_PULL);	// MOUSE_SCK
	initialize_gpio(GPIO_PORTD, GPIO_PIN2, GPIO_IN, GPIO_PUSH_PULL);	// MOUSE_MISO
	initialize_gpio(GPIO_PORTD, GPIO_PIN3, GPIO_OUT, GPIO_PUSH_PULL);	// MOUSE_MOSI
	initialize_gpio(GPIO_PORTD, GPIO_PIN4, GPIO_IN, GPIO_PUSH_PULL);	// BATT_CHG#
	initialize_gpio(GPIO_PORTD, GPIO_PIN5, GPIO_OUT, GPIO_PUSH_PULL);	// CHG_LED#
	initialize_gpio(GPIO_PORTD, GPIO_PIN6, GPIO_IN, GPIO_PUSH_PULL);	// D-
	initialize_gpio(GPIO_PORTD, GPIO_PIN7, GPIO_IN, GPIO_PUSH_PULL);	// D+

	PORTD.PIN2CTRL |= PORT_OPC_PULLUP_gc;


	//J Port E
	gpio_output(GPIO_PORTE, GPIO_PIN2, 1);
	gpio_output(GPIO_PORTE, GPIO_PIN3, 1);

	initialize_gpio(GPIO_PORTE, GPIO_PIN0, GPIO_IN, GPIO_PUSH_PULL);	// SDA
	initialize_gpio(GPIO_PORTE, GPIO_PIN1, GPIO_IN, GPIO_PUSH_PULL);	// SCL
	initialize_gpio(GPIO_PORTE, GPIO_PIN2, GPIO_OUT, GPIO_PUSH_PULL);	// LED2
	initialize_gpio(GPIO_PORTE, GPIO_PIN3, GPIO_OUT, GPIO_PUSH_PULL);	// STATUS_LED


	return;
}



/*---------------------------------------------------------------------------*/
static void _lsm6ds3h_int1_cb(void)
{	
	lsm6ds3h_on_int1();
}


/*---------------------------------------------------------------------------*/
static void _lsm6ds3h_int2_cb(void)
{
	lsm6ds3h_on_int2();
}


/*---------------------------------------------------------------------------*/
static void _timer0_cb(void)
{
	//J システム時間を更新
	sSystemTickMs += 5;
	
	// Battery 電圧を取得
	int ret = adc_selectChannel(ADC_CH7, ADC_GAIN_1X);
	if (ret == ADC_OK) {
		sBatteryVoltage = adc_grabOneShot();
		sBatteryVoltage = CONVERT_TO_MILI_VOLT(sBatteryVoltage);
	}

	//J CHG LEDのアップデート
	uint8_t chg_pin = 0;
	(void) gpio_input(GPIO_PORTD, GPIO_PIN4, &chg_pin);
	if(chg_pin == 0) { //J 充電中
		aiMini4WdHidsSetSystemLed(AI_MINI_4WD_CHG_LED);
	}
	else { //J 非充電中
		if (sBatteryVoltage < AI_MINI_4WD_LOW_BATTERY_THRESHOLD) { //J Low Battery
			sLowBatteryCount++;
			if (sLowBatteryCount > 20) {
				sLowBatteryCount = 0;
				
				aiMini4WdHidsToggleSystemLed(AI_MINI_4WD_CHG_LED);
			}
			
		} else {
			aiMini4WdHidsClearSystemLed(AI_MINI_4WD_CHG_LED);
		}
	}
	
	//J HIDのスイッチ状態などをアップデート
	aiMini4WdHidsUpdate();
	
	//J Timerモジュールのアップデート
	aiMini4WdTimerUpdate();

	if (sExtTriggerIgnoreCounter) {
		sExtTriggerIgnoreCounter--;
	}

	return;	
}


/*---------------------------------------------------------------------------*/
static void _ext_trigger_cb(void)
{
	if (sExtTriggerIgnoreCounter == 0) {
		//J チャタ除去までやった上での処理にしたい
		if (sExtTriggerCb != NULL) {
			sExtTriggerCb();
		}
		
		sExtTriggerIgnoreCounter = 100;
	}
	
	
	return;
}
