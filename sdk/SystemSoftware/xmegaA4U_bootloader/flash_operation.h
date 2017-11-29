/*
 * Copyright (c) 2017 Quantum.inc. All rights reserved.
 * This software is released under the MIT License, see LICENSE.txt.
 */

#include <avr/io.h>

#ifndef FLASH_OPERATION_H_
#define FLASH_OPERATION_H_

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

// ---------------------------------------------------------------------------
// Assembler
// ---------------------------------------------------------------------------
uint8_t  readFlashByte (uint32_t address);
uint16_t readFlashWord (uint32_t address);
void     writeFlashWord(uint16_t address, uint16_t val);
void     writeFlashApplicationPageStart(uint32_t address);

//J Buffer�̃T�C�Y��128Byte�Œ�ł�
void     loadPageToFlashPageBuffer(uint16_t bufAddr);
void     writeApplicationPageFromFlashPageBuffer(uint32_t address);
void     readFlashPage(uint8_t *data, uint32_t address);


void     eraseUserSignature(void);
void     writeUserSignature(void);
uint8_t  readUserSignature(uint16_t address);
uint8_t  readCalibrationInfo(uint16_t address);


// ---------------------------------------------------------------------------
uint8_t  writeLockBit(uint8_t lock);
uint8_t  readLockBit(void);
uint8_t  readFuseByte(uint8_t idx);

void     eraseFrashOnApplicationSection(void);

// ---------------------------------------------------------------------------
void	 waitForDoneFlashOperation(void);



// ---------------------------------------------------------------------------
// C++
// ---------------------------------------------------------------------------
void     programFlashPageWithErase(uint32_t pageAddress, uint8_t *buf, uint16_t bufSize, uint8_t doErase);

#ifdef __cplusplus
}
#endif /*__cplusplus*/

#endif /* FLASH_OPERATION_H_ */