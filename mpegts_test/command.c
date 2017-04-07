#include <stdlib.h>

#include "monitor.h"
#include "fci_types.h"
#include "monitor.h"
#include "clib.h"
#include "fci_dal.h"
#include "fci_oal.h"
#include "fci_dataread.h"
#include "fc8300_api.h"

static int hDevice = 0;
u8 sig_start;
u8 dump_start;
u16 devid = DIV_BROADCAST;

void mmi_devset_cmd(int argc, char *argv[])
{
	if (argc == 2) {
		devid = htoi(argv[1]);
	} else
		devid = DIV_BROADCAST;
}

void mmi_bbm_read(int argc, char *argv[])
{
	u16 addr;
	u16  length;
	u8  data;
	int i;

	if(argc == 2) {
		addr = htoi(argv[1]);
		BBM_READ(hDevice, devid, addr, &data);

		print_log("[0x%04X] : 0x%02X\n", addr, data);
	} else if(argc == 3) {
		addr = htoi(argv[1]);
		length    = htoi(argv[2]);

		for(i=0; i<length; i++)
		{
			if((i % 8) == 0) print_log("\n[0x%04X] : ", addr+i);
			BBM_READ(hDevice, devid, addr+i, &data);
			print_log("%02X ", data & 0xFF);
		}
		print_log("\n");
	} else {
		print_log("usage : %s [start addr] [length] ; byte read command\n", argv[0]);
	}
}

void mmi_bbm_write(int argc, char *argv[])
{
	u16  dest_addr;
	u8   dest_data;
	u8   length;
	int  i;


	if(argc == 3) {
		dest_addr = htoi(argv[1]);
		dest_data = htoi(argv[2]);

		BBM_WRITE(hDevice, devid, dest_addr, dest_data);
	} else if(argc == 4) {
		dest_addr = htoi(argv[1]);
		dest_data = htoi(argv[2]);
		length    = htoi(argv[3]);

		if(dest_data == 0x1234) {
			dest_data = 0;
			for(i=0; i<=length; i++)
				BBM_WRITE(hDevice, devid, dest_addr+i, dest_data);
		} else {
			for(i=0; i<length; i++)
				BBM_WRITE(hDevice, devid, dest_addr+i, dest_data);
		}
	} else {
		print_log("usage : %s [start addr] [data] [length] ; byte write command\n", argv[0]);
	}
}

void mmi_bbm_wread(int argc, char *argv[])
{
	u16  length;
	u16 dest_addr, target_addr;
	u16 data;
	int i;

	if(argc == 2) {
		dest_addr = htoi(argv[1]);
		BBM_WORD_READ(hDevice, devid, dest_addr, &data);
		print_log("[0x%04X] : 0x%04X\n", dest_addr, data);
	} else if(argc == 3) {
		dest_addr = htoi(argv[1]);
		length    = htoi(argv[2]);

		for(i=0; i<length; i+=2)
		{
			target_addr = dest_addr + i;
			if((i % 4) == 0) print_log("\n[0x%04X] : ", target_addr);
			BBM_WORD_READ(hDevice, devid, target_addr, &data);
			print_log("%04X\n", data);
		}
		print_log("\n");
	} else {
		print_log("usage : %s [start addr] [length] ; word read command\n", argv[0]);
	}
}

void mmi_bbm_wwrite(int argc, char *argv[])
{
	u16  dest_addr;
	u16   dest_data;
	u16   length;
	int  i;

	if(argc == 3) {
		dest_addr = htoi(argv[1]);
		dest_data = htoi(argv[2]);

		BBM_WORD_WRITE(hDevice, devid, dest_addr, dest_data);
	} else if(argc == 4) {
		dest_addr = htoi(argv[1]);
		dest_data = htoi(argv[2]);
		length    = htoi(argv[3]);

		for(i=0; i<length; i+=2) {
			BBM_WORD_WRITE(hDevice, devid, dest_addr+i, dest_data);
		}
	} else {
		print_log("usage : %s [start addr] [data] [length] ; word write command\n", argv[0]);
	}
}

void mmi_bbm_lread(int argc, char *argv[])
{
	u16  length;
	u16 dest_addr, target_addr;
	u32 data;
	int i;

	if(argc == 2) {
		dest_addr = htoi(argv[1]);
		BBM_LONG_READ(hDevice, devid, dest_addr, &data);
		print_log("[0x%04X] : 0x%08X\n", dest_addr, data);
	} else if(argc == 3) {
		dest_addr = htoi(argv[1]);
		length    = htoi(argv[2]);

		for(i=0; i<length; i+=4) {
			target_addr = dest_addr + i;
			if((i % 2) == 0) print_log("\n[0x%04X] : ", target_addr);
			BBM_LONG_READ(hDevice, devid, target_addr, &data);
			print_log("%08X\n", data);
		}
		print_log("\n");
	} else {
		print_log("usage : %s [start addr] [length] ; long read command\n", argv[0]);
	}
}

void mmi_bbm_lwrite(int argc, char *argv[])
{
	u16  dest_addr;
	u32   dest_data;
	u16   length;
	int  i;

	if(argc == 3) {
		dest_addr = htoi(argv[1]);
		dest_data = htoi(argv[2]);

		BBM_LONG_WRITE(hDevice, devid, dest_addr, dest_data);
	} else if(argc == 4) {
		dest_addr = htoi(argv[1]);
		dest_data = htoi(argv[2]);
		length    = htoi(argv[3]);

		for(i=0; i<length; i+=4) {
			BBM_LONG_WRITE(hDevice, devid, dest_addr+i, dest_data);
		}
	} else {
		print_log("usage : %s [start addr] [data] [length] ; long write command\n", argv[0]);
	}
}

void mmi_bbm_verify(int argc, char *argv[])
{
	u8  dest_addr;
	u8  dest_data, temp_data;
	int  i;
	int  retry;

	switch(argc) {
		case 2:
			dest_data = 0xAA;
			dest_addr = htoi(argv[1]);

			print_log("%s [0x%X] [0x%X] [0x%X]\n", argv[0], dest_addr, dest_data, 1);

			BBM_WRITE(hDevice, devid, dest_addr, dest_data);

			//Sleep(10);

			BBM_READ(hDevice, devid, dest_addr, &temp_data);

			if(dest_data != temp_data) {
				print_log("Mismatch Data ;  addr[0x%X] write [0x%X] : read [0x%X]\n",
						dest_addr, dest_data, temp_data);
			}
			break;
		case 3:
			dest_data = 0xff;
			dest_addr = htoi(argv[1]);
			retry     = htoi(argv[2]);

			print_log("%s [0x%X] [0x%X]\n", argv[0], dest_addr, retry);

			for(i=0; i<retry; i++) {
				BBM_WRITE(hDevice, devid, dest_addr, dest_data);

				//Sleep(10);

				BBM_READ(hDevice, devid, dest_addr, &temp_data);
				if(dest_data != temp_data) {
					print_log("\n 0x%xth Mismatch Data ;  addr[0x%X] write [0x%X] : read [0x%X]\n",
							i, dest_addr, dest_data, temp_data);
				}
				dest_data--;
				print_log(".");
			}
			break;
		default:
			print_log("Usage : %s [address] [retry]\n", argv[0]);
			break;
	}
}

void mmi_bbm_i2c_verify(int argc, char *argv[])
{
	u8  dest_addr;
	u8  dest_data, temp_data;
	int  retry;
	int  i;

	switch(argc) {
		case 3:
			dest_addr = htoi(argv[1]);
			retry     = htoi(argv[2]);

			dest_data = 0;
			for(i=0; i<retry; i++) {
				dest_data++;

				BBM_TUNER_WRITE(hDevice, devid, dest_addr, 1, &dest_data, 1);
				BBM_TUNER_READ(hDevice, devid, dest_addr, 1, &temp_data, 1);

				if(dest_data != temp_data) {
					print_log("\n0x%Xth Mismatch Data ;  addr[0x%X] write [0x%X] : read [0x%X]\n",
							i, dest_addr, dest_data, temp_data);
				}
				print_log(".");
			}
			break;
		default:
			print_log("Usage : %s [address] [retry] [delay]\n", argv[0]);
			break;
	}
}

void mmi_tuner_set(int argc, char *argv[])
{
	int res;
	u32 freq = 0;
	u8 subch = 0;

	switch (argc)
	{

		case 2:
			freq = dtoi(argv[1]);

			print_log("Tuner is set channel freq = %d\n", freq);
			mtv_set_channel(hDevice, freq, 0x16);

			res = mtv_lock_check(hDevice);
			if(res)
			{
				print_log("Lock Fail\n");
				//continue;
			}
			else
				print_log("Lock OK \n");

			break;

		case 3:
			freq = dtoi(argv[1]);
			subch = dtoi(argv[2]);

			print_log("Tuner is set channel freq = %d, subch : %d\n", freq, subch);
			mtv_set_channel(hDevice, freq, subch);

			res = mtv_lock_check(hDevice);
			if(res)
			{
				print_log("Lock Fail\n");
				//continue;
			}
			else
				print_log("Lock OK \n");

			break;

		default:
			print_log("Usage : %s [u1|ytn|kdmb|mbc|kbs|sbs]\n", argv[0]);
			break;
	}
}

void mmi_bbm_reset_cmd(int argc, char *argv[])
{
	switch(argc) {
		case 1:
			BBM_RESET(hDevice, devid);
			break;
		default:
			print_log("Usage : %s [cpu|bbm]\n", argv[0]);
			break;
	}
}

void mmi_bbm_init_cmd(int argc, char *argv[])
{
	int res = BBM_NOK;
	u32 product, mode;

	res = mtv_drv_open(&hDevice);

	mtv_power_on(hDevice);

	if(argc == 3) {
		product = dtoi(argv[1]);
		mode = dtoi(argv[2]);
	} else if(argc == 2) {
		product = dtoi(argv[1]);
		mode = ISDBT_13SEG;
	}
	else {
		product = 8300;
		mode = ISDBT_13SEG;
	}
#if 1
	res |= mtv_init(hDevice, product, mode);
#endif

	print_log("Init result : %s product : %s, mode : %d\n"
		, (res)?"Fail":"Success"
		, (product==8300)?"FC8300_CSP":"FC8300_BGA"
		, mode);
}

void mmi_bbm_deinit_cmd(int argc, char *argv[])
{
	int res = BBM_NOK;

	mtv_power_off(hDevice);

	res |= mtv_deinit(hDevice);

	print_log("DeInit result : %d\n", res);
}

void mmi_i2c_read_cmd(int argc, char *argv[])
{
	u8  addr;
	u8  data[256], length;
	u8  tmp;
	int i;

	if(argc == 2) {
		data[0] = 0;
		addr = htoi(argv[1]);

		BBM_TUNER_READ(hDevice, devid, addr, 1, &data[0], 1);

		print_log("[0x%08X] : 0x%02X\n", addr, data[0]);
	} else if(argc == 3) {

		addr = htoi(argv[1]);
		length    = htoi(argv[2]);

		for(i=0; i<length; i++) {
			tmp = addr+i;
			BBM_TUNER_READ(hDevice, devid, tmp, 1, &data[i], 1);
		}

		for(i=0; i<length; i++) {
			tmp = addr+i;
			if((i % 8) == 0) print_log("\n[0x%08X] : ", tmp);
			print_log("%02X ", data[i] & 0xFF);
		}
		print_log("\n");
	} else {
		print_log("Usage : %s [start addr] [length]\n", argv[0]);
	}
}

void mmi_i2c_write_cmd(int argc, char *argv[])
{
	u8  dest_addr;
	u8   dest_data, length;
	int  i;

	if(argc == 3) {
		dest_addr = htoi(argv[1]);
		dest_data = htoi(argv[2]);

		BBM_TUNER_WRITE(hDevice, devid, dest_addr, 1, &dest_data, 1);
	} else if(argc == 4) {
		dest_addr = htoi(argv[1]);
		dest_data = htoi(argv[2]);
		length    = htoi(argv[3]);

		if(dest_data == 0x1234) {
			dest_data = 0;
			for(i=0; i<=length; i++)
				BBM_TUNER_WRITE(hDevice, devid, dest_addr+i, 1, &dest_data, 1);
		} else {
			for(i=0; i<length; i++)
				BBM_TUNER_WRITE(hDevice, devid, dest_addr+i, 1, &dest_data, 1);
		}
	} else {
		print_log("Usage : %s [start addr] [data] [length]\n", argv[0]);
	}
}

void mmi_scan_cmd(int argc, char *argv[])
{
	int res = BBM_NOK;
	int i = 0;
	u32 f_rf;

	for(i = 13; i <= 63; i++)
	{
		print_log("Channel number : %d \n", i);

		f_rf = (i - 13) * 6000 + 473143;
		mtv_set_channel(hDevice, f_rf, 0x16);
		res = mtv_lock_check(hDevice);
		if(res)
		{
			print_log("Lock Fail\n");
			//continue;
		}
		else
			print_log("Lock OK \n");
	}
}

void mmi_dm_cmd(int argc, char *argv[])
{
	u8 cn, lock;
	u32 berA, perA, berB, perB, berC, perC;
	s32 i32RSSI;

	mtv_signal_quality_info(hDevice, &lock, &cn, &berA, &perA, &berB, &perB, &berC, &perC, &i32RSSI);

	print_log("Lock : 0x%x CN : %3d, RSSI : %3d\n",lock, cn, i32RSSI);
	print_log("BerA : %6d, PerA : %6d, BerB : %6d, PerB : %6d, BerC : %6d, PerC : %6d, \n", berA, perA, berB, perB, berC, perC);


}

void mmi_data_dump_cmd(int argc, char *argv[])
{
	if (dump_start) {
		data_dump_stop(hDevice);
		dump_start = 0;
	} else {
		data_dump_start(hDevice);
		dump_start = 1;
	}
}

void mmi_sig_cmd(int argc, char *argv[])
{
	if (sig_start) {
		data_sig_stop(hDevice);
		sig_start = 0;
	} else {
		data_sig_start(hDevice);
		sig_start = 1;
	}
}

u8 ptchk_start;
void mmi_ptcheck_cmd(int argc, char *argv[])
{
	u16 gap;
	u32 bps;
	int size = 0;
	int ch_num;

	print_log("ptchk_start : %d\n", ptchk_start);

	if (ptchk_start) {
		BBM_WORD_WRITE(hDevice, devid, 0xa0, 0);
		ptcheck_stop(hDevice);
		ptchk_start = 0;
	} else {
		if(argc >= 2) {
			bps = dtoi(argv[1]);
			if (bps > 52000000)
				bps = 52000000;
			gap = 1000000/(bps/(188*8))*6.2 + 0x8000;

			print_log("hDevice : %d\n", hDevice);
			BBM_WORD_WRITE(hDevice, devid, 0xa0, gap);
			if (argc >= 3)
				size = dtoi(argv[2]);
			else
				size = 100;

			if (argc >= 4)
				ch_num = dtoi(argv[3]);
			else
				ch_num = 1;

		}
		else {
			gap = 0x8200;
			bps = 18000000;
			size = 100;
			BBM_WORD_WRITE(hDevice, devid, 0xa0, gap);
			ch_num = 1;
		}
		print_log("Pattern check Set : 0x%x, bps : %d, dumpsize : %dMB\n", gap, bps, size);

		ptcheck_start(hDevice, size, ch_num);
		ptchk_start = 1;
	}
}

void mmi_exit_cmd(int argc, char *argv[])
{
	mtv_power_off(hDevice);
	mtv_drv_close(hDevice);
	exit(0);
}

void anal_command(int argc, char *argv[])
{
	if (iscmd("he"))                 Mon_help_command();
	else if(iscmd("?"))             Mon_help_command();
	else if(iscmd("help"))          Mon_help_command();

	else if(iscmd("brd"))		mmi_bbm_read(argc, argv);
	else if(iscmd("bwr"))		mmi_bbm_write(argc, argv);
	else if(iscmd("wrd"))		mmi_bbm_wread(argc, argv);
	else if(iscmd("wwr"))		mmi_bbm_wwrite(argc, argv);
	else if(iscmd("lrd"))		mmi_bbm_lread(argc, argv);
	else if(iscmd("lwr"))		mmi_bbm_lwrite(argc, argv);

	else if(iscmd("verify"))	mmi_bbm_verify(argc, argv);
	else if(iscmd("iverify"))	mmi_bbm_i2c_verify(argc, argv);

	else if(iscmd("tuner"))		mmi_tuner_set(argc, argv);

	else if(iscmd("reset"))		mmi_bbm_reset_cmd(argc, argv);
	else if(iscmd("init"))		mmi_bbm_init_cmd(argc, argv);
	else if(iscmd("deinit"))	mmi_bbm_deinit_cmd(argc, argv);

	else if(iscmd("ird"))		mmi_i2c_read_cmd(argc, argv);
	else if(iscmd("iwr"))		mmi_i2c_write_cmd(argc, argv);

	else if(iscmd("scan"))		mmi_scan_cmd(argc, argv);
	else if(iscmd("dm"))		mmi_dm_cmd(argc, argv);
	else if(iscmd("sig"))		mmi_sig_cmd(argc, argv);
	else if(iscmd("ptcheck"))		mmi_ptcheck_cmd(argc, argv);
	else if(iscmd("data_dump"))	mmi_data_dump_cmd(argc, argv);
	else if(iscmd("devset"))	mmi_devset_cmd(argc, argv);
	else if(iscmd("exit"))		mmi_exit_cmd(argc, argv);

	else print_log("\nNo such command - type help\n");
}
