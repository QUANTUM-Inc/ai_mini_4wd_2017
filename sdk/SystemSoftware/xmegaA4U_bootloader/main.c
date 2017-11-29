/*
 * Copyright (c) 2017 Quantum.inc. All rights reserved.
 * This software is released under the MIT License, see LICENSE.txt.
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <avr/io.h>

#include "fs/ff.h"
#include "error.h"
#include "flash_operation.h"

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
typedef struct HexLineData_t {
	uint8_t  size;
	uint16_t addr;
	uint8_t  type;
	uint8_t  data[16];
} HexLineData;

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
#define HEX_RECODE_TYPE_DATA						(0x00)
#define HEX_RECODE_TYPE_EOF							(0x01)
#define HEX_RECODE_TYPE_EXT_SEGMENT_ADDRESS			(0x02)
#define HEX_RECODE_TYPE_START_SEGMENT_ADDRESS		(0x03)
#define HEX_RECODE_TYPE_EXT_LINER_ADDRESS			(0x04)
#define HEX_RECODE_TYPE_START_LINERE_ADDRESS		(0x05)

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
static int8_t _parseHexFileLine(const char *line, HexLineData *data);
static int8_t _char2hex(const char c, uint8_t *hex);
static int8_t _str2hex8bit(const char *line,  uint8_t *data);
static int8_t _str2hex16bit(const char *line, uint16_t *data);


#define LED0_ON()						(PORTC.OUTCLR = (1 << 0))
#define LED0_OFF()						(PORTC.OUTSET = (1 << 0))
#define LED1_ON()						(PORTC.OUTCLR = (1 << 1))
#define LED1_OFF()						(PORTC.OUTSET = (1 << 1))
#define LED2_ON()						(PORTE.OUTCLR = (1 << 2))
#define LED2_OFF()						(PORTE.OUTSET = (1 << 2))
#define LED_STATUS_ON()					(PORTE.OUTCLR = (1 << 3))
#define LED_STATUS_OFF()				(PORTE.OUTSET = (1 << 3))

#ifdef DEBUG
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
static void sendByte(uint8_t dat)
{
	while ( !(USARTC0.STATUS & USART_DREIF_bm) );
	
	USARTC0.DATA = dat;
	
	return;
}

/*---------------------------------------------------------------------------*/
#define UART_BSEL_VALUE				(1047)
#define UART_BSCALE_VALUE			(-6)

/*---------------------------------------------------------------------------*/
static void _init_uart(void)
{
	/*J 20.18.7 BAUDCTRLA ? Baud Rate Control register A */
	/*J 20.18.8 BAUDCTRLB ? Baud Rate Control register B */
	USARTC0.BAUDCTRLA = (uint8_t)(UART_BSEL_VALUE & 0xff);
	USARTC0.BAUDCTRLB = (uint8_t)(((uint8_t)UART_BSCALE_VALUE << 4) & 0xff) | (uint8_t)((uint8_t)(UART_BSEL_VALUE>>8) & 0x0f);

	USARTC0.BAUDCTRLA = 0x2E;
	USARTC0.BAUDCTRLB = 0x98;;


	/* 20.18.3 CTRLA ? Control register A */
	USARTC0.CTRLA = USART_RXCINTLVL_OFF_gc | USART_TXCINTLVL_OFF_gc | USART_DREINTLVL_OFF_gc;

	/* 20.18.4 CTRLB ? Control register B */
	USARTC0.CTRLB = USART_RXEN_bm | USART_TXEN_bm;

	/* 20.18.5 CTRLC ? Control register C */
	USARTC0.CTRLC = USART_CMODE_ASYNCHRONOUS_gc | USART_PMODE_DISABLED_gc | USART_CHSIZE_8BIT_gc;
	
	//J Port Setting
	PORTC.DIRSET = (1 << 3);	//J TXはピン3
	PORTC.DIRCLR = (1 << 2);	//J RXはピン2
	
	return;
}
#endif /*DEBUG*/

#define FLASH_PAGE_BUFFER_SIZE					(256)

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
FATFS fs;
FIL   file;

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
int main(void)
{
	int8_t ret = AI_OK;

	LED0_OFF(); LED1_OFF(); LED2_OFF();
	PORTC.DIRSET = (1 << 0) | (1 << 1);
	PORTE.DIRSET = (1 << 2) | (1 << 3);
	
	PORTA.DIRCLR = (1 << 4); // SW0
	
	//------------------------------------------------------------------------
	//J クロック設定
	OSC.CTRL |= OSC_RC32MEN_bm; // turn on 32 MHz oscillator
	while (!(OSC.STATUS & OSC_RC32MRDY_bm)) { }; // wait for it to start
	CCP = CCP_IOREG_gc;
	CLK.CTRL = CLK_SCLKSEL_RC32M_gc;

#ifdef DEBUG
	_init_uart();	
#endif
	//------------------------------------------------------------------------
	ret = f_mount(&fs, "", 0);
	if (ret != FR_OK) {
		goto ENSURE;
	}

	ret = f_open(&file, "MINI4WD.AUP", FA_READ);
	if (ret != FR_OK) {
		goto ENSURE;
	}
	
	//J Application Areaを消す
#ifndef DEBUG
	eraseFrashOnApplicationSection();
	waitForDoneFlashOperation();
#endif

	LED0_ON();

	uint8_t pos = 0;
	char line[128];

	HexLineData hexdata;

	uint32_t base = 0;
	uint8_t i=0;
	uint16_t page_buffer_index = 0;
    while (1) {
		UINT rlen = 0;
		char c;
		ret = f_read(&file, &c, 1, &rlen);
		if ((ret != FR_OK) || (rlen == 0)) {
			break;
		}

		if ('\r' == c) {
			;
		}
		else if ('\n' == c) {
			line[pos++] = '\0';
			pos = 0;
			ret = _parseHexFileLine(line, &hexdata);
			if (ret == AI_OK) {
				switch (hexdata.type) {
				case HEX_RECODE_TYPE_DATA:
					for (i=0 ; i<hexdata.size ; i+=2 ) {
#ifdef DEBUG
						sendByte(hexdata.data[i+0]);
						sendByte(hexdata.data[i+1]);
#endif

#ifndef DEBUG
						uint16_t flash_word = ((uint16_t)(hexdata.data[i+1])) <<8 | hexdata.data[i];
						writeFlashWord(page_buffer_index << 1, flash_word);
						page_buffer_index++;
						waitForDoneFlashOperation();
#endif
						if (page_buffer_index == FLASH_PAGE_BUFFER_SIZE/sizeof(uint16_t)) {
							page_buffer_index = 0;
#ifndef DEBUG
							writeFlashApplicationPageStart(base);
							waitForDoneFlashOperation();
#endif
							base += FLASH_PAGE_BUFFER_SIZE;
						}
					}

					break;
				case HEX_RECODE_TYPE_EOF:
					LED0_ON();
					LED1_ON();

#ifndef DEBUG
					writeFlashApplicationPageStart(base);
					waitForDoneFlashOperation();
#endif
					goto ENSURE;
					break;
				case HEX_RECODE_TYPE_EXT_SEGMENT_ADDRESS:
					base = (((uint32_t)hexdata.data[0]) << 16) | (((uint32_t)hexdata.data[1]) << 8) ;
					break;
				default:
					break;
				}
				
				
			}
			
		}
		else {
			line[pos++] = c;
		}

		
#ifdef DEBUG
//		sendByte(c);
#endif
    }

ENSURE:	

	LED2_ON();

	while ((PORTA.IN & (1 << 4)) == 0); //SW0が押下されている間はBootloaderにとどまる

	asm ("jmp	0x00000");
	
	return 0;
}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
//J Hex ファイルのフォーマットをパースする
static int8_t _parseHexFileLine(const char *line, HexLineData *data)
{
	if (line[0] != ':') {
		return AI_ERROR_INVALID_HEX_FORMAT;
	}
	line++;

	int8_t ret = _str2hex8bit(line, &data->size);
	if (ret != AI_OK) {
		return ret;
	}
	line += 2;

	ret = _str2hex16bit(line, &data->addr);
	if (ret != AI_OK) {
		return ret;
	}
	line += 4;

	ret = _str2hex8bit(line, &data->type);
	if (ret != AI_OK) {
		return ret;
	}
	line += 2;
	
	uint8_t i=0;
	for (i=0 ; i<data->size ; ++i) {
		ret = _str2hex8bit(line, &(data->data[i]) );
		if (ret != AI_OK) {
			return ret;
		}
		line += 2;
	}

	//J Checksumは気にしない

	return AI_OK;	
}


/*---------------------------------------------------------------------------*/
static int8_t _str2hex8bit(const char *line,  uint8_t *data)
{
	if ( (line == NULL) || (data == NULL) ) {
		return AI_ERROR_NULL;
	}

	*data = 0;
	int8_t i=0;
	for (i=0 ; i<2 ; ++i) {
		uint8_t hex = 0;
		int8_t ret = _char2hex(line[i], &hex);
		if (ret != AI_OK) {
			return ret;
		}
		*data = (*data << 4) | hex;
	}
	
	return AI_OK;
}



/*---------------------------------------------------------------------------*/
static int8_t _str2hex16bit(const char *line, uint16_t *data)
{
	if ( (line == NULL) || (data == NULL) ) {
		return AI_ERROR_NULL;
	}

	*data = 0;
	int8_t i=0;
	for (i=0 ; i<4 ; ++i) {
		uint8_t hex = 0;
		int8_t ret = _char2hex(line[i], &hex);
		if (ret != AI_OK) {
			return ret;
		}
		*data = (*data << 4) | (uint16_t)hex;
	}

	return AI_OK;
}


/*---------------------------------------------------------------------------*/
static int8_t _char2hex(const char c, uint8_t *hex)
{
	if (('0' <= c) && (c <= '9')) {
		*hex = c - '0' + 0x00;
		return AI_OK;
	}
	else if (('a' <= c) && (c <= 'f')) {
		*hex = c - 'a' + 0x0a;
		return AI_OK;
	}
	else if (('A' <= c) && (c <= 'F')) {
		*hex = c - 'A' + 0x0a;
		return AI_OK;
	}

	return AI_ERROR_INVALID;	
}
