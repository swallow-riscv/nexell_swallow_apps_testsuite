#include <string.h>

#include <sys/ioctl.h>

#include "fci_types.h"
#include "fci_dal.h"

#define DEVICE_FILENAME	"/dev/isdbt"

int BBM_RESET(HANDLE hDevice, DEVICEID devid)
{
	ioctl_info info;

	info.buff[0] = devid;

	ioctl(hDevice, IOCTL_ISDBT_RESET, &info);

	return BBM_OK;
}

int BBM_PROBE(HANDLE hDevice, DEVICEID devid)
{
	int res;
	ioctl_info info;

	info.buff[0] = devid;

	res = ioctl(hDevice, IOCTL_ISDBT_PROBE, &info);

	return res;
}

int BBM_INIT(HANDLE hDevice)
{
	int res;

	res = ioctl(hDevice, IOCTL_ISDBT_INIT);

	return res;
}

int BBM_DEINIT(HANDLE hDevice)
{
	int res;

	res = ioctl(hDevice, IOCTL_ISDBT_DEINIT);

	return res;
}

int BBM_READ(HANDLE hDevice, DEVICEID devid, u16 addr, u8 *data)
{
	int res;
	ioctl_info info;

	info.buff[0] = addr;
	info.buff[1] = devid;

	res = ioctl(hDevice, IOCTL_ISDBT_BYTE_READ, &info);

	*data = info.buff[2];

	return res;
}

int BBM_BYTE_READ(HANDLE hDevice, DEVICEID devid, u16 addr, u8 *data)
{
	int res;
	ioctl_info info;

	info.buff[0] = addr;
	info.buff[1] = devid;

	res = ioctl(hDevice, IOCTL_ISDBT_BYTE_READ, &info);

	*data = info.buff[2];

	return res;
}

int BBM_WORD_READ(HANDLE hDevice, DEVICEID devid, u16 addr, u16 *data)
{
	int res;
	ioctl_info info;

	info.buff[0] = addr;
	info.buff[1] = devid;

	res = ioctl(hDevice, IOCTL_ISDBT_WORD_READ, &info);

	*data = info.buff[2];

	return res;
}

int BBM_LONG_READ(HANDLE hDevice, DEVICEID devid, u16 addr, u32 *data)
{
	int res;
	ioctl_info info;

	info.buff[0] = addr;
	info.buff[1] = devid;

	res = ioctl(hDevice, IOCTL_ISDBT_LONG_READ, &info);

	*data = info.buff[2];

	return res;
}

int BBM_BULK_READ(HANDLE hDevice, DEVICEID devid, u16 addr, u8 *data, u16 size)
{
	int res;
	ioctl_info info;

	info.buff[0] = addr;
	info.buff[1] = devid;
	info.buff[2] = size;

	res = ioctl(hDevice, IOCTL_ISDBT_BULK_READ, &info);

	memcpy((void*)data, (void*)&info.buff[3], size);

	return res;
}

int BBM_WRITE(HANDLE hDevice, DEVICEID devid, u16 addr, u8 data)
{
	int res;
	ioctl_info info;

	info.buff[0] = addr;
	info.buff[1] = data;
	info.buff[2] = devid;

	res = ioctl(hDevice, IOCTL_ISDBT_BYTE_WRITE, &info);

	return res;
}

int BBM_BYTE_WRITE(HANDLE hDevice, DEVICEID devid, u16 addr, u8 data)
{
	int res;
	ioctl_info info;

	info.buff[0] = addr;
	info.buff[1] = data;
	info.buff[2] = devid;

	res = ioctl(hDevice, IOCTL_ISDBT_BYTE_WRITE, &info);

	return res;
}

int BBM_WORD_WRITE(HANDLE hDevice, DEVICEID devid, u16 addr, u16 data)
{
	int res;
	ioctl_info info;

	info.buff[0] = addr;
	info.buff[1] = data;
	info.buff[2] = devid;

	res = ioctl(hDevice, IOCTL_ISDBT_WORD_WRITE, &info);

	return res;
}

int BBM_LONG_WRITE(HANDLE hDevice, DEVICEID devid, u16 addr, u32 data)
{
	int res;
	ioctl_info info;

	info.buff[0] = addr;
	info.buff[1] = data;
	info.buff[2] = devid;

	res = ioctl(hDevice, IOCTL_ISDBT_LONG_WRITE, &info);

	return res;
}

int BBM_BULK_WRITE(HANDLE hDevice, DEVICEID devid, u16 addr, u8 *data, u16 size)
{
	int res;
	ioctl_info info;

	info.buff[0] = addr;
	info.buff[1] = size;
	info.buff[2] = devid;
	memcpy((void*)&info.buff[3], data, size);

	res = ioctl(hDevice, IOCTL_ISDBT_BULK_WRITE, &info);

	return res;
}

int BBM_TUNER_READ(HANDLE hDevice, DEVICEID devid, u8 addr, u8 alen, u8 *buffer, u8 len)
{
	int res;
	ioctl_info info;

	info.buff[0] = addr;
	info.buff[1] = alen;
	info.buff[2] = len;
	info.buff[3] = devid;

	res = ioctl(hDevice, IOCTL_ISDBT_TUNER_READ, &info);

	memcpy((void*)buffer, (void*)&info.buff[4], len);

	return res;
}

int BBM_TUNER_WRITE(HANDLE hDevice, DEVICEID devid, u8 addr, u8 alen, u8 *buffer, u8 len)
{
	int res;
	ioctl_info info;

	info.buff[0] = addr;
	info.buff[1] = alen;
	info.buff[2] = len;
	info.buff[3] = devid;

	memcpy((void*)&info.buff[4], buffer, len);

	res = ioctl(hDevice, IOCTL_ISDBT_TUNER_WRITE, &info);

	return res;
}

int BBM_TUNER_SET_FREQ(HANDLE hDevice, DEVICEID devid, u32 freq, u8 subch)
{
	int res;
	ioctl_info info;

	info.buff[0] = freq;
	info.buff[1] = subch;
	info.buff[2] = devid;

	res = ioctl(hDevice, IOCTL_ISDBT_TUNER_SET_FREQ, &info);

	return res;
}

int BBM_SCAN_STATUS(HANDLE hDevice, DEVICEID devid)
{
	int res;
	ioctl_info info;

	info.buff[0] = devid;

	res = ioctl(hDevice, IOCTL_ISDBT_SCAN_STATUS, &info);

	return res;
}


int BBM_TUNER_SELECT(HANDLE hDevice, DEVICEID devid, u32 product, u32 band)
{
	int res;
	ioctl_info info;

	info.buff[0] = product;
	info.buff[1] = band;
	info.buff[2] = devid;

	res = ioctl(hDevice, IOCTL_ISDBT_TUNER_SELECT, &info);

	return res;
}

int BBM_TUNER_DESELECT(HANDLE hDevice, DEVICEID devid)
{
	int res;
	ioctl_info info;

	info.buff[0] = devid;

	res = ioctl(hDevice, IOCTL_ISDBT_TUNER_DESELECT, &info);

	return res;
}

int BBM_TUNER_GET_RSSI(HANDLE hDevice, DEVICEID devid, s32* rssi)
{
	int res;
	ioctl_info info;

	info.buff[0] = devid;

	res = ioctl(hDevice, IOCTL_ISDBT_TUNER_GET_RSSI, &info);

	*rssi = info.buff[1];

	return res;
}

int BBM_POWER_ON(HANDLE hDevice)
{
	int res;

	res = ioctl(hDevice, IOCTL_ISDBT_POWER_ON);

	return res;
}

int BBM_POWER_OFF(HANDLE hDevice)
{
	int res;

	res = ioctl(hDevice, IOCTL_ISDBT_POWER_OFF);

	return res;
}

int BBM_TS_START(HANDLE hDevice)
{
	int res;

	res = ioctl(hDevice, IOCTL_ISDBT_TS_START);

	return res;
}

int BBM_TS_STOP(HANDLE hDevice)
{
	int res;

	res = ioctl(hDevice, IOCTL_ISDBT_TS_STOP);

	return res;
}

