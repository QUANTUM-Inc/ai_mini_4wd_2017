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
#include <xmegaA4U_uart.h>

#include <memory_manager.h>

#include <system/usb/mmc.h>
#include <system/usb/scsi.h>

typedef struct ScsiControlContext_t
{
	uint8_t  buf[512];
	uint32_t lba;
	uint8_t  *op_ptr;
	uint16_t byte_count;
	uint16_t block_count;
} ScsiControlContext;

//static ScsiControlContext sCtx = {{0}, 0xffffffff, NULL, 0, 0};
static ScsiControlContext *sCtx = NULL;

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
static inline uint32_t _ltob(uint32_t val)
{
	uint32_t ret = 0;
	
	ret =	((val & 0x000000ff) << 24) |
			((val & 0x0000ff00) <<  8) |
			((val & 0x00ff0000) >>  8) |
			((val & 0xff000000) >> 24);

	return ret;
}

static inline uint16_t _ltob16(uint16_t val)
{
	uint16_t ret = 0;
	
	ret =	((val & 0x00ff) <<  8) |
			((val & 0xff00) >>  8);

	return ret;
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
static int8_t _scsi_process_test_unit_ready(uint8_t *cdb, size_t cdb_len, uint8_t *buf, size_t buf_len, size_t *transfer_len);
static int8_t _scsi_process_seek(uint8_t *cdb, size_t cdb_len, uint8_t *buf, size_t buf_len, size_t *transfer_len);
static int8_t _scsi_process_start_stop_unit(uint8_t *cdb, size_t cdb_len, uint8_t *buf, size_t buf_len, size_t *transfer_len);
static int8_t _scsi_process_synchronize_cache(uint8_t *cdb, size_t cdb_len, uint8_t *buf, size_t buf_len, size_t *transfer_len);
static int8_t _scsi_process_request_sense(uint8_t *cdb, size_t cdb_len, uint8_t *buf, size_t buf_len, size_t *transfer_len);
static int8_t _scsi_process_inquiry(uint8_t *cdb, size_t cdb_len, uint8_t *buf, size_t buf_len, size_t *transfer_len);
static int8_t _scsi_process_read10(uint8_t *cdb, size_t cdb_len, uint8_t *buf, size_t buf_len, size_t *transfer_len);
static int8_t _scsi_process_read_capacity10(uint8_t *cdb, size_t cdb_len, uint8_t *buf, size_t buf_len, size_t *transfer_len);
static int8_t _scsi_process_read_capacity16(uint8_t *cdb, size_t cdb_len, uint8_t *buf, size_t buf_len, size_t *transfer_len);
static int8_t _scsi_process_write(uint8_t *cdb, size_t cdb_len, uint8_t *buf, size_t buf_len, size_t *transfer_len);
static int8_t _scsi_process_write10(uint8_t *cdb, size_t cdb_len, uint8_t *buf, size_t buf_len, size_t *transfer_len);
static int8_t _scsi_process_mode_select(uint8_t *cdb, size_t cdb_len, uint8_t *buf, size_t buf_len, size_t *transfer_len);
static int8_t _scsi_process_mode_select10(uint8_t *cdb, size_t cdb_len, uint8_t *buf, size_t buf_len, size_t *transfer_len);
static int8_t _scsi_process_mode_sense(uint8_t *cdb, size_t cdb_len, uint8_t *buf, size_t buf_len, size_t *transfer_len);
static int8_t _scsi_process_mode_sense10(uint8_t *cdb, size_t cdb_len, uint8_t *buf, size_t buf_len, size_t *transfer_len);
static int8_t _scsi_process_verify10(uint8_t *cdb, size_t cdb_len, uint8_t *buf, size_t buf_len, size_t *transfer_len);
static int8_t _scsi_process_write_and_verify10(uint8_t *cdb, size_t cdb_len, uint8_t *buf, size_t buf_len, size_t *transfer_len);
static int8_t _scsi_process_write_buffer(uint8_t *cdb, size_t cdb_len, uint8_t *buf, size_t buf_len, size_t *transfer_len);
static int8_t _scsi_process_prevent_allow_medium_removal(uint8_t *cdb, size_t cdb_len, uint8_t *buf, size_t buf_len, size_t *transfer_len);
static int8_t _scsi_process_read_format_capacities(uint8_t *cdb, size_t cdb_len, uint8_t *buf, size_t buf_len, size_t *transfer_len);

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
int8_t scsiInitialize(void)
{
	sCtx = aiMini4WdAllocateMemory(sizeof(ScsiControlContext));
	if (sCtx == NULL) {
		return SCSI_ERROR_NOMEM;
	}

	memset(sCtx, 0, sizeof(ScsiControlContext));
	sCtx->lba = 0xffffffff;

	return SCSI_OK;
}

/*---------------------------------------------------------------------------*/
int8_t scsiParseCommands(uint8_t *cdb, size_t cdb_len, uint8_t *buf, size_t buf_len, size_t *tranfer_len)
{
	int8_t ret = SCSI_OK;

	if ((cdb == NULL) || (tranfer_len == NULL) ) {
		return SCSI_ERROR_NULL;
	}

	switch (cdb[0])
	{
	case SCSI_OPCODE_INQUIRY:
		ret = _scsi_process_inquiry(cdb, cdb_len, buf, buf_len, tranfer_len);
		break;
	case SCSI_OPCODE_READ_10:
		ret = _scsi_process_read10(cdb, cdb_len, buf, buf_len, tranfer_len);
		break;
	case SCSI_OPCODE_MODE_SENSE_6:
		ret = _scsi_process_mode_sense(cdb, cdb_len, buf, buf_len, tranfer_len);
		break;
	case SCSI_OPCODE_READ_CAPACITY_10:
		ret = _scsi_process_read_capacity10(cdb, cdb_len, buf, buf_len, tranfer_len);
		break;
	case SCSI_OPCODE_READ_FORMAT_CAPACITIES:
		ret = _scsi_process_read_format_capacities(cdb, cdb_len, buf, buf_len, tranfer_len);
		break;
	case SCSI_OPCODE_WRITE_10:
		ret = _scsi_process_write10(cdb, cdb_len, buf, buf_len, tranfer_len);
		break;
	case SCSI_OPCODE_START_STOP_UNIT:
		ret = _scsi_process_start_stop_unit(cdb, cdb_len, buf, buf_len, tranfer_len);
		break;

	/* Not Implemented */
	case SCSI_OPCODE_MODE_SENSE_10:
		_scsi_process_mode_sense10(cdb, cdb_len, buf, buf_len, tranfer_len);
		break;
	case SCSI_OPCODE_TEST_UNIT_READY:
		_scsi_process_test_unit_ready(cdb, cdb_len, buf, buf_len, tranfer_len);
		break;
	case SCSI_OPCODE_SEEK_6:
		_scsi_process_seek(cdb, cdb_len, buf, buf_len, tranfer_len);
		break;
	case SCSI_OPCODE_SYNCHRONIZE_CACHE_10:
		_scsi_process_synchronize_cache(cdb, cdb_len, buf, buf_len, tranfer_len);
		break;
	case SCSI_OPCODE_REQUEST_SENSE:
		_scsi_process_request_sense(cdb, cdb_len, buf, buf_len, tranfer_len);
		break;
	case SCSI_OPCODE_READ_CAPACITY_16:
		_scsi_process_read_capacity16(cdb, cdb_len, buf, buf_len, tranfer_len);
		break;
	case SCSI_OPCODE_WRITE_6:
		_scsi_process_write(cdb, cdb_len, buf, buf_len, tranfer_len);
		break;
	case SCSI_OPCODE_MODE_SELECT_6:
		_scsi_process_mode_select(cdb, cdb_len, buf, buf_len, tranfer_len);
		break;
	case SCSI_OPCODE_MODE_SELECT_10:
		_scsi_process_mode_select10(cdb, cdb_len, buf, buf_len, tranfer_len);
		break;
	case SCSI_OPCODE_VERIFY_10:
		_scsi_process_verify10(cdb, cdb_len, buf, buf_len, tranfer_len);
		break;
	case SCSI_OPCODE_WRITE_AND_VERIFY_10:
		_scsi_process_write_and_verify10(cdb, cdb_len, buf, buf_len, tranfer_len);
		break;
	case SCSI_OPCODE_WRITE_BUFFER:
		_scsi_process_write_buffer(cdb, cdb_len, buf, buf_len, tranfer_len);
		break;
	case SCSI_OPCODE_PREVENT_ALLOW_MEDIUM_REMOVAL:
		_scsi_process_prevent_allow_medium_removal(cdb, cdb_len, buf, buf_len, tranfer_len);
		break;
	default:		
#if 0
		len = sprintf (line, "Unsupported SCSI Command = 0x%02x\r\n", cdb[0]);
		uart_txNonBuffer((uint8_t *)line, len);
#endif
		break;
	}

	return ret;
}

uint8_t scsiCheckMoreTransfer(uint8_t *cdb, size_t cdb_len)
{
	if ((cdb == NULL) || (cdb_len == 0)) {
		return 0;
	}
	
	uint8_t opcode = cdb[0];
	uint8_t ret = 0;
	switch (opcode)
	{
	case SCSI_OPCODE_INQUIRY:
	case SCSI_OPCODE_READ_10:
	case SCSI_OPCODE_MODE_SENSE_6:
	case SCSI_OPCODE_READ_CAPACITY_10:
	case SCSI_OPCODE_READ_FORMAT_CAPACITIES:
	case SCSI_OPCODE_WRITE_10:
		ret = 1;
		break;

	case SCSI_OPCODE_START_STOP_UNIT:
	case SCSI_OPCODE_PREVENT_ALLOW_MEDIUM_REMOVAL:
	case SCSI_OPCODE_TEST_UNIT_READY:
		ret = 0;
		break;


	/* Not Implemented */
	case SCSI_OPCODE_MODE_SENSE_10:
	case SCSI_OPCODE_SEEK_6:
	case SCSI_OPCODE_SYNCHRONIZE_CACHE_10:
	case SCSI_OPCODE_REQUEST_SENSE:
	case SCSI_OPCODE_READ_CAPACITY_16:
	case SCSI_OPCODE_WRITE_6:
	case SCSI_OPCODE_MODE_SELECT_6:
	case SCSI_OPCODE_MODE_SELECT_10:
	case SCSI_OPCODE_VERIFY_10:
	case SCSI_OPCODE_WRITE_AND_VERIFY_10:
	case SCSI_OPCODE_WRITE_BUFFER:
	default:
		ret = 0;
		break;
	}

	return ret;
}




/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
static int8_t _scsi_process_test_unit_ready(uint8_t *cdb, size_t cdb_len, uint8_t *buf, size_t buf_len, size_t *transfer_len)
{
	return SCSI_NOT_IMPLEMENTED;
}

/*---------------------------------------------------------------------------*/
static int8_t _scsi_process_seek(uint8_t *cdb, size_t cdb_len, uint8_t *buf, size_t buf_len, size_t *transfer_len)
{
	return SCSI_NOT_IMPLEMENTED;
}

/*---------------------------------------------------------------------------*/
static int8_t _scsi_process_start_stop_unit(uint8_t *cdb, size_t cdb_len, uint8_t *buf, size_t buf_len, size_t *transfer_len)
{
	return SCSI_OK;
}

/*---------------------------------------------------------------------------*/
static int8_t _scsi_process_synchronize_cache(uint8_t *cdb, size_t cdb_len, uint8_t *buf, size_t buf_len, size_t *transfer_len)
{
	return SCSI_OK;
}

/*---------------------------------------------------------------------------*/
static int8_t _scsi_process_request_sense(uint8_t *cdb, size_t cdb_len, uint8_t *buf, size_t buf_len, size_t *transfer_len)
{
	return SCSI_OK;
}

/*---------------------------------------------------------------------------*/
static int8_t _scsi_process_inquiry(uint8_t *cdb, size_t cdb_len, uint8_t *buf, size_t buf_len, size_t *transfer_len)
{
	if ((buf == NULL) || (buf_len == 0)) {
		return -1;
	}
	
	ScsiInquiryData *response = (ScsiInquiryData *)buf;
	memset (response, 0x00, sizeof(ScsiInquiryData));
	response->peripheralDeviceType = 0;
	response->removable.bm.rmb = 1;
	response->version = 0x00;
	response->aca_flags.bm.response_data_format = 1;
	response->aca_flags.bm.norm_aca = 0;
	response->aca_flags.bm.hi_sup = 0;
	response->additional_length = 31;
	response->sccs_flags.bm.protect = 1;
	memcpy(response->vender_identification, "Generic-", strlen("Generic-"));
	memcpy(response->product_identification, "SD/MMC(AI)", strlen("SD/MMC(AI)"));
	memcpy(response->product_revision_level, "0.01", strlen("0.01"));
	
	*transfer_len =  sizeof(ScsiInquiryData);
	
	return SCSI_OK;
}


/*---------------------------------------------------------------------------*/
static int8_t _scsi_process_read10(uint8_t *cdb, size_t cdb_len, uint8_t *buf, size_t buf_len, size_t *transfer_len)
{
	ScsiCdb10 *cdb10 = (ScsiCdb10 *)cdb;

	uint32_t lba = _ltob(cdb10->logical_block_address);
	uint16_t blocks = _ltob16(cdb10->length);

	//J 新しいブロックの読み込み
	if (sCtx->byte_count == 0) {
		sCtx->lba = lba;
		sCtx->op_ptr = sCtx->buf;

		mmc_read_block(lba + sCtx->block_count, sCtx->buf, 512);
		
		memcpy(buf, sCtx->op_ptr, buf_len);
		sCtx->op_ptr += buf_len;
		sCtx->byte_count += buf_len;
	}
	//J ブロックの残りを処理
	else {
		memcpy(buf, sCtx->op_ptr, buf_len);
		sCtx->op_ptr += buf_len;
		sCtx->byte_count += buf_len;
	}

	*transfer_len = buf_len;

	//J 1セクタ送り終わるごとに次の通信に備える
	if (sCtx->byte_count == 512) {
		sCtx->block_count++;
		sCtx->byte_count = 0;
		sCtx->op_ptr = NULL;
	}

	//J 停止条件は、block_count == blocks 
	if (sCtx->block_count == blocks) {
		sCtx->block_count = 0;
		sCtx->byte_count = 0;
		sCtx->op_ptr = NULL;

		return SCSI_OK;
	}
	else {
		return SCSI_ERROR_CONTINUE;		
	}
}

/*---------------------------------------------------------------------------*/
static int8_t _scsi_process_read_capacity10(uint8_t *cdb, size_t cdb_len, uint8_t *buf, size_t buf_len, size_t *transfer_len)
{
	if ((buf == NULL) || (buf_len == 0)) {
		return -1;
	}

	uint32_t last_lba = mmc_get_num_logical_block() - 1;

	ScsiCapacityData10 *response = (ScsiCapacityData10 *)buf;
	response->block_size = _ltob(512);
	response->last_logical_block_address = _ltob(last_lba);
		
	*transfer_len =  sizeof(ScsiCapacityData10);

	return SCSI_OK;	 
}

/*---------------------------------------------------------------------------*/
static int8_t _scsi_process_read_capacity16(uint8_t *cdb, size_t cdb_len, uint8_t *buf, size_t buf_len, size_t *transfer_len)
{
	return SCSI_OK;
}

/*---------------------------------------------------------------------------*/
static int8_t _scsi_process_write(uint8_t *cdb, size_t cdb_len, uint8_t *buf, size_t buf_len, size_t *transfer_len)
{
	return SCSI_OK;
}

/*---------------------------------------------------------------------------*/
static int8_t _scsi_process_write10(uint8_t *cdb, size_t cdb_len, uint8_t *buf, size_t buf_len, size_t *transfer_len)
{
	//J CDB は毎回同じものが来ることになってる
	ScsiCdb10 *cdb10 = (ScsiCdb10 *)cdb;

	uint32_t lba = _ltob(cdb10->logical_block_address);
	uint16_t blocks = _ltob16(cdb10->length);

	//J 新しいブロックへの書き込み
	if ((sCtx->byte_count == 0) || (sCtx->lba != lba)) {
		sCtx->lba = lba;
		sCtx->op_ptr = sCtx->buf;
	}

	//J 流れてくるバッファをとにかく読み込む
	memcpy(sCtx->op_ptr, buf, buf_len);
	sCtx->op_ptr     += buf_len;
	sCtx->byte_count += buf_len;

	//J 1ブロック分データ溜まった
	if (sCtx->byte_count == 512) {
		mmc_write_block(lba + sCtx->block_count, sCtx->buf, 512);

		sCtx->block_count++;
		sCtx->byte_count = 0;
		sCtx->op_ptr = NULL;
	}

	//J 必要なブロック処理出来たら終わる
	if (sCtx->block_count == blocks) {
		sCtx->block_count = 0;
		sCtx->byte_count = 0;
		sCtx->op_ptr = NULL;

		return SCSI_OK;
	}
	else {
		return SCSI_ERROR_CONTINUE;
	}
}

/*---------------------------------------------------------------------------*/
static int8_t _scsi_process_mode_sense(uint8_t *cdb, size_t cdb_len, uint8_t *buf, size_t buf_len, size_t *transfer_len)
{
	//J この機能本当にいる？
	memset (buf, 0x00, 8);

	*transfer_len = 8;

	return SCSI_OK; // TODO
}

/*---------------------------------------------------------------------------*/
static int8_t _scsi_process_mode_sense10(uint8_t *cdb, size_t cdb_len, uint8_t *buf, size_t buf_len, size_t *transfer_len)
{
	return SCSI_OK;
}


/*---------------------------------------------------------------------------*/
static int8_t _scsi_process_mode_select(uint8_t *cdb, size_t cdb_len, uint8_t *buf, size_t buf_len, size_t *transfer_len)
{
	return SCSI_OK;
}

/*---------------------------------------------------------------------------*/
static int8_t _scsi_process_mode_select10(uint8_t *cdb, size_t cdb_len, uint8_t *buf, size_t buf_len, size_t *transfer_len)
{
	return SCSI_OK;
}

/*---------------------------------------------------------------------------*/
static int8_t _scsi_process_verify10(uint8_t *cdb, size_t cdb_len, uint8_t *buf, size_t buf_len, size_t *transfer_len)
{
	return SCSI_OK;
}

/*---------------------------------------------------------------------------*/
static int8_t _scsi_process_write_and_verify10(uint8_t *cdb, size_t cdb_len, uint8_t *buf, size_t buf_len, size_t *transfer_len)
{
	return SCSI_OK;
}

/*---------------------------------------------------------------------------*/
static int8_t _scsi_process_write_buffer(uint8_t *cdb, size_t cdb_len, uint8_t *buf, size_t buf_len, size_t *transfer_len)
{
	return SCSI_OK;
}

/*---------------------------------------------------------------------------*/
static int8_t _scsi_process_prevent_allow_medium_removal(uint8_t *cdb, size_t cdb_len, uint8_t *buf, size_t buf_len, size_t *transfer_len)
{
	return SCSI_OK;
}

/*---------------------------------------------------------------------------*/
static int8_t _scsi_process_read_format_capacities(uint8_t *cdb, size_t cdb_len, uint8_t *buf, size_t buf_len, size_t *transfer_len)
{
	if ((buf == NULL) || (buf_len == 0)) {
		return -1;
	}

	ScsiCapacityList *response = (ScsiCapacityList *)buf;
	memset (response, 0x00, sizeof(ScsiCapacityList));
	
	uint32_t num_blocks = mmc_get_num_logical_block();

	response->capacityListHeader.capacity_list_length = 8;
	response->currentCapacityHeader.descriptor_code = SCSI_CAPACITY_DESCRIPTOR_TYPE_FORMATTED_MEDIA;
	response->currentCapacityHeader.number_of_blocks = _ltob(num_blocks);
	response->currentCapacityHeader.block_length[0] = 0x00;	// 512決め打ち
	response->currentCapacityHeader.block_length[1] = 0x02;	// 512決め打ち
	response->currentCapacityHeader.block_length[2] = 0x00;	// 512決め打ち

	*transfer_len = sizeof (ScsiCapacityList);
	
	return SCSI_OK;
}


