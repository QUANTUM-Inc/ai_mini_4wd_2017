/*
 * Copyright (c) 2017 Quantum.inc. All rights reserved.
 * This software is released under the MIT License, see LICENSE.txt.
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <xmegaA4U_gpio.h>
#include <xmegaA4U_spi.h>

#include <system/usb/mmc.h>


//J Need for original code
#define	CS_H()		(gpio_output(GPIO_PORTC, GPIO_PIN4, 1))	/* Set MMC CS "high" */
#define CS_L()		(gpio_output(GPIO_PORTC, GPIO_PIN4, 0))	/* Set MMC CS "low" */

/* MMC command*/
#define CMD0	(0)			/* GO_IDLE_STATE */
#define CMD1	(1)			/* SEND_OP_COND */
#define	ACMD41	(0x80 | 41)	/* SEND_OP_COND (SDC) */
#define CMD8	(8)			/* SEND_IF_COND */
#define CMD9	(9)			/* SEND_CSD */
#define CMD10	(10)		/* SEND_CID */
#define CMD12	(12)		/* STOP_TRANSMISSION */
#define CMD13	(13)		/* SEND_STATUS */
#define ACMD13	(0x80 | 13)	/* SD_STATUS (SDC) */
#define CMD16	(16)		/* SET_BLOCKLEN */
#define CMD17	(17)		/* READ_SINGLE_BLOCK */
#define CMD18	(18)		/* READ_MULTIPLE_BLOCK */
#define CMD23	(23)		/* SET_BLOCK_COUNT */
#define	ACMD23	(0x80 | 23)	/* SET_WR_BLK_ERASE_COUNT (SDC) */
#define CMD24	(24)		/* WRITE_BLOCK */
#define CMD25	(25)		/* WRITE_MULTIPLE_BLOCK */
#define CMD32	(32)		/* ERASE_ER_BLK_START */
#define CMD33	(33)		/* ERASE_ER_BLK_END */
#define CMD38	(38)		/* ERASE */
#define CMD55	(55)		/* APP_CMD */
#define CMD58	(58)		/* READ_OCR */


typedef struct MmcAccessContext_t {
	uint8_t block_access;
	uint8_t card_type;
} MmcAccessContext;

static MmcAccessContext sCtx;

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
static void _delay_us (uint16_t n);
static int8_t _mmc_tx(uint8_t *buf, uint16_t buflen);
static int8_t _mmc_rx(uint8_t *buf, uint16_t buflen);
static int8_t _mmc_wait_for_ready(void);
static int8_t _mmc_select(void);
static void _mmc_deselect (void);
static int8_t _mmc_send_command(uint8_t cmd, uint32_t args);
static int8_t _mmc_rx_datablock(uint8_t *buf, uint16_t byte_count);
static int8_t _mmc_tx_datablock(uint8_t *buf, uint16_t byte_count, uint8_t token);


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
static void _delay_us (uint16_t n)
{
	do {
		volatile uint8_t wait=8; //J このへんフレキシブルに
		while(--wait);

	} while (--n);

	return;
}

/*--------------------------------------------------------------------------*/
static int8_t _mmc_tx(uint8_t *buf, uint16_t buflen)
{
	uint8_t data=0;

	while (buflen) {
		data = *buf++;
		(void)spi_tx(SPI_ON_PORTC, data);
		buflen--;
	}

	return 0;
}


/*--------------------------------------------------------------------------*/
static int8_t _mmc_rx(uint8_t *buf, uint16_t buflen)
{
	while (buflen) {
		uint8_t data = spi_rx(SPI_ON_PORTC);
		*buf++ = data;
		buflen--;
	}

	return 0;
}

/*--------------------------------------------------------------------------*/
static int8_t _mmc_wait_for_ready(void)
{
	uint8_t data = 0;
	uint16_t wait = 0;
	
	for (wait = 10000 ; wait != 0 ; wait--) {
		_mmc_rx(&data, 1);

		if (data == 0xff) {
			return 0;
		}
		
		_delay_us(100);
	}
	
	return -2; //Timeout
}

/*--------------------------------------------------------------------------*/
static int8_t _mmc_select(void)
{
	uint8_t data = 0;

	CS_L();
	_mmc_rx(&data, 1);
	
	int8_t ret = _mmc_wait_for_ready();
	if (ret == 0) {
		return 0;
	}

	_mmc_deselect();
	
	return -1;
}

/*--------------------------------------------------------------------------*/
static void _mmc_deselect (void) 
{
	uint8_t data = 0;
	CS_H();

	//J Dummy Clocks
	_mmc_rx(&data, 1);

	return;
}


/*--------------------------------------------------------------------------*/
static int8_t _mmc_send_command(uint8_t cmd, uint32_t args)
{
	int8_t ret = 0;
	uint8_t data = 0;
	uint8_t buf[6];
	
	//J CMD55 を先に送る必要があるもの
	if (cmd & 0x80) {
		cmd = cmd & ~0x80;
		ret = _mmc_send_command(CMD55, 0);
		if (ret > 1) {
			return ret;
		}
	}


	//J Stop Reading 以外の場合には、Select
	if (cmd != CMD12) {
		_mmc_deselect();
		ret = _mmc_select();
		if (0 != ret) {
			return 0xff;
		}
	}

	
	//J コマンドのパッキング
	if (cmd == CMD0)		data = 0x95;
	else if (cmd == CMD8)	data = 0x87;
	else					data = 1;
	
	buf[0] = 0x40 | cmd;		// Start bit + cmd
	buf[1] = (uint8_t)(args >> 24);
	buf[2] = (uint8_t)(args >> 16);
	buf[3] = (uint8_t)(args >>  8);
	buf[4] = (uint8_t)(args >>  0);
	buf[5] = data; // CRC

	//J コマンドの送出
	_mmc_tx(buf, 6);

	//J コマンドのレスポンスを受け取る
	if (cmd == CMD12) {
		_mmc_rx(&data, 1);
	}
	
	//J 上手くいけば最上位ビットが落ちるはず
	uint8_t try = 10;
	do {
		_mmc_rx(&data, 1);
	} while (--try && (data & 0x80));

	return data;
}

/*--------------------------------------------------------------------------*/
static int8_t _mmc_rx_datablock(uint8_t *buf, uint16_t byte_count)
{
	uint8_t data[2];
	uint16_t wait = 0;
	if (buf == NULL) {
		return -6;
	}

	for (wait = 1000 ; wait != 0 ; wait--) {
		_mmc_rx(data, 1);
		if (data[0] != 0xff){
			break; // Ready for receive
		}
		_delay_us(100);
	}
	if (data[0] != 0xFE) {
		return -3; // Invalid Data Token
	}
	
	_mmc_rx(buf, byte_count);
	_mmc_rx(data, 2); // CRC

	return 0;
}

/*--------------------------------------------------------------------------*/
static int8_t _mmc_tx_datablock(uint8_t *buf, uint16_t byte_count, uint8_t token)
{
	if (byte_count != 512) {
		return -5;
	}
	if (buf == NULL) {
		return -6;
	}

	int8_t ret = 0;
	uint8_t data[2];
	
	//J 通信可能な状態を確認
	ret = _mmc_wait_for_ready();
	if (ret != 0) {
		return ret;
	}

	data[0] = token;
	_mmc_tx(data, 1);
	if (token == 0xFD) {
		return 0;
	}

	//J データブロックを送る
	_mmc_tx(buf, 512);	// Send Block
	_mmc_rx(data, 2);	// Dummy CRC

	//J Check Response
	_mmc_rx(data, 1);	// Response
	if ((data[0] & 0x1f) != 0x05) {
		return -9;
	}

	//J Wait Busy
	do {
		_mmc_rx(data, 1);	// Busy Check
	} while (data[0] == 0);

	//J Dummy Clock
	(void)_mmc_rx(data, 1);	// Dummy
	(void)_mmc_rx(data, 1);	// Dummy
	(void)_mmc_rx(data, 1);	// Dummy
	(void)_mmc_rx(data, 1);	// Dummy
	(void)_mmc_rx(data, 1);	// Dummy
	(void)_mmc_rx(data, 1);	// Dummy

	return 0;
}



/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
int8_t mmc_initialize(void)
{
	memset (&sCtx, 0x00, sizeof(sCtx));
	sCtx.card_type = CT_UNKNOWN;

	_delay_us(10000);

	CS_H();

	//J IOの設定は上側でまとめてやる
	//J XmegaはSSがLoに落ちていないときはクロック出せない...
	uint8_t cnt = 0;
	for (cnt = 10; cnt ; cnt--) {
		(void)spi_txrx_pio(SPI_ON_PORTC, 0xff);
	}

	//J この後は、SCLKを叩くときは必ずSSがLoになるはず
	int8_t ret =initialize_spi_master(SPI_ON_PORTC, SPI_TYPE_3WIRE, SPI_TRANSFER_MODE_0, SPI_DATA_ORDER_MSB_FIRST, SPI_CLK_DIV_2);
	if (ret != SPI_OK) {
		return ret;
	}

	//J IDLE State
	ret = _mmc_send_command(CMD0, 0);
	if (ret != 1) {
		goto ENSURE;
	}

	ret = _mmc_send_command(CMD8, 0x1AA);
	// SD v2
	if (ret == 1) {
		uint8_t buf[4];
		_mmc_rx(buf, sizeof(buf)); //動作電圧を取得
		if (buf[2] == 0x01 && buf[3] == 0xAA) { //2.7V駆動OK
			uint16_t wait = 0;
			for (wait = 1000 ; wait != 0 ; wait--) {
				ret = _mmc_send_command(ACMD41, (uint32_t)1 << 30);
				if (ret == 0) {
					break;
				}
				_delay_us(1000);
			}
			
			//J ACMD41が成立している？
			if (wait != 0) {
				ret = _mmc_send_command(CMD58, 0);
				if (ret == 0) {
					uint8_t buf[4];
					_mmc_rx(buf, sizeof(buf));
					
					sCtx.card_type = CT_SDv2;
					sCtx.block_access = (buf[0] & 0x40) ? 1 : 0;
				}
			}
		}
	}
	// SD v1/MMC
	else {
		uint8_t cmd = 0;
		ret = _mmc_send_command(ACMD41, 0);
		if (ret <= 1) {
			sCtx.card_type = CT_SDv1;
			cmd = ACMD41;
		}
		else {
			sCtx.card_type = CT_MMC;
			cmd = CMD1;
		}
		
		uint16_t wait = 0;
		for (wait = 1000 ; wait != 0 ; wait--) {
			ret = _mmc_send_command(cmd, 0);
			if (ret == 0) {
				break;
			}
			
			_delay_us(1000);
		}
		
		//J 初期化終了後にアクセス単位を512バイトに変更
		if ((wait == 0) || (_mmc_send_command(CMD16, 512) != 0)){
			sCtx.card_type = CT_UNKNOWN;
		}
	}

ENSURE:
	_mmc_deselect();

	return sCtx.card_type;
}


/*--------------------------------------------------------------------------*/
int8_t mmc_get_cid(MmcCid *cid)
{
	int8_t ret = 0;
	
	if (cid == NULL) {
		return -1;
	}
	
	ret = _mmc_send_command(CMD10, 0);
	if (ret != 0) {
		return ret;
	}

	ret = _mmc_rx_datablock((uint8_t *)cid, sizeof(MmcCid));

	return ret;
}


/*--------------------------------------------------------------------------*/
int8_t mmc_get_csd(MmcCsd *csd)
{
	int8_t ret = 0;
	
	if (csd == NULL) {
		return -1;
	}
	
	ret = _mmc_send_command(CMD9, 0);
	if (ret != 0) {
		return ret;
	}

	ret = _mmc_rx_datablock((uint8_t *)csd, sizeof(MmcCsd));

	return ret;
}


/*--------------------------------------------------------------------------*/
int8_t mmc_read_block(uint32_t sector, uint8_t *buf, uint16_t buflen)
{
	if (sCtx.block_access == 0) {
		sector *= 512;
	} 

	//J 力弱く1ブロックごとのアクセスに専念する = CMD17だけ対応
	int8_t ret = _mmc_send_command(CMD17, sector);
	if (ret == 0) {
		ret = _mmc_rx_datablock(buf, 512); //シングルアクセスなのでCMD12によるSTOP Transmission不要
	}
	_mmc_deselect();

	return ret;
}


/*--------------------------------------------------------------------------*/
int8_t mmc_write_block(uint32_t sector, uint8_t *buf, uint16_t buflen)
{
	if (sCtx.block_access == 0) {
		sector *= 512;
	}

	//J 力弱く1ブロックごとのアクセスに専念する = CMD24だけ対応
	int8_t ret = _mmc_send_command(CMD24, sector);
	if (ret == 0) {
		//J Access Token = 0xFE;
		_mmc_tx_datablock(buf, buflen, 0xFE); //シングルアクセスなのでCMD12によるSTOP Transmission不要
	}
	_mmc_deselect();
	
	return ret;	
}

/*--------------------------------------------------------------------------*/
uint16_t mmc_get_block_size(void)
{
	return 512;
}

/*--------------------------------------------------------------------------*/
uint32_t mmc_get_num_logical_block(void)
{
	MmcCsd csd;
	mmc_get_csd(&csd);

	uint32_t c_size = ((uint32_t)(csd.bytes[7] & 0x3f) << 16) | ((uint32_t)(csd.bytes[8]) << 8) | ((uint32_t)csd.bytes[9]);

	//J C_SIZE * 512 * 1024 がカードの容量. 1LBAあたり512Byteなので、以下の式でLBの数が出る
	return (c_size + 1)*1024;
}
