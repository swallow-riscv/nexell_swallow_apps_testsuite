#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>

#include <linux/videodev2.h>
#include <nx-v4l2.h>

#include "mpegts.h"

#include "fci_types.h"
#include "fci_oal.h"
#include "fci_dataread.h"
#include "fc8300_api.h"

int f_index;
static pthread_t p_thr_data;
static pthread_t p_thr_sig;
static pthread_t ptcheck_thr_sig;
//#define MSC_BUF_SIZE 128*1024
//#define MSC_BUF_SIZE PAGE_ALIGN(TS_PAGE_SIZE)	/*	(80 * 188)	*/
#define MSC_BUF_SIZE (TS_PAGE_SIZE)	/*	(80 * 188)	*/
#define DUMP_PATH "/mnt/"

typedef struct _data_dump_param{
	int ch_num;
	int num;
	int dev;
} data_dump_param;

#define FEATURE_TS_CHECK
#define FEATURE_OVERRUN_CHECK

#ifdef FEATURE_TS_CHECK
#define MAX_DEMUX           2

#define V4L2_CID_MPEGTS_PAGE_SIZE       (V4L2_CTRL_CLASS_USER | 0x1001)
#define MAX_BUFFER_COUNT 32

/*
 * Sync Byte 0xb8
 */
#define SYNC_BYTE_INVERSION

struct pid_info {
	unsigned long count;
	unsigned long discontinuity;
	unsigned long continuity;
};

struct demux_info {
	struct pid_info  pids[8192];

	unsigned long    ts_packet_c;
	unsigned long    malformed_packet_c;
	unsigned long    tot_scraped_sz;
	unsigned long    packet_no;
	unsigned long    sync_err;
	unsigned long 	   sync_err_set;
};

static int is_sync(unsigned char* p) {
	int syncword = p[0];
#ifdef SYNC_BYTE_INVERSION
	if(0x47 == syncword || 0xb8 == syncword)
		return 1;
#else
	if(0x47 == syncword)
		return 1;
#endif
	return 0;
}
static struct demux_info demux[MAX_DEMUX];

int print_pkt_log()
{
	unsigned long i=0;

	print_log("\nPKT_TOT : %d, SYNC_ERR : %d, SYNC_ERR_BIT : %d, ERR_PKT : %d \n", demux[0].ts_packet_c, demux[0].sync_err, demux[0].sync_err_set, demux[0].malformed_packet_c);

	for(i=0;i<8192;i++)
	{
		if(demux[0].pids[i].count>0)
			print_log("PID : %d, TOT_PKT : %d, DISCONTINUITY : %d \n", i, demux[0].pids[i].count, demux[0].pids[i].discontinuity);
	}

	return 0;

}

int put_ts_packet(int no, unsigned char* packet, int sz) {
	unsigned char* p;
	int transport_error_indicator, pid, continuity_counter, last_continuity_counter;
	//int payload_unit_start_indicator;
	int i; // e = 0
	if((sz % 188)) {
		//e = 1;
		print_log("L : %d", sz);
		//print_log("Video %d Invalid size: %d\n", no, sz);
	} else {
		for(i = 0; i < sz; i += 188) {
			p = packet + i;

			pid = ((p[1] & 0x1f) << 8) + p[2];

			demux[no].ts_packet_c++;
			if(!is_sync(packet + i)) {
				//e = 1;
				print_log("S     ");
				demux[no].sync_err++;
				if(0x80==(p[1] & 0x80))
					demux[no].sync_err_set++;
				print_log("0x%x, 0x%x, 0x%x, 0x%x \n", *p, *(p+1),  *(p+2), *(p+3));
				//print_log("   Video %d Invalid sync: 0x%02x, Offset: %d, Frame No: %d\n", no, *(packet + i), i, i / 188);
				//break;
				continue;
			}

			// Error Indicator가 설정되면 Packet을 버림
			transport_error_indicator = (p[1] & 0x80) >> 7;
			if(1 == transport_error_indicator) {
				demux[no].malformed_packet_c++;
				//e++;
				//print_log("I      ");
				//print_log("   Video %d PID 0x%04x: err_ind, Offset: %d, Frame No: %d\n", no, pid, i, i / 188);
				continue;
			}

			//payload_unit_start_indicator = (p[1] & 0x40) >> 6;

			demux[no].pids[pid].count++;

			// Continuity Counter Check
			continuity_counter = p[3] & 0x0f;

			if(demux[no].pids[pid].continuity == -1) {
				demux[no].pids[pid].continuity = continuity_counter;
			} else {
				last_continuity_counter = demux[no].pids[pid].continuity;

				demux[no].pids[pid].continuity = continuity_counter;

				if(((last_continuity_counter + 1) & 0x0f) != continuity_counter) {
					demux[no].pids[pid].discontinuity++;
					//e++;
					//print_log("D      ");
					//print_log("   Video %d PID 0x%04x: last counter %x ,current %x, Offset: %d,Frame No: %d, start_ind: %d\n", no, pid, last_continuity_counter, continuity_counter, i, i / 188, payload_unit_start_indicator);
				}
			}
		}
	}

	//if(e) {
	//	print_log("Video %d Received Size: %d, Frame Count: %d, Error Count: %d\n", no, sz, sz / 188, e);
	//}
	return 0;
}


void create_tspacket_anal() {
	int n, i;

	for(n = 0; n < MAX_DEMUX; n++) {
		memset((void*)&demux[n], 0, sizeof(demux[n]));

		for(i = 0; i < 8192; i++) {
			demux[n].pids[i].continuity = -1;
		}
	}

}
#endif

u8 dump_start_thread;
void data_dump_thread(void *param)
{
	data_dump_param *data = (data_dump_param*)param;
	int hDevice = data->dev;
	int num = data->num;
	u8 buf[MSC_BUF_SIZE];
	int i;
	int check_cnt_size=0;
	int monitor_size=0;
	int dump_size=0;
	u32 berA, perA, berB, perB, berC, perC;
	s32 i32RSSI;
	u8 slock=0, cnr;

	//while(1)
	{
		FILE *f;
		char *f_name[128];
		sprintf((void*)f_name,DUMP_PATH"data_dump_%04d.dat", f_index++);
		dump_size=0;
		f = fopen((void*)f_name, "wb");

#ifdef FEATURE_TS_CHECK
		create_tspacket_anal();
#endif

		print_log("Start data dump %s , pkt : %d\n", f_name, data->num);
		mtv_ts_start(hDevice);
		dump_start_thread = 1;
		for(i=0;i<num;i++)
		{
			int size;

			size = mtv_data_read(hDevice, buf, MSC_BUF_SIZE);

#ifdef FEATURE_TS_CHECK
			if(!(size%188)) {
				put_ts_packet(0, &buf[0], size);
				dump_size+=size;
				monitor_size+=size;
				check_cnt_size+=size;
#ifdef FEATURE_OVERRUN_CHECK
				{
					u8 over = 0;
					/*	BBM_READ(hDevice, 0x8001, &over);	*/
					if(over)
					{
						/*	BBM_WRITE(hDevice, 0x8001, over);	*/
						print_log("TS OVERRUN : %d\n", over);
					}
				}
#endif
				if(check_cnt_size>188*320*40)
				{
					print_pkt_log();
					check_cnt_size=0;
				}
			}
#endif

			if(monitor_size>188*320*40) {
				monitor_size=0;
				mtv_signal_quality_info(hDevice, &slock, &cnr, &berA, &perA, &berB, &perB, &berC, &perC, &i32RSSI);
				print_log("Lock : 0x%x CN : %3d, RSSI : %3d\n",slock, cnr, i32RSSI);
				print_log("BerA : %6d, PerA : %6d, BerB : %6d, PerB : %6d, BerC : %6d, PerC : %6d, \n", berA, perA, berB, perB, berC, perC);

			}

			/*if((dump_size<100*1024*1024)&&(size>0))*/
			if(!(size%188)) {
				fwrite(&buf[0], 1, size, f);
				dump_size+=size;
			}

			if(!dump_start_thread) {
				print_log("TS Recording : %d Bytes\n", dump_size);
				break;
			}

		}

		mtv_ts_stop(hDevice);
		fclose(f);
		print_log("\nEnd msc dump\n");
		f_index %= 10;
	}
}

int data_dump_start(int hDevice)
{
	int thr_id;
	static data_dump_param param;
	param.dev = hDevice;
	param.num = 100000000;// 6000

	thr_id = pthread_create(&p_thr_data, NULL, (void*)&data_dump_thread, (void*)&param);

	if(thr_id <0)
	{
		print_log("read thread create error.\n");
		return -1;
	}

	pthread_detach(p_thr_data);
	return 0;
}

int data_dump_stop(int hDevice)
{
	dump_start_thread = 0;
	return 0;
}

u8 sig_start_thread;
#define MON_TIME 1000
void sig_thread(void *param)
{
	data_dump_param *data = (data_dump_param*)param;
	int hDevice = data->dev;

	u8 wscn, lock;
	s32 rssi;
	u32 berA, perA, berB, perB, berC, perC;
 	sig_start_thread = 1;

	while(1)
	{
		mtv_signal_quality_info(hDevice, &lock, &wscn, &berA, &perA, &berB, &perB, &berC, &perC, &rssi);
		print_log("Lock : 0x%x CN : %3d, RSSI : %3d\n",lock, wscn, rssi);
		print_log("BerA : %6d, PerA : %6d, BerB : %6d, PerB : %6d, BerC : %6d, PerC : %6d, \n", berA, perA, berB, perB, berC, perC);
		msWait(MON_TIME);

		if(!sig_start_thread)
			break;
	}
}


int data_sig_start(int hDevice)
{
	int thr_id;
	static data_dump_param param;
	param.dev = hDevice;
	param.num = 100000000;// 6000

	thr_id = pthread_create(&p_thr_sig, NULL, (void*)&sig_thread, (void*)&param);

	if(thr_id <0)
	{
		print_log("sig thread create error.\n");
		return -1;
	}

	pthread_detach(p_thr_sig);
	return 0;
}

int data_sig_stop(int hDevice)
{

	sig_start_thread = 0;
	return 0;
}

u8 ptcheck_thread_start;

void ptcheck_thread(void *param)
{
	data_dump_param *data = (data_dump_param*)param;
	int hDevice = data->dev;
	int num = data->num;
	int check_cnt_size=0;
	int check_dump_size=0;
	int size;
	FILE *f;
	char *f_name[128];
	int ch_num = data->ch_num;
	int ret = 0;
	void *vaddrs[MAX_BUFFER_COUNT];
	int buffer_length[MAX_BUFFER_COUNT];
	int dq_index = 0;

	struct timeval timestamp;
	unsigned char *data_buf = NULL;
	int i = 0;

	ptcheck_thread_start = 1;

	print_log("pattern check start \n");

	sprintf((void*)f_name,DUMP_PATH"pattern_ts.dat");
	f = fopen((void*)f_name, "wb");

	print_log("\nchannel number : %d\n", ch_num);
	int mpegts_video_fd = nx_v4l2_open_device(nx_mpegts_video, ch_num);
	if (mpegts_video_fd < 0) {
		fprintf(stderr, "fail to open mpegts_video\n");
		return;
	}

	nx_v4l2_set_ctrl(mpegts_video_fd, nx_mpegts_video,
			V4L2_CID_MPEGTS_PAGE_SIZE, TS_PAGE_SIZE);
	ret = nx_v4l2_reqbuf_mmap(mpegts_video_fd, nx_mpegts_video,
			MAX_BUFFER_COUNT);
	if (ret) {
		fprintf(stderr, "fail to reqbuf!\n");
		return;
	}

	for (i = 0; i < MAX_BUFFER_COUNT; i++) {
		struct v4l2_buffer v4l2_buf;
		ret = nx_v4l2_query_buf_mmap(mpegts_video_fd, nx_mpegts_video,
				i, &v4l2_buf);
		if (ret) {
			fprintf(stderr, "fail to query buf: index %d\n", i);
			return;
		}

		vaddrs[i] = mmap(NULL, v4l2_buf.length, PROT_READ | PROT_WRITE,
				MAP_SHARED, mpegts_video_fd,
				v4l2_buf.m.offset);
		if (vaddrs[i] == MAP_FAILED) {
			fprintf(stderr, "failed to mmap: index %d\n", i);
			return;
		}

		buffer_length[i] = v4l2_buf.length;
	}

	for (i = 0; i < MAX_BUFFER_COUNT; i++) {
		ret = nx_v4l2_qbuf_mmap(mpegts_video_fd, nx_mpegts_video, i);
		if (ret) {
			fprintf(stderr, "failed to qbuf: index %d\n", i);
			return;
		}
	}

	nx_v4l2_streamoff(mpegts_video_fd, nx_mpegts_video);
	ret = nx_v4l2_streamon_mmap(mpegts_video_fd, nx_mpegts_video);
	if (ret) {
		fprintf(stderr, "failed to streamon\n");
		return;
	}

#ifdef FEATURE_TS_CHECK
	create_tspacket_anal();
#endif
	mtv_ts_start(hDevice);
	while(1)
	{
		ret = nx_v4l2_dqbuf_mmap_with_timestamp(mpegts_video_fd,
				nx_mpegts_video, &dq_index, &timestamp);
		if (ret) {
			fprintf(stderr, "failed to dqbuf\n");
			return;
		}

		print_log("dq index : %d\n", dq_index);

		data_buf = (unsigned char *)vaddrs[dq_index];
		size = buffer_length[dq_index];

#ifdef FEATURE_TS_CHECK
		if((size > 0) && (!(size % 188))) {

			put_ts_packet(0, data_buf, size);

			check_cnt_size += size;
			check_dump_size += size;

			if (check_dump_size < (num * 1024 * 1024))
				fwrite(data_buf, 1, size, f);
#ifdef FEATURE_OVERRUN_CHECK
			{
				u8 over = 0;
				/*	BBM_READ(hDevice, 0x8001, &over);	*/
				if(over)
				{
					/*	BBM_WRITE(hDevice, 0x8001, over);	*/
					print_log("TS OVERRUN : %d\n", over);
				}
			}
#endif
			if(check_cnt_size > (188 * 80 * 40))
			{
				print_pkt_log();
				check_cnt_size=0;
			}
		}
#endif

		ret = nx_v4l2_qbuf_mmap(mpegts_video_fd, nx_mpegts_video,
				dq_index);
		if (ret) {
			fprintf(stderr, "failed dqbuf index %d\n", dq_index);
			return;
		}

		if(!ptcheck_thread_start)
			break;
	}

	nx_v4l2_streamoff(mpegts_video_fd, nx_mpegts_video);
	mtv_ts_stop(hDevice);
	fclose(f);
	print_log("\nEnd pattern check\n");
}

int ptcheck_start(int hDevice, int size, int ch_num)
{
	int thr_id;
	static data_dump_param param;
	param.dev = hDevice;
	param.num = size;
	param.ch_num = ch_num;

	thr_id = pthread_create(&ptcheck_thr_sig, NULL, (void*)&ptcheck_thread, (void*)&param);

	if(thr_id <0)
	{
		print_log("sig thread create error.\n");
		return -1;
	}

	pthread_detach(ptcheck_thr_sig);
	return 0;
}

int ptcheck_stop(int hDevice)
{
	ptcheck_thread_start = 0;

	return 0;
}

