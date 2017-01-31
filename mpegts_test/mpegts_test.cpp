/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: Jongkeun, Choi <jkchoi@nexell.co.kr>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>

#include <linux/videodev2.h>
#include <nx-v4l2.h>

#include "mpegts.h"
#include "option.h"

#define V4L2_CID_MPEGTS_PAGE_SIZE       (V4L2_CTRL_CLASS_USER | 0x1001)
#define MAX_BUFFER_COUNT 32

int mpegts_test(uint32_t count)
{
	int ret = 0;
	void *vaddrs[MAX_BUFFER_COUNT];
	int buffer_length[MAX_BUFFER_COUNT];
	int dq_index;
	int loop_count = 0;

	struct timeval timestamp;
	unsigned char *data = NULL;
	int length = 0;
	int i = 0;

	int mpegts_video_fd = nx_v4l2_open_device(nx_mpegts_video, 0);
	if (mpegts_video_fd < 0) {
		fprintf(stderr, "fail to open mpegts_video\n");
		return -1;
	}

	nx_v4l2_set_ctrl(mpegts_video_fd, nx_mpegts_video, V4L2_CID_MPEGTS_PAGE_SIZE,
			TS_PAGE_SIZE);

	ret = nx_v4l2_reqbuf_mmap(mpegts_video_fd, nx_mpegts_video,
		MAX_BUFFER_COUNT);
	if (ret) {
		fprintf(stderr, "fail to reqbuf!\n");
		return -1;
	}

	for (i = 0; i < MAX_BUFFER_COUNT; i++) {
		struct v4l2_buffer v4l2_buf;
		ret = nx_v4l2_query_buf_mmap(mpegts_video_fd, nx_mpegts_video,
						i, &v4l2_buf);
		if (ret) {
			fprintf(stderr, "fail to query buf: index %d\n", i);
			return -1;
		}

		vaddrs[i] = mmap(NULL, v4l2_buf.length, PROT_READ | PROT_WRITE,
				MAP_SHARED, mpegts_video_fd,
				v4l2_buf.m.offset);
		if (vaddrs[i] == MAP_FAILED) {
			fprintf(stderr, "failed to mmap: index %d\n", i);
			return -1;
		}

		buffer_length[i] = v4l2_buf.length;
	}

	for (i = 0; i < MAX_BUFFER_COUNT; i++) {
		ret = nx_v4l2_qbuf_mmap(mpegts_video_fd, nx_mpegts_video, i);
		if (ret) {
			fprintf(stderr, "failed to qbuf: index %d\n", i);
			return -1;
		}
	}

	nx_v4l2_streamoff(mpegts_video_fd, nx_mpegts_video);
	ret = nx_v4l2_streamon_mmap(mpegts_video_fd, nx_mpegts_video);
	if (ret) {
		fprintf(stderr, "failed to streamon\n");
		return ret;
	}

	loop_count = count;
	while (loop_count--) {
		ret = nx_v4l2_dqbuf_mmap_with_timestamp(mpegts_video_fd, nx_mpegts_video, &dq_index, &timestamp);
		if (ret) {
			fprintf(stderr, "failed to dqbuf\n");
			return ret;
		}

		data = (unsigned char *)vaddrs[dq_index];
		length = buffer_length[dq_index];

		ret = nx_v4l2_qbuf_mmap(mpegts_video_fd, nx_mpegts_video,
					dq_index);
		if (ret) {
			fprintf(stderr, "failed qbuf index %d\n", dq_index);
			return ret;
		}
	}
	nx_v4l2_streamoff(mpegts_video_fd, nx_mpegts_video);

	return ret;
}

int main(int argc, char *argv[])
{
	int err;
	int dbg_on = 0;
	int ret = 0;
	uint32_t count = 0;

	ret = handle_option(argc, argv, &count);
	if (ret) {
		fprintf(stderr, "failto handle_option\n");
		return ret;
	}

	err = mpegts_test(count);
	if (err < 0) {
		fprintf(stderr, "failed to do mpegts_teste\n");
		return -1;
	}

	return ret;
}
