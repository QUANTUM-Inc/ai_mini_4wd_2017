/*
 * Copyright (c) 2017 Quantum.inc. All rights reserved.
 * This software is released under the MIT License, see LICENSE.txt.
 */

#include <stdint.h>
#include <string.h>

#include <xmegaA4U_utils.h>
#include <xmegaA4U_uart.h>

#include <error.h>

#include <system/drivers/adns-9800.h>

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
#define ADNS9800_REG_PRODUCT_ID						(0x00)
#define ADNS9800_REG_REVISION_ID					(0x01)
#define ADNS9800_REG_MOTION							(0x02)
#define ADNS9800_REG_DELTA_X_L						(0x03)
#define ADNS9800_REG_DELTA_X_H						(0x04)
#define ADNS9800_REG_DELTA_Y_L						(0x05)
#define ADNS9800_REG_DELTA_Y_H						(0x06)
#define ADNS9800_REG_SQUAL							(0x07)
#define ADNS9800_REG_PIXEL_SUM						(0x08)
#define ADNS9800_REG_MAXIMUM_PIXEL					(0x09)
#define ADNS9800_REG_MINIMUM_PIXEL					(0x0A)
#define ADNS9800_REG_SHUTTER_LOWER					(0x0B)
#define ADNS9800_REG_SHUTTER_UPPER					(0x0C)
#define ADNS9800_REG_FRAME_PERIOD_LOWER				(0x0D)
#define ADNS9800_REG_FRAME_PERIOD_UPPER				(0x0E)
#define ADNS9800_REG_CONFIGURATION_I				(0x0F)
#define ADNS9800_REG_CONFIGURATION_II				(0x10)
#define ADNS9800_REG_FRAME_CAPTURE					(0x12)
#define ADNS9800_REG_SROM_ENABLE					(0x13)
#define ADNS9800_REG_RUN_DOWNSHIFT					(0x14)
#define ADNS9800_REG_REST1_RATE						(0x15)
#define ADNS9800_REG_REST1_DOWNSHIFT				(0x16)
#define ADNS9800_REG_REST2_RATE						(0x17)
#define ADNS9800_REG_REST2_DOWNSHIFT				(0x18)
#define ADNS9800_REG_REST3_RATE						(0x19)
#define ADNS9800_REG_FRAME_PERIOD_MAX_BOUND_LOWER	(0x1A)
#define ADNS9800_REG_FRAME_PERIOD_MAX_BOUND_UPPER	(0x1B)
#define ADNS9800_REG_FRAME_PERIOD_MIN_BOUND_LOWER	(0x1C)
#define ADNS9800_REG_FRAME_PERIOD_MIN_BOUND_UPPER	(0x1D)
#define ADNS9800_REG_SHUTTER_MAX_BOUND_LOWER		(0x1E)
#define ADNS9800_REG_SHUTTER_MAX_BOUND_UPPER		(0x1F)
#define ADNS9800_REG_LASER_CTRL0					(0x20)
#define ADNS9800_REG_OBSERVATION					(0x24)
#define ADNS9800_REG_DATA_OUT_LOWER					(0x25)
#define ADNS9800_REG_DATA_OUT_UPPER					(0x26)
#define ADNS9800_REG_SROM_ID						(0x2A)
#define ADNS9800_REG_LIFT_DETECTION_THR				(0x2E)
#define ADNS9800_REG_CONFIGURATION_V				(0x2f)
#define ADNS9800_REG_CONFIGURATION_IV				(0x39)
#define ADNS9800_REG_POWER_UP_RESET					(0x3A)
#define ADNS9800_REG_SHUTDOWN						(0x3B)
#define ADNS9800_REG_INVERSE_PRODUCT_ID				(0x3F)
#define ADNS9800_REG_SNAP_ANGLE						(0x42)
#define ADNS9800_REG_MOTION_BURST					(0x50)
#define ADNS9800_REG_SROM_LOAD_BURST				(0x62)
#define ADNS9800_REG_PIXEL_BURST					(0x64)

#define ADNS9800_REG_READ							(0x00)
#define ADNS9800_REG_WRITE							(0x80)


static void _delay(uint8_t ms);
static uint8_t _adns9800_readReg(uint8_t reg, uint8_t *dat);
static uint8_t _adns9800_writeReg(uint8_t reg, uint8_t dat);

/*---------------------------------------------------------------------------*/
#define FIRMWARE_LENGTH  (3070 + 1)
extern const  uint8_t adns9800_firmware[] PROGMEM;

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
int8_t adns9800_prove(void)
{
	//J 起動方法(データシートより)
	// 1. Apply power to VDD5/VDD3 and VDDIO in any order
	// 2. Drive NCS high, and then low to reset the SPI port.
	// 3. Write 0x5a to Power_Up_Reset register (address 0x3a).
	// 4. Wait for at least 50ms time.
	// 5. Read from registers 0x02, 0x03, 0x04, 0x05 and 0x06  one time regardless of the motion pin state.
	// 6. SROM download.
	// 7. Enable laser by setting Forced_Disable bit (Bit-7) of LASER_CTRL0 register (address 0x20) to 0
	
	//J Power Up Reset レジスタにマジックコードを書く
	_adns9800_writeReg(ADNS9800_REG_POWER_UP_RESET, 0x5A);

	//J 50msほど待つ
	_delay(50);

	//J 0x02 - 0x06 のデータを読む
	uint8_t dummy = 0;
	_adns9800_readReg(ADNS9800_REG_MOTION, &dummy);
	_adns9800_readReg(ADNS9800_REG_DELTA_X_L, &dummy);
	_adns9800_readReg(ADNS9800_REG_DELTA_X_H, &dummy);
	_adns9800_readReg(ADNS9800_REG_DELTA_Y_L, &dummy);
	_adns9800_readReg(ADNS9800_REG_DELTA_Y_H, &dummy);

	//J SROM情報を送りつける
	// 1. Select the 3 K bytes SROM size at Configuration_IV register, address 0x39
	// 2. Write 0x1d to SROM_Enable register for initializing
	// 3. Wait for one frame
	// 4. Write 0x18 to SROM_Enable register again to start SROM downloading
	// 5. Write SROM file into SROM_Load_Burst register, 1st data must start with
	//    SROM_Load_Burst register address. All the
	//    SROM data must be downloaded before SROM start running.

	_adns9800_writeReg(ADNS9800_REG_CONFIGURATION_IV, 0x02);
	_adns9800_writeReg(ADNS9800_REG_SROM_ENABLE, 0x1D);		//J マジックナンバー？
	_delay(10);
	_adns9800_writeReg(ADNS9800_REG_SROM_ENABLE, 0x18);		//J マジックナンバー？

	//J SROMを打ち込む
	uint16_t srom = (uint16_t)adns9800_firmware;
	uint32_t i=0;

	uart_spi_ss(0);
	
	for (i=0 ; i<FIRMWARE_LENGTH ; ++i, srom++) {
		uint8_t dat = pgm_read_byte(srom);
		uart_spi_txrx_wo_ss(&dat, 1);
		_delay(1);
	}

	uart_spi_ss(1);

	_delay(200);

#if 0 //J 標準以外の設定だとうまく動かない問題が解決しない
	//J Fixed Frame Rateモードで動かす
	uint8_t configureation_2 = 0x08;
	_adns9800_writeReg(ADNS9800_REG_CONFIGURATION_II, configureation_2);
	_delay(200);

	uint16_t frame_period_max_bound = 0x1770; //J 8333Hz
//	frame_period_max_bound = 0x2EE0; // 4166Hz
//	frame_period_max_bound = 0x5dc0; // 2083Hz
//	frame_period_max_bound = 0x3A9E;
	uint16_t frame_period_min_bound = 0x0fa0;
	uint16_t shutter_max_bound      = frame_period_max_bound - frame_period_min_bound;

	_adns9800_writeReg(ADNS9800_REG_FRAME_PERIOD_MAX_BOUND_LOWER, (frame_period_max_bound >> 0) & 0xff);
	_adns9800_writeReg(ADNS9800_REG_FRAME_PERIOD_MAX_BOUND_UPPER, (frame_period_max_bound >> 8) & 0xff);

	_adns9800_writeReg(ADNS9800_REG_FRAME_PERIOD_MIN_BOUND_LOWER, (frame_period_min_bound >> 0) & 0xff);
	_adns9800_writeReg(ADNS9800_REG_FRAME_PERIOD_MIN_BOUND_UPPER, (frame_period_min_bound >> 8) & 0xff);

	_adns9800_writeReg(ADNS9800_REG_SHUTTER_MAX_BOUND_LOWER, (shutter_max_bound >> 0) & 0xff);
	_adns9800_writeReg(ADNS9800_REG_SHUTTER_MAX_BOUND_UPPER, (shutter_max_bound >> 8) & 0xff);

	_delay(200);

	//J CPI(Click/Inchi)の変更1800cpiを前提とするならいじる必要なし
	_adns9800_writeReg(ADNS9800_REG_CONFIGURATION_I, 0x9); // 1800cpi
	_delay(200);
#endif

	//J レーザーレジスタの内容を変更
	uint8_t laserCtrl0 = 0;
	_adns9800_readReg(ADNS9800_REG_LASER_CTRL0, &laserCtrl0);
	laserCtrl0 = laserCtrl0 & 0xF0;
	_adns9800_writeReg(ADNS9800_REG_LASER_CTRL0, laserCtrl0);

	return 0;
}


/*---------------------------------------------------------------------------*/
int8_t adns9800_getValue(Adn9800MotionData *motion_data)
{
	uint8_t cmd = ADNS9800_REG_MOTION_BURST;
	
	memset (motion_data, 0x00, sizeof(Adn9800MotionData));
	
	//J MOTION から Frame Period Lowerまで一気に読む？（14Byte)
	uart_spi_ss(0);

	uint8_t ret =uart_spi_txrx_wo_ss(&cmd, 1);
	if (ret != UART_OK) {
		goto ENSURE;
	}

	volatile uint8_t wait = 10;
	while (wait--);

	//J Motionだけ読みだして、Motionビットが立っていればRead、でなければ無視する
	ret =uart_spi_txrx_wo_ss((uint8_t *)&(motion_data->motion), 1);
	if (ret != UART_OK) {
		goto ENSURE;
	}
	else if ((motion_data->motion & 0x80) == 0) {
		goto ENSURE;
	}

	ret =uart_spi_txrx_wo_ss((uint8_t *)&(motion_data->observation), sizeof(Adn9800MotionData) - 1);
	if (ret != UART_OK) {
		goto ENSURE;
	}

ENSURE:
	uart_spi_ss(1);

	return 0;
}


/*---------------------------------------------------------------------------*/
int8_t adns9800_getProductId(uint8_t *product_id)
{
	if (product_id == NULL) {
		return AI_ERROR_NULL;
	}
	
	return (int8_t)_adns9800_readReg(ADNS9800_REG_PRODUCT_ID, product_id);
}

/*---------------------------------------------------------------------------*/
int8_t adns9800_getInvProductId(uint8_t *inv_product_id)
{
	if (inv_product_id == NULL) {
		return AI_ERROR_NULL;
	}
	
	return (int8_t)_adns9800_readReg(ADNS9800_REG_INVERSE_PRODUCT_ID, inv_product_id);
}

/*---------------------------------------------------------------------------*/
int8_t adns9800_getRevisionId(uint8_t *revision_id)
{
	if (revision_id == NULL) {
		return AI_ERROR_NULL;
	}
	
	return (int8_t)_adns9800_readReg(ADNS9800_REG_REVISION_ID, revision_id);
}

/*---------------------------------------------------------------------------*/
int8_t adns9800_getSromId(uint8_t *srom_id)
{
	if (srom_id == NULL) {
		return AI_ERROR_NULL;
	}
	
	return (int8_t)_adns9800_readReg(ADNS9800_REG_SROM_ID, srom_id);
}



/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
static uint8_t _adns9800_writeReg(uint8_t reg, uint8_t dat)
{
	uint8_t cmd[2];
	cmd[0] = reg | ADNS9800_REG_WRITE;
	cmd[1] = dat;

	uint8_t ret = uart_spi_txrx(cmd, 2);
	if (ret != UART_OK) {
		return ret;
	}

	return ret;
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
static uint8_t _adns9800_readReg(uint8_t reg, uint8_t *dat)
{
	if (dat == NULL) {
		return -1;
	}
	
	uint8_t cmd[2];
	cmd[0] = reg | ADNS9800_REG_READ;
	cmd[1] = 0xFF;

	uart_spi_ss(0);

	uint8_t ret =uart_spi_txrx_wo_ss(&cmd[0], 1);
	if (ret != UART_OK) {
		return ret;
	}

	volatile uint8_t wait = 255;
	while (wait--);

	ret =uart_spi_txrx_wo_ss(&cmd[1], 1);
	if (ret != UART_OK) {
		return ret;
	}

	uart_spi_ss(1);

	*dat = cmd[1];
	
	return ret;
}

/*---------------------------------------------------------------------------*/
static void _delay(uint8_t ms)
{
	volatile uint8_t _ms = (volatile uint8_t)ms;
	
	//J System Clockが32MHzであると仮定すると、1ms待つにはNOPを32x1000回実行すればOKなはず
	while (_ms) {
		volatile uint16_t wait = 3200; //J多分この1/4でいい
		while (wait--);
		
		_ms--;
	}
	return;
}