/*
 * Copyright (c) 2016 Nexell Co., Ltd.
 * Author: Sungwoo, Park <swpark@nexell.co.kr>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>

#include <sys/types.h>

#include <linux/videodev2.h>

#include "media-bus-format.h"
#include "nx-drm-allocator.h"
#include "nx-v4l2.h"

#include "option.h"

#ifndef ALIGN
#define ALIGN(x, a) (((x) + (a) - 1) & ~((a) - 1))
#endif

static size_t calc_alloc_size(uint32_t w, uint32_t h, uint32_t f)
{
	uint32_t y_stride = ALIGN(w, 32);
	uint32_t y_size = y_stride * ALIGN(h, 16);
	size_t size = 0;

	switch (f) {
	case V4L2_PIX_FMT_YUYV:
	case V4L2_PIX_FMT_NV16:
	case V4L2_PIX_FMT_NV61:
		size = y_size << 1;
		break;

	case V4L2_PIX_FMT_YUV420:
		size = y_size +
			2 * (ALIGN(y_stride >> 1, 16) * ALIGN(h >> 1, 16));
		break;

	case V4L2_PIX_FMT_NV21:
	case V4L2_PIX_FMT_NV12:
		size = y_size + y_stride * ALIGN(h >> 1, 16);
		break;
	}

	return size;
}

#define MAX_BUFFER_COUNT	4

int main(int argc, char *argv[])
{
	int ret;
	uint32_t m, w, h, f, bus_f, count;

	ret = handle_option(argc, argv, &m, &w, &h, &f, &bus_f, &count);
	if (ret) {
		fprintf(stderr, "failed to handle_option\n");
		return ret;
	}

	// workaround code
	if (f == 0)
		f = V4L2_PIX_FMT_YUV420;

	if (bus_f == 0)
		bus_f = MEDIA_BUS_FMT_YUYV8_2X8;

	int sensor_fd = nx_v4l2_open_device(nx_sensor_subdev, m);
	if (sensor_fd < 0) {
		fprintf(stderr, "failed to open camera %d sensor\n", m);
		return -ENODEV;
	}

	bool is_mipi = nx_v4l2_is_mipi_camera(m);

	int csi_subdev_fd;
	if (is_mipi) {
		csi_subdev_fd = nx_v4l2_open_device(nx_csi_subdev, m);
		if (csi_subdev_fd < 0) {
			fprintf(stderr, "failed open mipi csi\n");
			return -ENODEV;
		}
	}

	int clipper_subdev_fd = nx_v4l2_open_device(nx_clipper_subdev, m);
	if (clipper_subdev_fd < 0) {
		fprintf(stderr, "failed to open clipper_subdev %d\n", m);
		return -ENODEV;
	}

	int clipper_video_fd = nx_v4l2_open_device(nx_clipper_video, m);
	if (clipper_video_fd < 0) {
		fprintf(stderr, "failed to open clipper_video %d\n", m);
		return -ENODEV;
	}

	nx_v4l2_streamoff(clipper_video_fd, nx_clipper_video);

	ret = nx_v4l2_link(true, m, nx_clipper_subdev, 1,
			   nx_clipper_video, 0);

	if (is_mipi) {
		// link sensor to mipi csi
		ret = nx_v4l2_link(true, m, nx_sensor_subdev, 0, nx_csi_subdev,
				   0);
		if (ret) {
			fprintf(stderr, "failed to link sensor to csi\n");
			return ret;
		}

		// link mipi csi to clipper subdev
		ret = nx_v4l2_link(true, m, nx_csi_subdev, 1, nx_clipper_subdev,
				   0);
		if (ret) {
			fprintf(stderr, "failed to link csi to clipper\n");
			return ret;
		}
	} else {
		ret = nx_v4l2_link(true, m, nx_sensor_subdev, 0,
				   nx_clipper_subdev, 0);
		if (ret) {
			fprintf(stderr, "failed to link sensor to clipper\n");
			return ret;
		}
	}

	ret = nx_v4l2_set_format(sensor_fd, nx_sensor_subdev, w, h,
				bus_f);
	if (ret) {
		fprintf(stderr, "failed to set_format for sensor\n");
		return ret;
	}

	if (is_mipi) {
		ret = nx_v4l2_set_format(csi_subdev_fd, nx_csi_subdev, w, h, f);
		if (ret) {
			fprintf(stderr, "failed to set_format for csi\n");
			return ret;
		}
	}

	ret = nx_v4l2_set_format(clipper_subdev_fd, nx_clipper_subdev, w, h,
				bus_f);
	if (ret) {
		fprintf(stderr, "failed to set_format for clipper subdev\n");
		return ret;
	}

	ret = nx_v4l2_set_format(clipper_video_fd, nx_clipper_video, w, h, f);
	if (ret) {
		fprintf(stderr, "failed to set_format for clipper video\n");
		return ret;
	}

	// set crop
	ret = nx_v4l2_set_crop(clipper_subdev_fd, nx_clipper_subdev, 0, 0, w,
			       h);
	if (ret) {
		fprintf(stderr, "failed to set_crop for clipper subdev\n");
		return ret;
	}


	ret = nx_v4l2_reqbuf(clipper_video_fd, nx_clipper_video,
			     MAX_BUFFER_COUNT);
	if (ret) {
		fprintf(stderr, "failed to reqbuf\n");
		return ret;
	}

	// allocate buffers
	int drm_fd = open_drm_device();
	if (drm_fd < 0) {
		fprintf(stderr, "failed to open_drm_device\n");
		return -ENODEV;
	}

	size_t alloc_size = calc_alloc_size(w, h, f);
	if (alloc_size <= 0) {
		fprintf(stderr, "invalid alloc size %lu\n", alloc_size);
		return -EINVAL;
	}

	int gem_fds[MAX_BUFFER_COUNT] = { -1, };
	int dma_fds[MAX_BUFFER_COUNT] = { -1, };
	int i;

	for (i = 0; i < MAX_BUFFER_COUNT; i++) {
		int gem_fd = alloc_gem(drm_fd, alloc_size, 0);
		if (gem_fd < 0) {
			fprintf(stderr, "failed to alloc_gem\n");
			return -ENOMEM;
		}

		int dma_fd = gem_to_dmafd(drm_fd, gem_fd);
		if (dma_fd < 0) {
			fprintf(stderr, "failed to gem_to_dmafd\n");
			return -ENOMEM;
		}

		gem_fds[i] = gem_fd;
		dma_fds[i] = dma_fd;
	}

	// qbuf
	for (i = 0; i < MAX_BUFFER_COUNT; i++) {
		ret = nx_v4l2_qbuf(clipper_video_fd, nx_clipper_video, 1, i,
				   &dma_fds[i], (int *)&alloc_size);
		if (ret) {
			fprintf(stderr, "failed qbuf index %d\n", i);
			return ret;
		}
	}

	ret = nx_v4l2_streamon(clipper_video_fd, nx_clipper_video);
	if (ret) {
		fprintf(stderr, "failed to streamon\n");
		return ret;
	}

	int loop_count = count;
	while (loop_count--) {
		int dq_index;
		ret = nx_v4l2_dqbuf(clipper_video_fd, nx_clipper_video, 1,
				    &dq_index);
		if (ret) {
			fprintf(stderr, "failed to dqbuf\n");
			return ret;
		}

		printf("dq index : %d\n", dq_index);

		ret = nx_v4l2_qbuf(clipper_video_fd, nx_clipper_video, 1,
				   dq_index, &dma_fds[dq_index],
				   (int *)&alloc_size);
		if (ret) {
			fprintf(stderr, "failed qbuf index %d\n", dq_index);
			return ret;
		}
	}

	nx_v4l2_streamoff(clipper_video_fd, nx_clipper_video);

	// free buffers
	for (i = 0; i < MAX_BUFFER_COUNT; i++) {
		if (dma_fds[i] >= 0)
			close(dma_fds[i]);
		if (gem_fds[i] >= 0)
			close(gem_fds[i]);
	}

	return ret;
}
