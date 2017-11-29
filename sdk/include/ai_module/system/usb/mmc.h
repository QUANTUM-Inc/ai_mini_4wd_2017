/*
 * Copyright (c) 2017 Quantum.inc. All rights reserved.
 * This software is released under the MIT License, see LICENSE.txt.
 */

#ifndef MMC_H_
#define MMC_H_

// Card Type MMC
#define CT_UNKNOWN	(0x00)
#define CT_MMC		(0x01)
#define CT_SDv1		(0x02)
#define CT_SDv2		(0x04)
#define CT_SDC		(CT_SDv1|CT_SDv2)	/* SD */
#define CT_BLOCK	(0x08)				/* Block Address */


typedef union MmcCid_t{
	uint8_t bytes[16];
	struct {
		uint8_t  MID;
		uint8_t  OID[2];
		uint8_t  PNM[6];
		uint8_t  PRV;
		uint32_t PSN;
		uint8_t  MDT;
		uint8_t  CRC7;
	} _;
} MmcCid;


typedef union MmcCsd_t{
	uint8_t bytes[16];
} MmcCsd;


int8_t mmc_initialize(void);

int8_t mmc_get_cid(MmcCid *cid);
int8_t mmc_get_csd(MmcCsd *csd);

uint16_t mmc_get_bit_slice(uint8_t *bytes, uint8_t pos, uint8_t nbits);

int8_t mmc_read_block(uint32_t sector, uint8_t *buf, uint16_t buflen);
int8_t mmc_write_block(uint32_t sector, uint8_t *buf, uint16_t buflen);

uint16_t mmc_get_block_size(void);
uint32_t mmc_get_num_logical_block(void);

#endif /* MMC_H_ */