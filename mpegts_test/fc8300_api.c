#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/kernel.h>

#include "fc8300_regs.h"
#include "fci_dal.h"
#include "fci_oal.h"
#include "fci_types.h"
#include "fc8300_api.h"


u8 broadcast_mode;
int mtv_drv_open(HANDLE *hDevice)
{
	int handle;

	handle = open("/dev/isdbt", O_RDWR | O_NDELAY);

	if (handle < 0) {
		printf("Cannot open handle : %d\n", handle);
		return BBM_NOK;
	}

	*hDevice = handle;

	return BBM_OK;
}

int mtv_drv_close(HANDLE hDevice)
{
	close(hDevice);

	return BBM_OK;
}

void mtv_power_on(HANDLE hDevice)
{
	BBM_POWER_ON(hDevice);
}

void mtv_power_off(HANDLE hDevice)
{
	BBM_POWER_OFF(hDevice);
}

int mtv_init(HANDLE hDevice, u32 product, u32 mode)
{
	int res = BBM_NOK;

	res = BBM_INIT(hDevice);
	broadcast_mode = (u8)mode;
	res |= BBM_TUNER_SELECT(hDevice, DIV_BROADCAST, product, mode);

	return res;
}

int mtv_deinit(HANDLE hDevice)
{
	int res = BBM_NOK;

	res = BBM_DEINIT(hDevice);

	return res;
}

int mtv_set_channel(HANDLE hDevice, u32 freq, u8 sub_channel_number)
{
	int res = BBM_NOK;

	res = BBM_TUNER_SET_FREQ(hDevice, DIV_BROADCAST, freq, sub_channel_number);

	return  res;
}

int mtv_lock_check(HANDLE hDevice)
{
	int res;

	res = BBM_SCAN_STATUS(hDevice, DIV_BROADCAST);

	return res;
}

int mtv_data_read(HANDLE hDevice, u8 *buf, u32 bufSize)
{
	s32 outSize;

	outSize = read(hDevice, buf, bufSize);

	return outSize;
}

void mtv_ts_start(HANDLE hDevice)
{
	BBM_TS_START(hDevice);
}

void mtv_ts_stop(HANDLE hDevice)
{
	BBM_TS_STOP(hDevice);
}

void mtv_monitor_mfd(HANDLE hDevice)
{
	u8 mfd_status;
	u8 mfd_value;
	static u8 master = 0;

	BBM_READ(hDevice, DIV_MASTER, 0x4064, &mfd_status);

	if (mfd_status & 0x01) {
		BBM_READ(hDevice, DIV_MASTER, 0x4065, &mfd_value);

		if (mfd_value < 12) {
			if (master != 1) {
				BBM_WRITE(hDevice, DIV_MASTER, 0x3022, 0x0b);
				BBM_WRITE(hDevice, DIV_MASTER, 0x255c, 0x10);
				BBM_WRITE(hDevice, DIV_MASTER, 0x2542, 0x04);

				master = 1;
			}
		} else {
			if (master != 2) {
				BBM_WRITE(hDevice, DIV_MASTER, 0x3022, 0x0f);
				BBM_WRITE(hDevice, DIV_MASTER, 0x255c, 0x3f);
				BBM_WRITE(hDevice, DIV_MASTER, 0x2542, 0x10);

				master = 2;
			}
		}
	}

#if defined(BBM_2_DIVERSITY) || defined(BBM_4_DIVERSITY)
	static u8 slave0 = 0;

	BBM_READ(hDevice, DIV_SLAVE0, 0x4064, &mfd_status);

	if (mfd_status & 0x01) {
		BBM_READ(hDevice, DIV_SLAVE0, 0x4065, &mfd_value);

		if (mfd_value < 12) {
			if (slave0 != 1) {
				BBM_WRITE(hDevice, DIV_SLAVE0, 0x3022, 0x0b);
				BBM_WRITE(hDevice, DIV_SLAVE0, 0x255c, 0x10);
				BBM_WRITE(hDevice, DIV_SLAVE0, 0x2542, 0x04);

				slave0 = 1;
			}
		} else {
			if (slave0 != 2) {
				BBM_WRITE(hDevice, DIV_SLAVE0, 0x3022, 0x0f);
				BBM_WRITE(hDevice, DIV_SLAVE0, 0x255c, 0x3f);
				BBM_WRITE(hDevice, DIV_SLAVE0, 0x2542, 0x10);

				slave0 = 2;
			}
		}
	}
#endif

#if defined(BBM_4_DIVERSITY)
	static u8 slave1 = 0;
	static u8 slave2 = 0;

	BBM_READ(hDevice, DIV_SLAVE1, 0x4064, &mfd_status);

	if (mfd_status & 0x01) {
		BBM_READ(hDevice, DIV_SLAVE1, 0x4065, &mfd_value);

		if (mfd_value < 12) {
			if (slave1 != 1) {
				BBM_WRITE(hDevice, DIV_SLAVE1, 0x3022, 0x0b);
				BBM_WRITE(hDevice, DIV_SLAVE1, 0x255c, 0x10);
				BBM_WRITE(hDevice, DIV_SLAVE1, 0x2542, 0x04);

				slave1 = 1;
			}
		} else {
			if (slave1 != 2) {
				BBM_WRITE(hDevice, DIV_SLAVE1, 0x3022, 0x0f);
				BBM_WRITE(hDevice, DIV_SLAVE1, 0x255c, 0x3f);
				BBM_WRITE(hDevice, DIV_SLAVE1, 0x2542, 0x10);

				slave1 = 2;
			}
		}
	}

	BBM_READ(hDevice, DIV_SLAVE2, 0x4064, &mfd_status);

	if (mfd_status & 0x01) {
		BBM_READ(hDevice, DIV_SLAVE2, 0x4065, &mfd_value);

		if (mfd_value < 12) {
			if (slave2 != 1) {
				BBM_WRITE(hDevice, DIV_SLAVE2, 0x3022, 0x0b);
				BBM_WRITE(hDevice, DIV_SLAVE2, 0x255c, 0x10);
				BBM_WRITE(hDevice, DIV_SLAVE2, 0x2542, 0x04);

				slave2 = 1;
			}
		} else {
			if (slave2 != 2) {
				BBM_WRITE(hDevice, DIV_SLAVE2, 0x3022, 0x0f);
				BBM_WRITE(hDevice, DIV_SLAVE2, 0x255c, 0x3f);
				BBM_WRITE(hDevice, DIV_SLAVE2, 0x2542, 0x10);

				slave2 = 2;
			}
		}
	}
#endif
}

int mtv_signal_quality_info(HANDLE hDevice, u8 *Lock, u8 *CN, u32 *ui32BER_A, u32 *ui32PER_A
	, u32 *ui32BER_B, u32 *ui32PER_B, u32 *ui32BER_C, u32 *ui32PER_C, s32 *i32RSSI)
{
	s32 res;
	u8 sync;
	u8 cn;
	struct dm_st {
		u8  start;
		s8  rssi;
		u8  sync_0;
		u8  sync_1;

		u8  fec_on;
		u8  fec_layer;
		u8  wscn;
		u8  reserved;

		u16 vit_a_ber_rxd_rsps;
		u16 vit_a_ber_err_rsps;
		u32 vit_a_ber_err_bits;

		u16 vit_b_ber_rxd_rsps;
		u16 vit_b_ber_err_rsps;
		u32 vit_b_ber_err_bits;

		u16 vit_c_ber_rxd_rsps;
		u16 vit_c_ber_err_rsps;
		u32 vit_c_ber_err_bits;

		u16 reserved0;
		u16 reserved1;
		u32 reserved2;

		u32 dmp_a_ber_rxd_bits;
		u32 dmp_a_ber_err_bits;

		u32 dmp_b_ber_rxd_bits;
		u32 dmp_b_ber_err_bits;

		u32 dmp_c_ber_rxd_bits;
		u32 dmp_c_ber_err_bits;

		u32 reserved3;
		u32 reserved4;
	} dm;

	res = BBM_BULK_READ(hDevice, DIV_BROADCAST, BBM_DM_DATA, (u8*) &dm, sizeof(dm));

	BBM_READ(hDevice, DIV_BROADCAST, 0x403d, &cn);
	BBM_TUNER_GET_RSSI(hDevice, DIV_BROADCAST, i32RSSI);

	dm.dmp_a_ber_err_bits = dm.dmp_a_ber_err_bits & 0x00ffffff;
	dm.dmp_b_ber_err_bits = dm.dmp_b_ber_err_bits & 0x00ffffff;
	dm.dmp_c_ber_err_bits = dm.dmp_c_ber_err_bits & 0x00ffffff;

	if (res)
		print_log(0, "mtv_signal_measure Error res : %d\n");

	BBM_READ(hDevice, DIV_BROADCAST, 0x50c5, &sync);

	if (sync)
		*Lock = 1;
	else
		*Lock = 0;

	if (dm.vit_a_ber_rxd_rsps)
		*ui32PER_A = ((double) dm.vit_a_ber_err_rsps / (double) dm.vit_a_ber_rxd_rsps) * 10000;
	else
		*ui32PER_A = 10000;

	if (dm.vit_b_ber_rxd_rsps)
		*ui32PER_B = ((double) dm.vit_b_ber_err_rsps / (double) dm.vit_b_ber_rxd_rsps) * 10000;
	else
		*ui32PER_B = 10000;

	if (dm.vit_c_ber_rxd_rsps)
		*ui32PER_C = ((double) dm.vit_c_ber_err_rsps / (double) dm.vit_c_ber_rxd_rsps) * 10000;
	else
		*ui32PER_C = 10000;

	if (dm.dmp_a_ber_rxd_bits)
		*ui32BER_A = ((double) dm.dmp_a_ber_err_bits / (double) dm.dmp_a_ber_rxd_bits) * 10000;
	else
		*ui32BER_A = 10000;

	if (dm.dmp_b_ber_rxd_bits)
		*ui32BER_B = ((double) dm.dmp_b_ber_err_bits / (double) dm.dmp_b_ber_rxd_bits) * 10000;
	else
		*ui32BER_B = 10000;

	if (dm.dmp_c_ber_rxd_bits)
		*ui32BER_C = ((double) dm.dmp_c_ber_err_bits / (double) dm.dmp_c_ber_rxd_bits) * 10000;
	else
		*ui32BER_C = 10000;

	*CN = cn / 4;

	mtv_monitor_mfd(hDevice);

	return BBM_OK;
}

