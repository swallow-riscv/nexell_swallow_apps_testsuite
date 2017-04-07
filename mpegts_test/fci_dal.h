#ifndef __FCI_DAL_H__
#define __FCI_DAL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "fci_types.h"

#define IOCTL_MAGIC     't'

typedef struct {
        uint32_t size;
        uint32_t buff[128];
} ioctl_info;

#define IOCTL_MAXNR                     25

#define IOCTL_ISDBT_RESET	\
	_IOW(IOCTL_MAGIC, 0, ioctl_info)
#define IOCTL_ISDBT_PROBE	\
	_IOW(IOCTL_MAGIC, 1, ioctl_info)
#define IOCTL_ISDBT_INIT	\
	_IO(IOCTL_MAGIC, 2)
#define IOCTL_ISDBT_DEINIT	\
	_IO(IOCTL_MAGIC, 3)

#define IOCTL_ISDBT_BYTE_READ	\
	_IOWR(IOCTL_MAGIC, 4, ioctl_info)
#define IOCTL_ISDBT_WORD_READ	\
	_IOWR(IOCTL_MAGIC, 5, ioctl_info)
#define IOCTL_ISDBT_LONG_READ	\
	_IOWR(IOCTL_MAGIC, 6, ioctl_info)
#define IOCTL_ISDBT_BULK_READ	\
	_IOWR(IOCTL_MAGIC, 7, ioctl_info)

#define IOCTL_ISDBT_BYTE_WRITE	\
	_IOW(IOCTL_MAGIC, 8, ioctl_info)
#define IOCTL_ISDBT_WORD_WRITE	\
	_IOW(IOCTL_MAGIC, 9, ioctl_info)
#define IOCTL_ISDBT_LONG_WRITE	\
	_IOW(IOCTL_MAGIC, 10, ioctl_info)
#define IOCTL_ISDBT_BULK_WRITE	\
	_IOW(IOCTL_MAGIC, 11, ioctl_info)

#define IOCTL_ISDBT_TUNER_READ	\
	_IOWR(IOCTL_MAGIC, 12, ioctl_info)
#define IOCTL_ISDBT_TUNER_WRITE	\
	_IOW(IOCTL_MAGIC, 13, ioctl_info)

#define IOCTL_ISDBT_TUNER_SET_FREQ	\
	_IOW(IOCTL_MAGIC, 14, ioctl_info)
#define IOCTL_ISDBT_TUNER_SELECT	\
	_IOW(IOCTL_MAGIC, 15, ioctl_info)
#define IOCTL_ISDBT_TUNER_DESELECT	\
	_IOW(IOCTL_MAGIC, 16, ioctl_info)

#define IOCTL_ISDBT_SCAN_STATUS	\
	_IOW(IOCTL_MAGIC, 17, ioctl_info)
#define IOCTL_ISDBT_TS_START	\
	_IO(IOCTL_MAGIC, 18)
#define IOCTL_ISDBT_TS_STOP	\
	_IO(IOCTL_MAGIC, 19)

#define IOCTL_ISDBT_TUNER_GET_RSSI	\
	_IOWR(IOCTL_MAGIC, 20, ioctl_info)

#define IOCTL_ISDBT_HOSTIF_SELECT	\
	_IOW(IOCTL_MAGIC, 21, ioctl_info)
#define IOCTL_ISDBT_HOSTIF_DESELECT	\
	_IO(IOCTL_MAGIC, 22)

#define IOCTL_ISDBT_POWER_ON	\
	_IO(IOCTL_MAGIC, 23)
#define IOCTL_ISDBT_POWER_OFF	\
	_IO(IOCTL_MAGIC, 24)


extern int BBM_RESET(HANDLE hDevice, DEVICEID devid);
extern int BBM_PROBE(HANDLE hDevice, DEVICEID devid);
extern int BBM_INIT(HANDLE hDevice);
extern int BBM_DEINIT(HANDLE hDevice);

extern int BBM_READ(HANDLE hDevice, DEVICEID devid, u16 addr, u8 *data);
extern int BBM_BYTE_READ(HANDLE hDevice, DEVICEID devid, u16 addr, u8 *data);
extern int BBM_WORD_READ(HANDLE hDevice, DEVICEID devid, u16 addr, u16 *data);
extern int BBM_LONG_READ(HANDLE hDevice, DEVICEID devid, u16 addr, u32 *data);
extern int BBM_BULK_READ(HANDLE hDevice, DEVICEID devid, u16 addr, u8 *data, u16 size);

extern int BBM_WRITE(HANDLE hDevice, DEVICEID devid, u16 addr, u8 data);
extern int BBM_BYTE_WRITE(HANDLE hDevice, DEVICEID devid, u16 addr, u8 data);
extern int BBM_WORD_WRITE(HANDLE hDevice, DEVICEID devid, u16 addr, u16 data);
extern int BBM_LONG_WRITE(HANDLE hDevice, DEVICEID devid, u16 addr, u32 data);
extern int BBM_BULK_WRITE(HANDLE hDevice, DEVICEID devid, u16 addr, u8 *data, u16 size);

extern int BBM_TUNER_READ(HANDLE hDevice, DEVICEID devid, u8 addr, u8 alen, u8 *buffer, u8 len);
extern int BBM_TUNER_WRITE(HANDLE hDevice, DEVICEID devid, u8 addr, u8 alen, u8 *buffer, u8 len);
extern int BBM_TUNER_SET_FREQ(HANDLE hDevice, DEVICEID devid, u32 freq, u8 subch);
extern int BBM_TUNER_SELECT(HANDLE hDevice, DEVICEID devid, u32 product, u32 band);
extern int BBM_TUNER_DESELECT(HANDLE hDevice, DEVICEID devid);
extern int BBM_TUNER_GET_RSSI(HANDLE hDevice, DEVICEID devid, s32 *rssi);

extern int BBM_HOSTIF_SELECT(HANDLE hDevice, u8 hostif);
extern int BBM_HOSTIF_DESELECT(HANDLE hDevice);

extern int BBM_POWER_ON(HANDLE hDevice);
extern int BBM_POWER_OFF(HANDLE hDevice);

extern int BBM_TS_START(HANDLE hDevice);
extern int BBM_TS_STOP(HANDLE hDevice);

extern int BBM_SCAN_STATUS(HANDLE hDevice, DEVICEID devid);

#ifdef __cplusplus
}
#endif

#endif /* __FCI_DAL_H__ */

