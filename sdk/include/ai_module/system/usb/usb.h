/*
 * Copyright (c) 2017 Quantum.inc. All rights reserved.
 * This software is released under the MIT License, see LICENSE.txt.
 */

#ifndef USB_H_
#define USB_H_


/*--------------------------------------------------------------------------*/
// Error Code
/*--------------------------------------------------------------------------*/
#define USB_OK									(0)
#define USB_ERROR_INVALID_EP					(-1)
#define USB_ERROR_NOBUF							(-2)
#define USB_ERROR_NULL							(-3)
#define USB_ERROR_NOT_FOUND						(-4)
#define USB_ERROR_NOMEM							(-5)


/*---------------------------------------------------------------------------*/
//J Function Switch
/*---------------------------------------------------------------------------*/
#define USB_DEBUG								(0)

/*---------------------------------------------------------------------------*/
//J USB Device Request の定義
/*---------------------------------------------------------------------------*/
typedef struct UsbDeviceRequest_t
{
	union {
		uint8_t byte;
		struct {
			uint8_t target    : 5;
			uint8_t type      : 2;
			uint8_t direction : 1;
		}bm;
	} bmRequestType;
	uint8_t bRequest;
	uint16_t wValue;
	uint16_t wIndex;
	uint16_t wLength;
} UsbDeviceRequest;

#define USB_OUT								(0)
#define USB_IN								(1)

#define UsbBmRequestDirHostToDevice			(0)
#define UsbBmRequestDirDeviceToHost			(1)

#define UsbBmRequestTypeStandard			(0)
#define UsbBmRequestTypeClass				(1)
#define UsbBmRequestTypeVender				(2)

#define UsbBmRequestTargetDevice			(0)
#define UsbBmRequestTargetInterface			(1)
#define UsbBmRequestTargetEndpoint			(2)
#define UsbBmRequestTargetOther				(3)

/*--------------------------------------------------------------------------*/
// Endpoint Register 領域
/*--------------------------------------------------------------------------*/
typedef struct UsbEpPair_t
{
	USB_EP_t out;
	USB_EP_t in;
} UsbEpPair;


/*--------------------------------------------------------------------------*/
// USBの各種転送完了時のコールバック関数類
/*--------------------------------------------------------------------------*/
//J Control 転送が入った後の処理を記述
typedef void (*UsbControlTansferCallback)(UsbDeviceRequest *req);

//J Transfer 割り込みが入ったときの処理をDispatchする関数ポインタ
typedef void (*UsbTranferDoneCb)(uint8_t ep_num, UsbEpPair *ep);

//J 平常時に全力Updateされる処理
typedef void (*UsbUpdateCb)(void);


/*--------------------------------------------------------------------------*/
// USB Transfer
/*--------------------------------------------------------------------------*/
UsbEpPair *usb_get_ep_context(int8_t epnum);

void usb_tranfer_trigger_status_stage(void);
void usb_transfer_status_stage(UsbEpPair *ep, uint8_t in);
int usb_transfer_bulk_in(UsbEpPair *ep, void *buf, size_t len);
int usb_transfer_bulk_out(UsbEpPair *ep);

int usb_ep_stall (UsbEpPair *ep, uint8_t in, uint8_t stall);
int usb_ep_nack(UsbEpPair *ep, uint8_t in, uint8_t nack);
int usb_ep_ready(UsbEpPair *ep, uint8_t in);

int initialize_usb(uint8_t full_function);
int usb_poll_status(void);

/*--------------------------------------------------------------------------*/
// Driver Install
/*--------------------------------------------------------------------------*/
int usb_register_device_descriptor(const uint8_t *descriptor, uint16_t descriptor_len);
int usb_register_class_control_transer_callback(UsbControlTansferCallback cb);
int usb_register_vender_control_transer_callback(UsbControlTansferCallback cb);
int usb_register_ep_transfer_done_callback(uint8_t ep_num, uint8_t in, UsbTranferDoneCb cb);
int usb_register_update_callback(uint8_t ep_num, UsbUpdateCb cb);


/*--------------------------------------------------------------------------*/
// Device State Control
/*--------------------------------------------------------------------------*/
typedef enum USB_DEVICE_STATE_t
{
	USB_DEVICE_STATE_WAIT_BUS_RESET,
	USB_DEVICE_STATE_CONNECTED,
	USB_DEVICE_STATE_RESET_WHEN_BUS_CONNECTED,
	USB_DEVICE_STATE_PEND_CPU_RESET,
	USB_DEVICE_STATE_WAIT_CPU_RESET
} USB_DEVICE_STATE;

USB_DEVICE_STATE usb_get_device_state(void);
int8_t usb_set_device_state(USB_DEVICE_STATE state);

/*--------------------------------------------------------------------------*/
// Device State Control
/*--------------------------------------------------------------------------*/
void usb_dump_regs(void);

#endif /* USB_H_ */