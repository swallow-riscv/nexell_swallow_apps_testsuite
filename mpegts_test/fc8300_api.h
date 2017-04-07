/*****************************************************************************
 Copyright(c) 2012 FCI Inc. All Rights Reserved

 File name : fc8300_api.h

 Description :

*******************************************************************************/

#ifndef __FC8300_API_H__
#define __FC8300_API_H__

#ifdef __cplusplus
extern "C" {
#endif
extern int mtv_drv_open(HANDLE *hDevice);
extern int mtv_drv_close(HANDLE hDevice);
extern void mtv_power_on(HANDLE hDevice);
extern void mtv_power_off(HANDLE hDevice);
extern int mtv_init(HANDLE hDevice, u32 product, u32 mode);
extern int mtv_set_channel(HANDLE hDevice, u32 freq, u8 sub_channel_number);
extern int mtv_lock_check(HANDLE hDevice);
extern int mtv_data_read(HANDLE hDevice, u8 *buf, u32 bufSize);
extern void mtv_ts_start(HANDLE hDevice);
extern void mtv_ts_stop(HANDLE hDevice);
extern int mtv_signal_quality_info(HANDLE hDevice, u8 *Lock, u8 *CN, u32 *ui32BER_A, u32 *ui32PER_A
	, u32 *ui32BER_B, u32 *ui32PER_B, u32 *ui32BER_C, u32 *ui32PER_C, s32 *i32RSSI);
extern int mtv_deinit(HANDLE hDevice);

#ifdef __cplusplus
}
#endif

#endif 		// __FC8300_API_H__
