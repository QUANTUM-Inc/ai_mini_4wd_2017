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

#include <system/system_system.h>
#include <system/usb/usb.h>

/*--------------------------------------------------------------------------*/
// Endpoint Register 領域
/*--------------------------------------------------------------------------*/
typedef struct UsbEpTable_t
{
	UsbEpPair EP0;
	UsbEpPair EP1;
	UsbEpPair EP2;
	UsbEpPair EP3;
	UsbEpPair EP4;
	UsbEpPair EP5;
	UsbEpPair EP6;
	UsbEpPair EP7;
	UsbEpPair EP8;
	UsbEpPair EP9;
	UsbEpPair EP10;
	UsbEpPair EP11;
	UsbEpPair EP12;
	UsbEpPair EP13;
	UsbEpPair EP14;
	UsbEpPair EP15;
	register8_t reserved_0x100;
	register8_t reserved_0x101;
	register8_t reserved_0x102;
	register8_t reserved_0x103;
	register8_t reserved_0x104;
	register8_t reserved_0x105;
	register8_t reserved_0x106;
	register8_t reserved_0x107;
	register8_t reserved_0x108;
	register8_t reserved_0x109;
	register8_t reserved_0x10A;
	register8_t reserved_0x10B;
	register8_t reserved_0x10C;
	register8_t reserved_0x10D;
	register8_t reserved_0x10E;
	register8_t reserved_0x10F;
	register8_t FrameNumL;
	register8_t FrameNumH;
} UsbEpTable;


//static UsbEpTable __attribute__ ((aligned(2))) sUsbEp;
static UsbEpTable *sUsbEp = NULL;

/*--------------------------------------------------------------------------*/
// USBの各種転送完了時のコールバック関数類
/*--------------------------------------------------------------------------*/
static UsbControlTansferCallback sUsbClassSpecificControlCb = NULL;
static UsbControlTansferCallback sUsbVenderSpecificControlCb = NULL;



typedef struct UsbEpTransferDoneCallbacl_t
{
	UsbTranferDoneCb out;
	UsbTranferDoneCb in;
} UsbEpTransferDoneCallback;

//J 共通のコールバック
static void _default_ep_transfer_cb(uint8_t ep_num, UsbEpPair *ep);
static void _ep0_out_transfer_cb(uint8_t ep_num, UsbEpPair *ep);
static void _ep0_in_transfer_cb (uint8_t ep_num, UsbEpPair *ep);

//J 各エンドポイント用のコールバック
#define SUPPORTED_NUM_EP				(16)
static UsbEpTransferDoneCallback *sEpTransferDoneCb = NULL;
static UsbUpdateCb *sUsbUpdateCb = NULL;



/*--------------------------------------------------------------------------*/
// Control 転送の Status Stageを行うために必要なフラグ
/*--------------------------------------------------------------------------*/
typedef enum UsbSetupStageType_t
{
	cUsbSetupStatusStageNone = 0,
	cUsbSetupStatusStage	 = 1,
} UsbSetupStageType;

static UsbSetupStageType sSetupStatusStageFlag = cUsbSetupStatusStageNone;

/*--------------------------------------------------------------------------*/
// Standard Request Processor
/*--------------------------------------------------------------------------*/
typedef enum UsbStandardCommand_t
{
	cGetStatus			= 0x00,
	cClearFeature		= 0x01,
	cSetFeature			= 0x03,
	cSetAddress			= 0x05,
	cGetDescriptor		= 0x06,
	cSetDescriptor		= 0x07,
	cGetConfiguration	= 0x08,
	cSetConfiguration	= 0x09,
	cGetInterface		= 0x0A,
	cSetInterface		= 0x0B,
	cSynchFrame			= 0x0C
} UsbStandardCommand;

static void _usb_std_request_get_status(UsbDeviceRequest *req);
static void _usb_std_request_clear_feature(UsbDeviceRequest *req);
static void _usb_std_request_set_feature(UsbDeviceRequest *req);
static void _usb_std_request_set_address(UsbDeviceRequest *req);
static void _usb_std_request_get_descriptor(UsbDeviceRequest *req);
static void _usb_std_request_set_descriptor(UsbDeviceRequest *req);
static void _usb_std_request_get_configuration(UsbDeviceRequest *req);
static void _usb_std_request_set_configuration(UsbDeviceRequest *req);
static void _usb_std_request_get_interface(UsbDeviceRequest *req);
static void _usb_std_request_set_interface(UsbDeviceRequest *req);

static void _usb_dispatch_standard_request(UsbDeviceRequest *req);
static void _usb_dispatch_class_request(UsbDeviceRequest *req);
static void _usb_dispatch_vender_request(UsbDeviceRequest *req);

static int _usb_search_descriptor(const uint8_t *descroptor, uint16_t descriptor_len, uint8_t type, uint8_t index, uint16_t required_len, uint8_t **found_desc, uint16_t *len);


/*
 * USBデバイスの初期化方法2種類
 */
static int _initialize_usb_full_function(void);
static int _initialize_usb_just_device(void);


/*--------------------------------------------------------------------------*/
// Endpoint Buffer そのうちAllocateする形に変える
/*--------------------------------------------------------------------------*/
#define	EP_BUF_SIZE			(64)

static uint8_t *sEp0InBuf  = NULL;
static uint8_t *sEp0OutBuf  = NULL;

static uint8_t *sEp1InBuf  = NULL;
static uint8_t *sEp1OutBuf  = NULL;

//J Device Address を割り込みハンドラ内で処理すると死ぬので、メインループ側で処理させる
static uint8_t sUsbDeviceAddress = 0;
static volatile uint8_t sTempSetAddress = 0;

static const uint8_t *sUsbDescriptor = NULL;
static uint16_t sUsbDescriptorLen = 0; 

extern int len;
extern char *line;


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
int initialize_usb(uint8_t full_function)
{
	if (full_function) {
		return _initialize_usb_full_function();
	}
	else {
		return _initialize_usb_just_device();
	}
}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
static void _initialize_usb_cpu_regs(void)
{
	//J これだけは税金で必要か…
	sUsbEp = aiMini4WdAllocateMemory(sizeof(UsbEpTable));
	if (sUsbEp == NULL) {
		return;
	}
	else {
		memset (sUsbEp, 0x00, sizeof(UsbEpTable));
	}

	//------------------------------------------------------------------------
	//------------------------------------------------------------------------
	//J PLLを使って48MHzを生成する
	//J Internal の 2MHzオシレータ起動
	OSC.CTRL |= OSC_RC2MEN_bm;
	while (0 == (OSC.STATUS & OSC_RC2MEN_bm));

	//J PLLで2MHz を 24倍して 48MHzを生成
	OSC.PLLCTRL =	(OSC_PLLSRC_RC2M_gc) |
					(0 << OSC_PLLDIV_bp) |
					(24);
	OSC.CTRL |= OSC_PLLEN_bm;
	while (0 == (OSC.STATUS & OSC_PLLEN_bm));

	//J USBのクロックを有効化
	CPU_CCP = CCP_IOREG_gc;
	CLK.USBCTRL =	(CLK_USBPSDIV_1_gc) |
					(CLK_USBSRC_PLL_gc) |
					(1 << CLK_USBSEN_bp);

	//------------------------------------------------------------------------
	//------------------------------------------------------------------------
	//J 20.14.1 CTRLA - Control Register A
	USB.CTRLA = (1 << USB_ENABLE_bp)  |		// USB Enable
				(1 << USB_SPEED_bp)   |		// Speed Select 0: low speed, 1: full speed
				(0 << USB_FIFOEN_bp)  |		// USB FIFO Enable
				(1 << USB_STFRNUM_bp) |		// Store Frame Number Enable
				(15<< USB_MAXEP_gp);		// Maximum Endpoint Address


	//J 20.14.2 CTRLB - Control register B
	USB.CTRLB = (0 << USB_PULLRST_bp) |		// Pull during Reset
				(0 << USB_RWAKEUP_bp) |		// Remote Wake-up Enable
				(0 << USB_GNACK_bp)   |		// Global NACK
				(0 << USB_ATTACH_bp);		// Attach USB Device to BUS Line
	

	//J 20.14.4 ADDR - Address register
	USB.ADDR = 0;							// USB Address Register
	
	//J 20.14.5 FIFOWP ? FIFO Write Pointer register
	USB.FIFOWP = 0;
	
	//J 20.14.6 FIFORP - FIFO Read Pointer register
	USB.FIFORP = 0;

	//J 20.14.7 EPPTRL - Endpoint Configuration Table Pointer Low
	//J 20.14.8 EPPTRH - Endpoint Configuration Table Pointer High
	USB.EPPTR = (uint16_t)sUsbEp; // EP Configuration Register へのポインタを置く

	//J 20.14.9 INTCTRLA - Interrupt Control Register A
	USB.INTCTRLA =	(1 << USB_SOFIE_bp) |		// Start of Frame Interrupt Enable
					(1 << USB_BUSEVIE_bp)  |	// Bus Event Interrupt Enable
					(1 << USB_BUSERRIE_bp) |	// Bus Error Interrupt Enable
					(1 << USB_STALLIE_bp)  |	// STALL Interrupt Enable
					USB_INTLVL_HI_gc;			// Interrupt Level = High

	//J 20.14.10 INTCTRLB - Interrupt Control Register B
	USB.INTCTRLB =  (1 << USB_TRNIE_bp) |		// Transaction Complete Interrupt Enable
					(1 << USB_SETUPIE_bp);		// SETUP Transaction Complete Interrupt Enable

	//J 20.14.11 INTFLAGSACLR - Clear Interrupt Flag register A
	USB.INTFLAGSACLR = 0xff;	//J Clear All Flags

	//J 20.14.12 INTFLAGSBCLR - Clear Interrupt Flag register B
	USB.INTFLAGSBCLR = 0xff;	//J Clear All Flags

	//J 20.14.13 CAL0 - Calibration Low
	USB.CAL0 = 0;

	//J 20.14.14 CAL1 - Calibration High
	USB.CAL1 = 0;

	return;
}


/*--------------------------------------------------------------------------*/
static int _initialize_usb_just_device(void)
{
	_initialize_usb_cpu_regs();

	//J Attach USB	
	//J 20.14.2 CTRLB - Control register B
	USB.CTRLB = (0 << USB_PULLRST_bp) |		// Pull during Reset
				(0 << USB_RWAKEUP_bp) |		// Remote Wake-up Enable
				(0 << USB_GNACK_bp)   |		// Global NACK
				(1 << USB_ATTACH_bp);		// Attach USB Device to BUS Line
	
	return 0;
}


/*--------------------------------------------------------------------------*/
static int _initialize_usb_full_function(void)
{

	sEp0InBuf  = aiMini4WdAllocateMemory((EP_BUF_SIZE));
	sEp0OutBuf = aiMini4WdAllocateMemory((EP_BUF_SIZE));
	sEp1InBuf  = aiMini4WdAllocateMemory((EP_BUF_SIZE));
	sEp1OutBuf = aiMini4WdAllocateMemory((EP_BUF_SIZE));
	if ((sEp0InBuf == NULL) || (sEp0OutBuf == NULL) || (sEp1InBuf == NULL) || (sEp1OutBuf == NULL)) {
		return USB_ERROR_NOMEM;
	}

	sEpTransferDoneCb = aiMini4WdAllocateMemory(sizeof(UsbEpTransferDoneCallback) * SUPPORTED_NUM_EP);
	if (sEpTransferDoneCb == NULL) {
		return USB_ERROR_NOMEM;
	}
	else {
		memset (sEpTransferDoneCb, 0x00, sizeof(UsbEpTransferDoneCallback) * SUPPORTED_NUM_EP);		
	}

	sUsbUpdateCb = aiMini4WdAllocateMemory(sizeof(UsbUpdateCb) * SUPPORTED_NUM_EP);
	if (sUsbUpdateCb == NULL) {
		return USB_ERROR_NOMEM;
	}
	else {
		memset (sUsbUpdateCb, 0x00, sizeof(UsbUpdateCb) * SUPPORTED_NUM_EP);
	}

	//J Table の初期値を設定する
	for (uint8_t i=0 ; i<SUPPORTED_NUM_EP ; ++i) {
		sEpTransferDoneCb[i].out = _default_ep_transfer_cb;
		sEpTransferDoneCb[i].in  = _default_ep_transfer_cb;
		sUsbUpdateCb[i] = NULL;
	}

	usb_register_ep_transfer_done_callback(0, USB_IN,  _ep0_in_transfer_cb);
	usb_register_ep_transfer_done_callback(0, USB_OUT, _ep0_out_transfer_cb);


	//J CPUレジスタの初期化
	_initialize_usb_cpu_regs();
	
	//------------------------------------------------------------------------
	// Endpoint Configuration / EP0(IN/OUT)
	//------------------------------------------------------------------------
	//J 20.15.1 Status
	sUsbEp->EP0.in.CTRL = (USB_EP_TYPE_CONTROL_gc) |
						(0 << USB_EP_MULTIPKT_bp) |		// Multi packet Transfer Enable
						(0 << USB_EP_PINGPONG_bp) |		// Ping Pong
						(0 << USB_EP_INTDSBL_bp) |		// Interrupt Disable
						(0 << USB_EP_STALL_bp) |		// Endpoint STALL
						(USB_EP_BUFSIZE_64_gc);

	sUsbEp->EP0.in.STATUS &= ~(1 << USB_EP_BUSNACK0_bp);
	sUsbEp->EP0.in.STATUS &= ~(1 << USB_EP_BUSNACK1_bp);

	sUsbEp->EP0.out.CTRL =(USB_EP_TYPE_CONTROL_gc) |
						(0 << USB_EP_MULTIPKT_bp) |		// Multi packet Transfer Enable
						(0 << USB_EP_PINGPONG_bp) |		// Ping Pong
						(0 << USB_EP_INTDSBL_bp) |		// Interrupt Disable
						(0 << USB_EP_STALL_bp) |		// Endpoint STALL
						(USB_EP_BUFSIZE_64_gc);

	//J Setup EP0(CONTROL転送に必要なEPなのでどの個体でも絶対使う)
	uint8_t *pBuf = sEp0InBuf;
	uint16_t pBufValue = (uint16_t)pBuf;
	sUsbEp->EP0.in.DATAPTRH = (pBufValue >> 8) & 0xff;
	sUsbEp->EP0.in.DATAPTRL = (pBufValue >> 0) & 0xff;
	sUsbEp->EP0.in.CNT = 0x8000;

	pBuf = sEp0OutBuf;
	pBufValue = (uint16_t)pBuf;
	sUsbEp->EP0.out.DATAPTRH = (pBufValue >> 8) & 0xff;
	sUsbEp->EP0.out.DATAPTRL = (pBufValue >> 0) & 0xff;

	//J 20.14.2 CTRLB - Control register B
	USB.CTRLB = (0 << USB_PULLRST_bp) |		// Pull during Reset
	(0 << USB_RWAKEUP_bp) |		// Remote Wake-up Enable
	(0 << USB_GNACK_bp)   |		// Global NACK
	(1 << USB_ATTACH_bp);		// Attach USB Device to BUS Line


	//------------------------------------------------------------------------
	// Endpoint Configuration / EP1(IN/OUT)
	//J 本当はここをAPIで切り出したいし、切り出さないと本当の意味で汎用性出せない
	//------------------------------------------------------------------------
	//J 20.15.1 Status
	sUsbEp->EP1.in.CTRL = (USB_EP_TYPE_BULK_gc) |
						(0 << USB_EP_MULTIPKT_bp) |		// Multi packet Transfer Enable
						(0 << USB_EP_PINGPONG_bp) |		// Ping Pong
						(0 << USB_EP_INTDSBL_bp) |		// Interrupt Disable
						(0 << USB_EP_STALL_bp) |		// Endpoint STALL
						(USB_EP_BUFSIZE_64_gc);

	sUsbEp->EP1.in.STATUS &= ~(1 << USB_EP_BUSNACK0_bp);
	sUsbEp->EP1.in.STATUS &= ~(1 << USB_EP_BUSNACK1_bp);

	sUsbEp->EP1.out.CTRL =(USB_EP_TYPE_BULK_gc) |
						(0 << USB_EP_MULTIPKT_bp) |		// Multi packet Transfer Enable
						(0 << USB_EP_PINGPONG_bp) |		// Ping Pong
						(0 << USB_EP_INTDSBL_bp) |		// Interrupt Disable
						(0 << USB_EP_STALL_bp) |		// Endpoint STALL
						(USB_EP_BUFSIZE_64_gc);


	pBuf = sEp1InBuf;
	pBufValue = (uint16_t)pBuf;
	sUsbEp->EP1.in.DATAPTRH = (pBufValue >> 8) & 0xff;
	sUsbEp->EP1.in.DATAPTRL = (pBufValue >> 0) & 0xff;
	sUsbEp->EP1.in.CNT = 0x8000;

	pBuf = sEp1OutBuf;
	pBufValue = (uint16_t)pBuf;
	sUsbEp->EP1.out.DATAPTRH = (pBufValue >> 8) & 0xff;
	sUsbEp->EP1.out.DATAPTRL = (pBufValue >> 0) & 0xff;

	//J 20.14.2 CTRLB - Control register B
	USB.CTRLB = (0 << USB_PULLRST_bp) |		// Pull during Reset
				(0 << USB_RWAKEUP_bp) |		// Remote Wake-up Enable
				(0 << USB_GNACK_bp)   |		// Global NACK
				(1 << USB_ATTACH_bp);		// Attach USB Device to BUS Line
	return 0;
}

/*---------------------------------------------------------------------------*/
int usb_poll_status(void)
{
	if (sTempSetAddress == 2) {
		sTempSetAddress = 0;
		USB.ADDR = sUsbDeviceAddress;
	}
	
	if (sUsbUpdateCb != NULL) {
		uint8_t i=0;
		for (i=0 ; i<15 ; ++i) {
			if (sUsbUpdateCb[i] != NULL) {
				sUsbUpdateCb[i]();
			}
		}
	}
	
	
	return USB_OK;	
}


/*---------------------------------------------------------------------------*/
int usb_ep_stall (UsbEpPair *ep, uint8_t in, uint8_t stall)
{
	if (ep == NULL) {
		return USB_ERROR_INVALID_EP;
	}

	USB_EP_t *_ep = NULL;
	if (in) {
		_ep = &(ep->in);
	}
	else {
		_ep = &(ep->out);
	}

	if (stall) {
		_ep->CTRL |= (1 << USB_EP_STALL_bp);
	}
	else {
		_ep->CTRL &=~(1 << USB_EP_STALL_bp);
	}

	return 0;
}


/*---------------------------------------------------------------------------*/
int usb_ep_nack(UsbEpPair *ep, uint8_t in, uint8_t nack)
{
	if (ep == NULL) {
		return USB_ERROR_INVALID_EP;
	}

	USB_EP_t *_ep = NULL;
	if (in) {
		_ep = &(ep->in);
	}
	else {
		_ep = &(ep->out);
	}

	if (nack) {
		_ep->STATUS |= ((1 << USB_EP_BUSNACK0_bp) | (1 << USB_EP_BUSNACK1_bp));
	}
	else {
		_ep->STATUS &=~((1 << USB_EP_BUSNACK0_bp) | (1 << USB_EP_BUSNACK1_bp));
	}

	return 0;
}

/*---------------------------------------------------------------------------*/
int usb_ep_ready(UsbEpPair *ep, uint8_t in)
{
	if (ep == NULL) {
		return USB_ERROR_INVALID_EP;
	}

	USB_EP_t *_ep = NULL;
	if (in) {
		_ep = &(ep->in);
	}
	else {
		_ep = &(ep->out);
	}

	_ep->STATUS &= ~(1 << USB_EP_OVF_bp);

	return 0;	
}


/*---------------------------------------------------------------------------*/
int usb_transfer_bulk_in(UsbEpPair *ep, void *buf, size_t len)
{
	if (ep == NULL) {
		return USB_ERROR_INVALID_EP;
	}
	if ((buf == NULL) && (len != 0)) {
		return USB_ERROR_NOBUF;
	}

	size_t copy_size = (len < EP_BUF_SIZE) ? len : EP_BUF_SIZE;
	if (copy_size != 0) {
		memcpy((void *)ep->in.DATAPTR, buf, copy_size);
	}

	//J バッファの中身を送る
	ep->in.CNT     = copy_size;
	ep->in.STATUS &= ~(1 << USB_EP_STALLF_bp); // Stall フラグを落とす

	usb_ep_ready(ep, USB_IN);
	usb_ep_nack(ep, USB_IN, 0);

	return 0;
}

/*---------------------------------------------------------------------------*/
int usb_transfer_bulk_out(UsbEpPair *ep)
{
	if (ep == NULL) {
		return USB_ERROR_INVALID_EP;
	}

	ep->out.CNT    = 0;
	ep->out.STATUS &= ~(1 << USB_EP_STALLF_bp); // Stall フラグを落とす

	usb_ep_ready(ep, USB_OUT);
	usb_ep_nack(ep, USB_OUT, 0);

	return 0;
}

/*---------------------------------------------------------------------------*/
void usb_transfer_status_stage(UsbEpPair *ep, uint8_t in)
{
	if (in) {
		(void)usb_transfer_bulk_in(ep, NULL, 0);
	}
	else {
		(void)usb_transfer_bulk_out(ep);
	}
}

/*---------------------------------------------------------------------------*/
void usb_tranfer_trigger_status_stage(void)
{
	sSetupStatusStageFlag = cUsbSetupStatusStage;
}

/*---------------------------------------------------------------------------*/
UsbEpPair *usb_get_ep_context(int8_t epnum)
{
	UsbEpPair *pEp= &(sUsbEp->EP0);
	
	return &(pEp[epnum]);
}

/*---------------------------------------------------------------------------*/
int usb_register_device_descriptor(const uint8_t *descriptor, uint16_t descriptor_len)
{
	sUsbDescriptor    = descriptor;
	sUsbDescriptorLen = descriptor_len;
	return USB_OK;
}

/*---------------------------------------------------------------------------*/
int usb_register_class_control_transer_callback(UsbControlTansferCallback cb)
{
	sUsbClassSpecificControlCb = cb;

	return USB_OK;
}

/*---------------------------------------------------------------------------*/
int usb_register_vender_control_transer_callback(UsbControlTansferCallback cb)
{
	sUsbVenderSpecificControlCb = cb;

	return USB_OK;
}


/*---------------------------------------------------------------------------*/
int usb_register_ep_transfer_done_callback(uint8_t ep_num, uint8_t in, UsbTranferDoneCb cb)
{
	if (ep_num > 15) {
		return USB_ERROR_INVALID_EP;
	}

	if (in) {
		sEpTransferDoneCb[ep_num].in = cb;
	}
	else {
		sEpTransferDoneCb[ep_num].out = cb;
	}

	return USB_OK;
}

/*---------------------------------------------------------------------------*/
int usb_register_update_callback(uint8_t ep_num, UsbUpdateCb cb)
{
	if (ep_num > 15) {
		return USB_ERROR_INVALID_EP;
	}

	sUsbUpdateCb[ep_num] = cb;

	return USB_OK;
}

/*---------------------------------------------------------------------------*/
static USB_DEVICE_STATE sUsbDeviceState = USB_DEVICE_STATE_WAIT_BUS_RESET;

USB_DEVICE_STATE usb_get_device_state(void)
{
	return sUsbDeviceState;
}

int8_t usb_set_device_state(USB_DEVICE_STATE state)
{
	//J State 変化は
	//J Wait Bus Reset -> Reset when bus connected
	//J Wait Bus Reset -> Pend Reset
	//J Reset when bus connected -> pend reset
	//J Pend Reset -> Reset when bus connected
	//J に限る

	//J CPU Reset がPendされている場合にはリセットする
	if (sUsbDeviceState == USB_DEVICE_STATE_WAIT_CPU_RESET) {
		if (state == USB_DEVICE_STATE_RESET_WHEN_BUS_CONNECTED) {
			//J CPU Reset
			aiMini4WdRebootForSystem(0);
		}
	}
	else if (sUsbDeviceState == USB_DEVICE_STATE_WAIT_BUS_RESET) {
		if (state == USB_DEVICE_STATE_RESET_WHEN_BUS_CONNECTED) {
			sUsbDeviceState = state;
			return 0;
		}
		else if (state == USB_DEVICE_STATE_PEND_CPU_RESET) {
			sUsbDeviceState = state;
			return 0;
		}
	}
	else if (sUsbDeviceState == USB_DEVICE_STATE_RESET_WHEN_BUS_CONNECTED) {
		if (state == USB_DEVICE_STATE_PEND_CPU_RESET){
			sUsbDeviceState = state;
			return 0;
		}
	}
	else if (sUsbDeviceState == USB_DEVICE_STATE_PEND_CPU_RESET) {
		if (state == USB_DEVICE_STATE_RESET_WHEN_BUS_CONNECTED){
			sUsbDeviceState = state;
			return 0;
		}
	}

	return -1;
}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
ISR(USB_BUSEVENT_vect)
{
	//J BUS SOFを受け取ったときに必要ならリセットさせる
	if (USB.INTFLAGSACLR & USB_SOFIF_bm) {
		//J USBバスに接続された場合にリセットさせる (Hot Plugされた時に他の挙動を殺してでもUSBデバイスを起動するため）
		if (sUsbDeviceState == USB_DEVICE_STATE_RESET_WHEN_BUS_CONNECTED) {
			aiMini4WdRebootForSystem(0);
		}
		else if (sUsbDeviceState == USB_DEVICE_STATE_PEND_CPU_RESET) {
			sUsbDeviceState = USB_DEVICE_STATE_WAIT_CPU_RESET;
		}
		else if (sUsbDeviceState == USB_DEVICE_STATE_WAIT_BUS_RESET) {
			sUsbDeviceState = USB_DEVICE_STATE_CONNECTED;
		}
	}
	
	USB.INTFLAGSACLR = 0xff;
}

/*---------------------------------------------------------------------------*/
ISR(USB_TRNCOMPL_vect)
{
	// SETUP transaction
	if (USB.INTFLAGSBSET & USB_SETUPIF_bm) {		
		UsbDeviceRequest *req = (UsbDeviceRequest *)sUsbEp->EP0.out.DATAPTR;
		if (req->bmRequestType.bm.target == UsbBmRequestTypeStandard) {
			_usb_dispatch_standard_request(req);
		}
		else if (req->bmRequestType.bm.target == UsbBmRequestTypeClass) {
			_usb_dispatch_class_request(req);
		}
		else if (req->bmRequestType.bm.target == UsbBmRequestTypeVender) {
			_usb_dispatch_vender_request(req);
		}
		else {
			
		}
		
		//J EP0のTANCOMPL0 を落としておく
		sUsbEp->EP0.out.STATUS &= ~(1 << USB_EP_TRNCOMPL0_bp);
	}
	// Transaction
	else if (USB.INTFLAGSBSET & USB_TRNIF_bm) {
		//J 有効なEPを走査してTRNCOMPL0 が立っているものを探す
		uint8_t nEp = 1 + ((USB.CTRLA & USB_MAXEP_gm) >> USB_MAXEP_gp); // EP0 は常に有効
		uint8_t i=0;
		UsbEpPair *pEp = &(sUsbEp->EP0); //OUT, IN の並びになっている
		for (i=0 ; i<nEp ; ++i) {
			// OUT
			if (pEp->out.STATUS & USB_EP_TRNCOMPL0_bm) {
				sEpTransferDoneCb[i].out(i, pEp);

				pEp->out.STATUS &= ~USB_EP_TRNCOMPL0_bm;
			}

			// IN
			if (pEp->in.STATUS & USB_EP_TRNCOMPL0_bm) {
				sEpTransferDoneCb[i].in(i, pEp);

				pEp->in.STATUS &= ~USB_EP_TRNCOMPL0_bm;
			}
			pEp++;
		}
	}

	USB.FIFORP = 0;
	USB.INTFLAGSBCLR = 0xff;

	return;
}



/*---------------------------------------------------------------------------*/
static void _usb_std_request_get_status(UsbDeviceRequest *req)
{
#if USB_DEBUG
	len = sprintf (line, "%s()\r\n", __FUNCTION__);
	uart_txNonBuffer((uint8_t *)line, len);
#endif
}

/*---------------------------------------------------------------------------*/
static void _usb_std_request_clear_feature(UsbDeviceRequest *req)
{
#if USB_DEBUG
	len = sprintf (line, "%s()\r\n", __FUNCTION__);
	uart_txNonBuffer((uint8_t *)line, len);
#endif
}

/*---------------------------------------------------------------------------*/
static void _usb_std_request_set_feature(UsbDeviceRequest *req)
{
#if USB_DEBUG
	len = sprintf (line, "%s()\r\n", __FUNCTION__);
	uart_txNonBuffer((uint8_t *)line, len);
#endif
}

/*---------------------------------------------------------------------------*/
static void _usb_std_request_set_address(UsbDeviceRequest *req)
{
	uint16_t address = req->wValue;

	sUsbDeviceAddress = (uint8_t)(address & USB_ADDR_gm);
	sTempSetAddress = 1;
	
	// スグにACKを返したい Status Stage用にEP0INも用意しておく
	usb_transfer_bulk_out(&(sUsbEp->EP0));
	usb_transfer_bulk_in(&(sUsbEp->EP0), NULL, 0);

	return;
}

/*---------------------------------------------------------------------------*/
static void _usb_std_request_get_descriptor(UsbDeviceRequest *req)
{

	uint16_t descriptor_type  = (req->wValue >> 8) & 0xff;
	uint16_t descriptor_index = (req->wValue >> 0) & 0xff;
	uint16_t required_len  = req->wLength;

	uint8_t *desc = NULL;
	uint16_t actual_descriptor_len = 0;
	int ret = _usb_search_descriptor(sUsbDescriptor, sUsbDescriptorLen, descriptor_type, descriptor_index, required_len, &desc, &actual_descriptor_len);
	if (ret != USB_OK) {
		desc = NULL;
		actual_descriptor_len = 0;
	}

	usb_transfer_bulk_in (&(sUsbEp->EP0), desc, actual_descriptor_len);

	//J 上の転送が終わった次のタイミングでStatus Stageを行うための準備
	sSetupStatusStageFlag = cUsbSetupStatusStage;

	return;
}

/*---------------------------------------------------------------------------*/
static void _usb_std_request_set_descriptor(UsbDeviceRequest *req)
{
#if USB_DEBUG
	len = sprintf (line, "%s()\r\n", __FUNCTION__);
	uart_txNonBuffer((uint8_t *)line, len);
#endif

	return;
}

/*---------------------------------------------------------------------------*/
static void _usb_std_request_get_configuration(UsbDeviceRequest *req)
{
#if USB_DEBUG
	len = sprintf (line, "%s()\r\n", __FUNCTION__);
	uart_txNonBuffer((uint8_t *)line, len);
#endif
}

/*---------------------------------------------------------------------------*/
static void _usb_std_request_set_configuration(UsbDeviceRequest *req)
{
	// スグにACKを返したい Status Stage用にEP0INも用意しておく
	usb_transfer_bulk_out(&(sUsbEp->EP0));

	usb_transfer_bulk_in(&(sUsbEp->EP0), NULL, 0);	
}

/*---------------------------------------------------------------------------*/
static void _usb_std_request_get_interface(UsbDeviceRequest *req)
{
#if USB_DEBUG
	len = sprintf (line, "%s()\r\n", __FUNCTION__);
	uart_txNonBuffer((uint8_t *)line, len);
#endif
}

/*---------------------------------------------------------------------------*/
static void _usb_std_request_set_interface(UsbDeviceRequest *req)
{
#if USB_DEBUG
	len = sprintf (line, "%s()\r\n", __FUNCTION__);
	uart_txNonBuffer((uint8_t *)line, len);
#endif
}

/*---------------------------------------------------------------------------*/
static void _usb_dispatch_standard_request(UsbDeviceRequest *req)
{
	if (req == NULL) {
		return;
	}
	
	uint8_t std_request = req->bRequest;
	switch (std_request)
	{
	case cGetStatus:
		_usb_std_request_get_status(req);
		break;
	case cClearFeature:
		_usb_std_request_clear_feature(req);
		break;
	case cSetFeature:
		_usb_std_request_set_feature(req);
		break;
	case cSetAddress:
		_usb_std_request_set_address(req);
		break;
	case cGetDescriptor:
		_usb_std_request_get_descriptor(req);
		break;
	case cSetDescriptor:
		_usb_std_request_set_descriptor(req);
		break;
	case cGetConfiguration:
		_usb_std_request_get_configuration(req);
		break;
	case cSetConfiguration:
		_usb_std_request_set_configuration(req);
		break;
	case cGetInterface:
		_usb_std_request_get_interface(req);
		break;
	case cSetInterface:
		_usb_std_request_set_interface(req);
		break;
	case cSynchFrame:
		//DNI
		break;
	default:
		break;
	}
}
 
/*---------------------------------------------------------------------------*/
static void _usb_dispatch_class_request(UsbDeviceRequest *req)
{
	if (sUsbClassSpecificControlCb) {
		sUsbClassSpecificControlCb(req);
	}

	return;
}

/*---------------------------------------------------------------------------*/
static void _usb_dispatch_vender_request(UsbDeviceRequest *req)
{
	if (sUsbVenderSpecificControlCb) {
		sUsbVenderSpecificControlCb(req);
	}
	
	return;	
}




/*---------------------------------------------------------------------------*/
static void _default_ep_transfer_cb(uint8_t ep_num, UsbEpPair *ep)
{
	return;
}

/*---------------------------------------------------------------------------*/
static void _ep0_out_transfer_cb(uint8_t ep_num, UsbEpPair *ep)
{
	//J Status Stage
	sSetupStatusStageFlag = cUsbSetupStatusStage;
	usb_transfer_status_stage(ep, USB_IN);
}

/*---------------------------------------------------------------------------*/
static void _ep0_in_transfer_cb (uint8_t ep_num, UsbEpPair *ep)
{
	//J Status Stage
	sSetupStatusStageFlag = cUsbSetupStatusStage;
	usb_transfer_status_stage(ep, USB_OUT);

	//J ここが正しいのかといわれるととても心苦しい
	if (sTempSetAddress == 1) {
		sTempSetAddress = 2;
	}
}

/*---------------------------------------------------------------------------*/
typedef union UsbDescritorHead_t
{
	uint8_t array[0];
	struct {
		uint8_t bLength;
		uint8_t bDescriptorType;
		uint8_t bytes[0];
	} desc;
} UsbDescritprHead;

/*---------------------------------------------------------------------------*/
static int _usb_search_descriptor(const uint8_t *descroptor, uint16_t descriptor_len, uint8_t type, uint8_t index, uint16_t required_len, uint8_t **found_desc, uint16_t *len)
{
	if (descroptor == NULL) {
		return USB_ERROR_NULL;
	}
	
	if ((found_desc == NULL) || (len == NULL)) {
		return USB_ERROR_NOBUF;
	}
	
	//J descriptor を走査して対象となるdescriptor の先頭を見つける
	uint8_t current_index = 0;
	UsbDescritprHead *head = (UsbDescritprHead *)descroptor;
	while (((uint16_t)head - (uint16_t)descroptor) < descriptor_len) {
		if (head->desc.bDescriptorType == type) {
			if (current_index == index) {
				*found_desc = head->array;
				*len = head->desc.bLength;

				//J Configuration Descriptor が全長で要求された場合は全長を返す
				if (type == 0x02) {
					uint16_t configuration_descriptor_len = head->desc.bytes[0] | (((uint16_t)head->desc.bytes[1])<< 8);
					if (configuration_descriptor_len == required_len) {
						*len = configuration_descriptor_len;
					}
				}
				
				return 0;
			}
			else {
				current_index++;
			}
		}

		head = (UsbDescritprHead *)((uint16_t)head + (uint16_t)head->desc.bLength);
	}

	return USB_ERROR_NOT_FOUND;
}




/*--------------------------------------------------------------------------*/
// Debug Function
/*--------------------------------------------------------------------------*/
#if USB_DEBUG
static void _usb_dump_ep_regs(USB_EP_t *ep, const char *label);
static void _usb_dump_ep_data(USB_EP_t *ep, const char *label);

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
void usb_dump_regs(void)
{
	len = sprintf (line, "USB Register Dump\r\n");
	uart_txNonBuffer((uint8_t*)line, len);
	
	len = sprintf (line, "CTRLA        = 0x%02x\r\n", USB.CTRLA);
	uart_txNonBuffer((uint8_t*)line, len);
	
	len = sprintf (line, "CTRLB        = 0x%02x\r\n", USB.CTRLB);
	uart_txNonBuffer((uint8_t*)line, len);
	
	len = sprintf (line, "STATUS       = 0x%02x\r\n", USB.STATUS);
	uart_txNonBuffer((uint8_t*)line, len);

	len = sprintf (line, "ADDR         = 0x%02x\r\n", USB.ADDR);
	uart_txNonBuffer((uint8_t*)line, len);

	len = sprintf (line, "FIFOWP       = 0x%02x\r\n", USB.FIFOWP);
	uart_txNonBuffer((uint8_t*)line, len);

	len = sprintf (line, "FIFORP       = 0x%02x\r\n", USB.FIFORP);
	uart_txNonBuffer((uint8_t*)line, len);

	len = sprintf (line, "EPPTRL       = 0x%02x\r\n", USB.EPPTRL);
	uart_txNonBuffer((uint8_t*)line, len);

	len = sprintf (line, "EPPTRH       = 0x%02x\r\n", USB.EPPTRH);
	uart_txNonBuffer((uint8_t*)line, len);

	len = sprintf (line, "INTCTRLA     = 0x%02x\r\n", USB.INTCTRLA);
	uart_txNonBuffer((uint8_t*)line, len);

	len = sprintf (line, "INTCTRLB     = 0x%02x\r\n", USB.INTCTRLB);
	uart_txNonBuffer((uint8_t*)line, len);

	len = sprintf (line, "INTFLAGSACLR = 0x%02x\r\n", USB.INTFLAGSACLR);
	uart_txNonBuffer((uint8_t*)line, len);

	len = sprintf (line, "INTFLAGSASET = 0x%02x\r\n", USB.INTFLAGSASET);
	uart_txNonBuffer((uint8_t*)line, len);

	len = sprintf (line, "INTFLAGSBCLR = 0x%02x\r\n", USB.INTFLAGSBCLR);
	uart_txNonBuffer((uint8_t*)line, len);

	len = sprintf (line, "INTFLAGSBSET = 0x%02x\r\n", USB.INTFLAGSBSET);
	uart_txNonBuffer((uint8_t*)line, len);

	len = sprintf (line, "CAL0         = 0x%02x\r\n", USB.CAL0);
	uart_txNonBuffer((uint8_t*)line, len);

	len = sprintf (line, "CAL1         = 0x%02x\r\n", USB.CAL1);
	uart_txNonBuffer((uint8_t*)line, len);

	len = sprintf (line, "sUsbEp       = 0x%02x\r\n", (uint16_t)sUsbEp);
	uart_txNonBuffer((uint8_t*)line, len);

	for (uint8_t i=0 ; i<16 ; ++i) {
		UsbEpPair *pEp = usb_get_ep_context(i);
		len = sprintf (line, "EP%02d = 0x%04x\r\n", i, (uint16_t)pEp);
		uart_txNonBuffer((uint8_t*)line, len);
	}

	_usb_dump_ep_regs(&(sUsbEp->EP0.out), "EP0OUT");
	_usb_dump_ep_regs(&(sUsbEp->EP0.in ), "EP0IN ");
	_usb_dump_ep_regs(&(sUsbEp->EP1.out), "EP1OUT");
	_usb_dump_ep_regs(&(sUsbEp->EP1.in ), "EP1IN ");


	_usb_dump_ep_data(&(sUsbEp->EP0.out), "EP0OUT");
	_usb_dump_ep_data(&(sUsbEp->EP0.in ), "EP0IN ");
	_usb_dump_ep_data(&(sUsbEp->EP1.out), "EP1OUT");
	_usb_dump_ep_data(&(sUsbEp->EP1.in ), "EP1IN ");
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
static void _usb_dump_ep_regs(USB_EP_t *ep, const char *label)
{
	len = sprintf (line, "%s REGS(0x%04x)  = %02x %02x %02x %02x %02x %02x %02x %02x\r\n",
					label,
					(uint16_t)ep,
					ep->STATUS,
					ep->CTRL,
					ep->CNTL,
					ep->CNTH,
					ep->DATAPTRL,
					ep->DATAPTRH,
					ep->AUXDATAL,
					ep->AUXDATAH);
	uart_txNonBuffer((uint8_t*)line, len);

	return;
}

/*---------------------------------------------------------------------------*/
static void _usb_dump_ep_data(USB_EP_t *ep, const char *label)
{
	uint8_t *pdata = (uint8_t *)ep->DATAPTR;
	
	len = sprintf (line, "%s: %02x %02x %02x %02x %02x %02x %02x %02x\r\n",
	label,
	pdata[0], pdata[1], pdata[2], pdata[3],
	pdata[4], pdata[5], pdata[6], pdata[7]);
	uart_txNonBuffer((uint8_t*)line, len);
	
	return;
}

#else /*USB_DEBUG*/

/*---------------------------------------------------------------------------*/
void usb_dump_regs(void)
{
	return;
}

#endif