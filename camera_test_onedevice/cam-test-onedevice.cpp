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
#include <fcntl.h>
#include <string.h>

#include <sys/ioctl.h>
#include <sys/types.h>

#include <linux/videodev2.h>

#include "media-bus-format.h"
#include "nx-drm-allocator.h"

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

static int v4l2_qbuf(int fd, int index, uint32_t buf_type, uint32_t mem_type,
		     int dma_fd, int length)
{
	struct v4l2_buffer v4l2_buf;

	bzero(&v4l2_buf, sizeof(v4l2_buf));
	v4l2_buf.m.fd = dma_fd;
	v4l2_buf.type = buf_type;
	v4l2_buf.memory = mem_type;
	v4l2_buf.index = index;
	v4l2_buf.length = length;

	return ioctl(fd, VIDIOC_QBUF, &v4l2_buf);
}

static int v4l2_dqbuf(int fd, uint32_t buf_type, uint32_t mem_type, int *index)
{
	int ret;
	struct v4l2_buffer v4l2_buf;

	bzero(&v4l2_buf, sizeof(v4l2_buf));
	v4l2_buf.type = buf_type;
	v4l2_buf.memory = mem_type;

	ret = ioctl(fd, VIDIOC_DQBUF, &v4l2_buf);

	if (ret)
		return ret;

	*index = v4l2_buf.index;
	return 0;
}

/* nx-camera-test-onedevice -w 1280 -h 720 -c 1000 */
int main(int argc, char *argv[])
{
	int ret;
	uint32_t w, h, count;
	uint32_t buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	ret = handle_option(argc, argv, &w, &h, &count);
	if (ret) {
		fprintf(stderr, "failed to handle_option\n");
		return ret;
	}

	/* pixel format */
	/* TODO: get format from command line */
	uint32_t f = V4L2_PIX_FMT_YUV420;

	int video_fd = open("/dev/video6", O_RDWR);
	if (video_fd < 0) {
		fprintf(stderr, "failed to open clipper_video\n");
		return -ENODEV;
	}

	/* set format */
	struct v4l2_format v4l2_fmt;
	bzero(&v4l2_fmt, sizeof(v4l2_fmt));
	v4l2_fmt.type = buf_type;
	v4l2_fmt.fmt.pix.width = w;
	v4l2_fmt.fmt.pix.height = h;
	v4l2_fmt.fmt.pix.pixelformat = f;
	ret = ioctl(video_fd, VIDIOC_S_FMT, &v4l2_fmt);
	if (ret) {
		fprintf(stderr, "failed to set format\n");
		return ret;
	}

	struct v4l2_requestbuffers req;
	bzero(&req, sizeof(req));
	req.count = count;
	req.memory = V4L2_MEMORY_DMABUF;
	req.type = buf_type;
	ret = ioctl(video_fd, VIDIOC_REQBUFS, &req);
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

	/* qbuf */
	for (i = 0; i < MAX_BUFFER_COUNT; i++) {
		ret = v4l2_qbuf(video_fd, i, buf_type, V4L2_MEMORY_DMABUF,
				dma_fds[i], alloc_size);
		if (ret) {
			fprintf(stderr, "failed qbuf index %d\n", i);
			return ret;
		}
	}

	/* stream on */
	ret = ioctl(video_fd, VIDIOC_STREAMON, &buf_type);
	if (ret) {
		fprintf(stderr, "failed to streamon\n");
		return ret;
	}

	int loop_count = count;
	while (loop_count--) {
		int dq_index;
		ret = v4l2_dqbuf(video_fd, buf_type, V4L2_MEMORY_DMABUF,
				 &dq_index);
		if (ret) {
			fprintf(stderr, "failed to dqbuf\n");
			return ret;
		}

		printf("dq index : %d\n", dq_index);

		ret = v4l2_qbuf(video_fd, dq_index, buf_type,
				V4L2_MEMORY_DMABUF, dma_fds[dq_index],
				alloc_size);
		if (ret) {
			fprintf(stderr, "failed qbuf index %d\n", dq_index);
			return ret;
		}
	}

	/* stream off */
	ioctl(video_fd, VIDIOC_STREAMOFF, &buf_type);

	/* free buffers */
	for (i = 0; i < MAX_BUFFER_COUNT; i++) {
		if (dma_fds[i] >= 0)
			close(dma_fds[i]);
		if (gem_fds[i] >= 0)
			close(gem_fds[i]);
	}

	return ret;
}
