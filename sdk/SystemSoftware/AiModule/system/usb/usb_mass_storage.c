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

#include <memory_manager.h>

#include <system/usb/usb.h>
#include <system/usb/scsi.h>

typedef struct UsbMassStorageDebugContext_t
{
	uint8_t num_cbw;

	uint8_t interrupt_in;
	uint8_t interrupt_out;

	uint8_t state_trace[16];
	uint8_t state_wptr;
	uint8_t state_rptr;
} UsbMassStorageDebugContext;

static UsbMassStorageDebugContext *sDebug = NULL;

/*--------------------------------------------------------------------------*/
// USB Mass Storage Class
/*--------------------------------------------------------------------------*/
#define WORD2BYTE(w)	((w>>0)&0xff), ((w>>8)&0xff)

uint8_t *sUsbMassStorageDeviceDescriptor = NULL;
const uint8_t cUsbMassStorageClassDescriptor[] PROGMEM // Program Memory の中に格納する
=
{
	//---------------------------------------------------------------------------
	// 0. Device Descriptor
	0x12,				// bLength
	0x01,				// bDescriptor Type = DEVICE(0x01)
	WORD2BYTE(0x200),	// bcdUSB
	0x00,				// bDeviceClass = Unknown(0x00)
	0x00,				// bDeviceSubClass = Unknown(0x00)
	0x00,				// bDeviceProtocol = Unknown(0x00)
	0x40,				// bMaxPacketSize0 = 64byte
	WORD2BYTE(0x0022),	// idVender
	WORD2BYTE(0x0033),	// idProduct
	WORD2BYTE(0x1984),	// bcdDevice
	0x00,				// iManufacturer
	0x00,				// iProduct
	0x00,				// iSerialNumber
	0x01,				// bNumConfigurations = 1
	//---------------------------------------------------------------------------
	// 1-1. String Descriptor - LANGID
	4,					// bLength = 4
	0x03,				// bDescriptorType = STRING(0x03)
	WORD2BYTE(0x0409),	// LANGID = English US(0x0409)
	//---------------------------------------------------------------------------
	// 1-2. String Descriptor - Manufacturer
	26,					// bLength = 26
	0x03,				// bDescriptorType = STRING(0x03)
	WORD2BYTE('Q'),
	WORD2BYTE('U'),
	WORD2BYTE('A'),
	WORD2BYTE('N'),
	WORD2BYTE('T'),
	WORD2BYTE('U'),
	WORD2BYTE('M'),
	WORD2BYTE(' '),
	WORD2BYTE('I'),
	WORD2BYTE('n'),
	WORD2BYTE('c'),
	WORD2BYTE('.'),
	//---------------------------------------------------------------------------
	// 1-3. String Descriptor - Product
	24,					// bLength = 24
	0x03,				// bDescriptorType = STRING(0x03)
	WORD2BYTE('A'),
	WORD2BYTE('I'),
	WORD2BYTE('-'),
	WORD2BYTE('U'),
	WORD2BYTE('n'),
	WORD2BYTE('i'),
	WORD2BYTE('t'),
	WORD2BYTE('2'),
	WORD2BYTE('0'),
	WORD2BYTE('1'),
	WORD2BYTE('7'),
	//---------------------------------------------------------------------------
	// 1-4. String Descriptor - Serial Number
	26,					// bLength
	0x03,				// bDescriptorType = STRING(0x03)
	WORD2BYTE('2'),
	WORD2BYTE('0'),
	WORD2BYTE('1'),
	WORD2BYTE('7'),
	WORD2BYTE('1'),
	WORD2BYTE('1'),
	WORD2BYTE('0'),
	WORD2BYTE('1'),
	WORD2BYTE('Q'),
	WORD2BYTE('0'),
	WORD2BYTE('0'),
	WORD2BYTE('0'),
	//---------------------------------------------------------------------------
	// 1. Configuration Descriptor
	0x09,				// bLength
	0x02,				// bDescriptorType = CONFIGURATION(0x02)
	WORD2BYTE(0x0020),	// wTotalLength = 32
	0x01,				// bNumInterfaces = 1
	0x01,				// bConfigurationValue = 1
	0x00,				// iConfiguration = ?
	0xA0,				// bmAttributes = 0xA0(Remote Wakeup)
	0x32,				// MaxPower = 50 (= 100mA)
	//---------------------------------------------------------------------------
	// 2. Interface Descriptor
	0x09,				// bLength
	0x04,				// bDescriptorType = INTERFACE(0x04)
	0x00,				// bInterfaceNumber = 0x00
	0x00,				// bAlternateSetting = 0x00
	0x02,				// bNumEndpoints = 2(IN/OUT)
	0x08,				// bInterfaceClass = MASS STORAGE Class(0x08)
	0x06,				// bInterfaceSubClass = SCSI transparent command set(0x06)
	0x50,				// bInterfaceProtocol = BULK-ONLY TRANSPORT
	0x00,				// iInterface
	//---------------------------------------------------------------------------
	// 3. Bulk-in Endpoint
	0x07,				// bLength = 0x07
	0x05,				// bDescriptorType = ENDPOINT(0x05)
	0x81,				// bEndpointAddress = EP1/IN (0x01 | 0x80)
	0x02,				// bmAttributes = Bulk endpoint(0x02)
	WORD2BYTE(0x0040),	// wMaxPacketSize = 64bytes
	0x00,				// bInterval = 0x00 Do not apply to Bulk endpoint
	//---------------------------------------------------------------------------
	// 4. Bulk-out Endpoint
	0x07,				// bLength = 0x07
	0x05,				// bDescriptorType = ENDPOINT(0x05)
	0x01,				// bEndpointAddress = EP1/OUT (0x01 | 0x00)
	0x02,				// bmAttributes = Bulk endpoint(0x02)
	WORD2BYTE(0x0040),	// wMaxPacketSize = 64bytes
	0x00,				// bInterval = 0x00 Do not apply to Bulk endpoint
	//---------------------------------------------------------------------------
	// 5. Device Qualifier Descriptor
	0x0A,				// bLength = 10
	0x06,				// bDescriptorType = DeviceQualifier(0x06)
	WORD2BYTE(0x200),	// bcdUSB
	0x00,				// bDeviceClass
	0x00,				// bDeviceSubClass
	0x00,				// bDeviceProtocol
	0x40,				// bMaxPacketSize0
	0x01,				// bNumConfigurations
	0x00				// Reserved
};


#define MASS_STORAGE_CLASS_DESCRIPTOR_SIZE					(140)


// CBW
typedef struct CommandBlockWrapper_t
{
	uint32_t dCBWsignature;
	uint32_t dCBWTag;
	uint32_t dCBWDataTransferLength;
	union {
		uint8_t byte;
		struct {
			uint8_t reserved  : 6;
			uint8_t obsolete  : 1;
			uint8_t direction : 1;
		}bm;
	} bmCBWFlags;
	uint8_t  bCBWLNU;
	uint8_t  bCBWCBLength;
	uint8_t  CBWB[16];
} CBW;

// CSW
typedef struct CommandStatusWrapper_t
{
	uint32_t dCSWSignature;
	uint32_t dCSWTag;
	uint32_t dCSWDataResidue;
	uint8_t  bCSWStatus;
} CSW;


#define CBW_SIGNATURE						(0x43425355)
#define CSW_SIGNATURE						(0x53425355)

#define CB_STATUS_PASSED					(0x00)
#define CB_STATUS_FAILED					(0x01)
#define CB_STATUS_PHASE_ERROR				(0x02)

static volatile uint8_t sStatusStage = 0;

static volatile uint8_t sCbdFlag = 0;

//J SCSIコマンドを受けて処理するためのバッファ
//static uint8_t sCommonBuf[64];
//static CBW sCbw;

static uint8_t *sCommonBuf = NULL;
static CBW *sCbw = NULL;


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
static void _usb_mass_storage_control_transfer_cb(UsbDeviceRequest *req);
static void _usb_mass_storage_bulk_only_in (uint8_t ep_num, UsbEpPair *ep);
static void _usb_mass_storage_bulk_only_out(uint8_t ep_num, UsbEpPair *ep);
static void _usb_mass_storage_update(void);

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
int usbInstallMassStorageClass(void)
{
	//J SCSI モジュールの初期化
	scsiInitialize();

	//J Debug 領域の取得	
	sDebug = aiMini4WdAllocateMemory(sizeof(UsbMassStorageDebugContext));
	if (sDebug == NULL) {
		return SCSI_ERROR_NOMEM;
	}
	memset (sDebug, 0x00, sizeof(UsbMassStorageDebugContext));

	//J Descriptor 領域の用意
	//J Program Memoryからコピーする
	sUsbMassStorageDeviceDescriptor = aiMini4WdAllocateMemory(MASS_STORAGE_CLASS_DESCRIPTOR_SIZE);
	if (sUsbMassStorageDeviceDescriptor == NULL) {
		return SCSI_ERROR_NOMEM;
	}
	memset (sUsbMassStorageDeviceDescriptor, 0, MASS_STORAGE_CLASS_DESCRIPTOR_SIZE);
	
	uint16_t addr = (uint16_t)cUsbMassStorageClassDescriptor;
	for (uint16_t i=0 ; i < MASS_STORAGE_CLASS_DESCRIPTOR_SIZE ; ++i) {
		sUsbMassStorageDeviceDescriptor[i] = pgm_read_byte(addr + i);
	}

	//J 必要なバッファの確保
	sCommonBuf = aiMini4WdAllocateMemory(64);
	if (sCommonBuf == NULL) {
		return SCSI_ERROR_NOMEM;
	}
	
	sCbw = aiMini4WdAllocateMemory(sizeof(CBW));
	if (sCbw == NULL) {
		return SCSI_ERROR_NOMEM;
	}

	
	//J これが正しいかは知らんけど、Class Driverをインストール
	usb_register_device_descriptor(sUsbMassStorageDeviceDescriptor, MASS_STORAGE_CLASS_DESCRIPTOR_SIZE);

	usb_register_class_control_transer_callback(_usb_mass_storage_control_transfer_cb);
	usb_register_vender_control_transer_callback(NULL);
	usb_register_ep_transfer_done_callback(1, USB_IN,  _usb_mass_storage_bulk_only_in);
	usb_register_ep_transfer_done_callback(1, USB_OUT, _usb_mass_storage_bulk_only_out);
	usb_register_update_callback(1, _usb_mass_storage_update);

	return USB_OK;
}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
#define USB_MASS_CLASS_REQ_BULK_ONLY_MASS_STORAGE_RESET			(0xff)
#define USB_MASS_CLASS_REQ_GET_MAX_LUN							(0xfe)

/*--------------------------------------------------------------------------*/
static void _uab_mass_request_bulk_only_mass_storage_reset(UsbDeviceRequest *req)
{
	// スグにACKを返したい Status Stage用にEP0INも用意しておく
	UsbEpPair *ep = usb_get_ep_context(0);
	usb_transfer_status_stage(ep, USB_IN);
	usb_transfer_status_stage(ep, USB_OUT);
}

/*--------------------------------------------------------------------------*/
static void _uab_mass_request_get_max_lun(UsbDeviceRequest *req)
{
	uint8_t lun = 0;
	UsbEpPair *ep = usb_get_ep_context(0);
	usb_transfer_bulk_in (ep, &lun, req->wLength);

	//J 上の転送が終わった次のタイミングでStatus Stageを行うための準備
	usb_tranfer_trigger_status_stage();
}

/*--------------------------------------------------------------------------*/
static void _usb_mass_storage_control_transfer_cb(UsbDeviceRequest *req)
{
	switch (req->bRequest)
	{
		case USB_MASS_CLASS_REQ_BULK_ONLY_MASS_STORAGE_RESET:
		_uab_mass_request_bulk_only_mass_storage_reset(req);
		break;
		case USB_MASS_CLASS_REQ_GET_MAX_LUN:
		_uab_mass_request_get_max_lun(req);
		break;
		default:
		//J NAK返すべき？
		break;
	}
	return;
}


typedef enum MassStorageCommandProtocolState_t
{
	cStateReady = 0,
	cStateGotCBD = 1,
	cStateWaitForInDataReady = 2,
	cStateWaitDoneDataInTransfer = 3,
	cStateWaitDoneDataOutTransfer = 4,
	cStateWaitForOutDataProcessed = 5,
	cStateTriggerStatusTransport = 6,
	cStateWaitForDoneStatusTransport = 7
} MassStorageCommandProtocolState;

static MassStorageCommandProtocolState sProtocolState = cStateReady;
static inline MassStorageCommandProtocolState _usb_mass_storage_get_protocol_state(void)
{
	return sProtocolState;
}
static uint8_t _usb_mass_storage_set_protocol_state(MassStorageCommandProtocolState state)
{
	sProtocolState = state;

	sDebug->state_trace[sDebug->state_wptr] = state;
	sDebug->state_wptr = (sDebug->state_wptr + 1) & 0x0f;

	return sProtocolState;
}


/*--------------------------------------------------------------------------*/
static void _usb_mass_storage_send_csw(UsbEpPair *ep, uint32_t tag)
{
	CSW csw;
	csw.dCSWSignature = CSW_SIGNATURE;
	csw.dCSWTag = tag;
	csw.dCSWDataResidue = 0; // ?
	csw.bCSWStatus = CB_STATUS_PASSED;

	_usb_mass_storage_set_protocol_state(cStateWaitForDoneStatusTransport);
	usb_transfer_bulk_in(ep, &csw, sizeof(CSW));
	
	return;
}


/*--------------------------------------------------------------------------*/
static void _usb_mass_storage_bulk_only_in (uint8_t ep_num, UsbEpPair *ep)
{
	sDebug->interrupt_in++;

	if (sProtocolState == cStateTriggerStatusTransport) {
		_usb_mass_storage_send_csw(ep, sCbw->dCBWTag);
	}
	else if (sProtocolState == cStateWaitDoneDataInTransfer) {
		_usb_mass_storage_set_protocol_state(cStateWaitForInDataReady);
	}
	else if (sProtocolState == cStateWaitForDoneStatusTransport) {
		_usb_mass_storage_set_protocol_state(cStateReady);
	}


	//J 受け入れ可能状態にしておく
	//ep->in.STATUS = (ep->in.STATUS) & (0xb9);		//J Clear UNF/OVF and BUSNACK0
	usb_ep_ready(ep, USB_IN);

	return;
}

/*--------------------------------------------------------------------------*/
static void _usb_mass_storage_bulk_only_out(uint8_t ep_num, UsbEpPair *ep)
{
	sDebug->interrupt_out++;

	if (sProtocolState == cStateReady) {
		sDebug->num_cbw++;
		
		//J CBW コマンドが来ることが想定されるので、パースする
		CBW *cbw = (CBW *)ep->out.DATAPTR;		
		if (cbw->dCBWsignature != CBW_SIGNATURE) {
			return;
		}
		else {
			sCbdFlag = 1;
			memcpy(sCbw, cbw, sizeof(CBW));
		}

		//J IN (to host)
		if (cbw->bmCBWFlags.bm.direction == USB_IN) {
			//J ここで、IN EndpointをNAK 設定にしておく
			usb_ep_nack(ep, USB_IN, 1);
			_usb_mass_storage_set_protocol_state(cStateWaitForInDataReady);
		}
		//J OUT (from host)
		else {
			//J 死にたい -> そもそもIN/OUT関係なく、即Statusを返す系コマンドかどうかだけでも知るべきかも？
			uint8_t more_transfer = scsiCheckMoreTransfer(cbw->CBWB, cbw->bCBWCBLength);
			if (more_transfer){
				_usb_mass_storage_set_protocol_state(cStateWaitDoneDataOutTransfer);
			}
			else {
				_usb_mass_storage_send_csw(ep, sCbw->dCBWTag);
			}
			
		}
	}
	else if (sProtocolState == cStateWaitDoneDataOutTransfer) {
		// ?
		_usb_mass_storage_set_protocol_state(cStateWaitForOutDataProcessed);
		usb_ep_nack(ep, USB_OUT, 1);

		return;
	}
	else if (sProtocolState == cStateTriggerStatusTransport) {
		_usb_mass_storage_send_csw(ep, sCbw->dCBWTag);
	}

	//J 受け入れ可能状態にしておく
//	ep->out.STATUS = (ep->out.STATUS) & (0xb9);		//J Clear UNF/OVF and BUSNACK0
	usb_ep_nack(ep, USB_OUT, 0);
	usb_ep_ready(ep, USB_OUT);


	return;
}

/*--------------------------------------------------------------------------*/
static void _usb_mass_storage_update(void)
{
	if (sCbdFlag) {

	}

	UsbEpPair *ep  = usb_get_ep_context(1);

	if (sProtocolState == cStateWaitForInDataReady) {
		size_t response_size = 0;
		int8_t ret = scsiParseCommands(sCbw->CBWB, sCbw->bCBWCBLength, sCommonBuf, 64, &response_size);
		//J 継続転送出ない限りは次の転送完了後にStatus Transportを実施する
		if (ret != SCSI_ERROR_CONTINUE) {
			_usb_mass_storage_set_protocol_state(cStateTriggerStatusTransport);
		}
		else {
			_usb_mass_storage_set_protocol_state(cStateWaitDoneDataInTransfer);
		}

		if ((ret == SCSI_OK) || (ret == SCSI_ERROR_CONTINUE)){
			usb_transfer_bulk_in(ep, sCommonBuf, response_size);
		}
		else {
			// NACK?
			usb_ep_nack(ep, USB_IN, 1);
		}

	}
	else if (sProtocolState == cStateWaitForOutDataProcessed) {
		size_t response_size = 0;
		//J EPサイズの変更に対してロバストでない
		int8_t ret = scsiParseCommands(sCbw->CBWB, sCbw->bCBWCBLength, (uint8_t *)ep->out.DATAPTR, 64, &response_size);

		//J 継続転送出ない限りは次の転送完了後にStatus Transportを実施する
		if (ret != SCSI_ERROR_CONTINUE) {
			_usb_mass_storage_set_protocol_state(cStateTriggerStatusTransport);
		}
		else {
			_usb_mass_storage_set_protocol_state(cStateWaitDoneDataOutTransfer);
		}

		if (ret == SCSI_ERROR_CONTINUE){
			usb_transfer_bulk_out(ep); //もっとくれ
		}
		else if (ret == SCSI_OK){
			usb_transfer_bulk_out(ep); //EPを有効化

			 //J CSW Status をセットしておく
			_usb_mass_storage_send_csw(ep, sCbw->dCBWTag);
		}
		else {
			// NACK?
			usb_ep_nack(ep, USB_OUT, 1);
		}
	}	

	return;	
}


#if USB_DEBUG
extern uint32_t len;
extern char *line;

void usb_mass_storage_debug_dump(void)
{
	len = sprintf (line, "\r\nDebug Dump - MASS STORAGE\r\n");
	uart_txNonBuffer((uint8_t *)line, len);

	len = sprintf (line, "INT(IN) = %x, INT(OUT) = %x\r\n", sDebug->interrupt_in, sDebug->interrupt_out);
	uart_txNonBuffer((uint8_t *)line, len);

	len = sprintf (line, "Last CBW.dir = %d. CBW.CDB.cmd = 0x%02x\r\n", 
					sCbw->bmCBWFlags.bm.direction, sCbw->CBWB[0]);
	uart_txNonBuffer((uint8_t *)line, len);
	

	len = sprintf (line, "Current Protocol State = %d\r\n", sProtocolState);
	uart_txNonBuffer((uint8_t *)line, len);
	
	len = sprintf (line, "Num of Received CBW = %d\r\n", sDebug->num_cbw);
	uart_txNonBuffer((uint8_t *)line, len);

	len = sprintf (line, "Protocol State Transition: ");
	uart_txNonBuffer((uint8_t *)line, len);
	uint8_t i;
	uint8_t index = (sDebug->state_wptr - 1) & 0xf;
	for (i=0 ; i<16 ; ++i) {
		len = sprintf (line, "%d<", sDebug->state_trace[index]);
		uart_txNonBuffer((uint8_t *)line, len);

		index = (index - 1) & 0xf;
	}
	len = sprintf (line, "\r\n");
	uart_txNonBuffer((uint8_t *)line, len);

	
	return;
}
#else /*USB_DEBUG*/
void usb_mass_storage_debug_dump(void)
{
	return;
}
#endif