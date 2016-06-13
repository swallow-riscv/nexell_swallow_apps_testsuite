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
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#include <sys/types.h>

#include <linux/videodev2.h>

#include <media-bus-format.h>
#include <nx-drm-allocator.h>
#include <nx-v4l2.h>
#include <libdrm/drm_fourcc.h>
#include <xf86drm.h>

#include <dp.h>
#include <dp_common.h>
#include <nx-scaler.h>

#include "option.h"

#ifndef ALIGN
#define ALIGN(x, a) (((x) + (a) - 1) & ~((a) - 1))
#endif

#define YUV_STRIDE_ALIGN_FACTOR		64
#define YUV_VSTRIDE_ALIGN_FACTOR	16

#define YUV_STRIDE(w)	ALIGN(w, YUV_STRIDE_ALIGN_FACTOR)
#define YUV_YSTRIDE(w)	(ALIGN(w/2, YUV_STRIDE_ALIGN_FACTOR) * 2)
#define YUV_VSTRIDE(h)	ALIGN(h, YUV_VSTRIDE_ALIGN_FACTOR)

static const uint32_t dp_formats[] = {

	/* 1 buffer */
	DRM_FORMAT_YUYV,
	DRM_FORMAT_YVYU,
	DRM_FORMAT_UYVY,
	DRM_FORMAT_VYUY,

	/* 2 buffer */
	DRM_FORMAT_NV12,	/* NV12 : 2x2 subsampled Cr:Cb plane */
	DRM_FORMAT_NV21,    /* NV21 : 2x2 subsampled Cb:Cr plane */
	DRM_FORMAT_NV16,    /* NV16 : 2x1 subsampled Cr:Cb plane */
	DRM_FORMAT_NV61,    /* NV61 : 2x1 subsampled Cb:Cr plane */

	/* 3 buffer */
	DRM_FORMAT_YUV420,	/* YU12 : 2x2 subsampled Cb (1) and Cr (2) planes */
	DRM_FORMAT_YVU420,	/* YV12 : 2x2 subsampled Cr (1) and Cb (2) planes */
	DRM_FORMAT_YUV422,	/* YU16 : 2x1 subsampled Cb (1) and Cr (2) planes */
	DRM_FORMAT_YVU422,	/* YV16 : 2x1 subsampled Cr (1) and Cb (2) planes */
	DRM_FORMAT_YUV444,	/* YU24 : non-subsampled Cb (1) and Cr (2) planes */
	DRM_FORMAT_YVU444,	/* YV24 : non-subsampled Cr (1) and Cb (2) planes */

	/* RGB 1 buffer */
	DRM_FORMAT_RGB565,
	DRM_FORMAT_BGR565,
	DRM_FORMAT_RGB888,
	DRM_FORMAT_BGR888,
	DRM_FORMAT_ARGB8888,
	DRM_FORMAT_ABGR8888,
	DRM_FORMAT_XRGB8888,
	DRM_FORMAT_XBGR8888,
};

static uint32_t choose_format(struct dp_plane *plane, int select)
{
	uint32_t format;
	int size = ARRAY_SIZE(dp_formats);

	if (select > (size-1)) {
		printf("fail : not support format index (%d) over size %d\n",
				select, size);
		return -EINVAL;
	}
	format = dp_formats[select];

	if (!dp_plane_supports_format(plane, format)) {
		printf("fail : not support %s\n", dp_forcc_name(format));
		return -EINVAL;
	}

	return format;
}


struct dp_device * display_device_init(int fd)
{
	struct dp_device *device;
	int err;

	err = drmSetClientCap(fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);
	if (err < 0) {
		printf("drmSetClientCap() failed: %d %m\n", err);
	}

	device = dp_device_open(fd);
	if (!device) {
		printf("fail : device open() %m\n");
		return NULL;
	}

	return device;
}

void set_plane(struct dp_device *device, struct dp_framebuffer *fb,
	uint32_t w, uint32_t h)
{
	int d_idx = 0, p_idx = 0;
	struct dp_plane *plane;

	plane = dp_device_find_plane_by_index(device,
			d_idx, p_idx);
	if (!plane) {
		printf("no overlay plane found\n");
		return;
	}
	dp_plane_set(plane, fb, 0,0, w, h,0,0,w,h);
}

struct dp_framebuffer *display_buffer_init(struct dp_device *device, int  x, int y, int gem_fd, int buf_size)
{
	struct dp_framebuffer *fb = NULL;
	int d_idx = 0, p_idx = 0, op_format = 8;
	struct dp_plane *plane;

	uint32_t format;
	int err;

	plane = dp_device_find_plane_by_index(device,
			d_idx, p_idx);
	if (!plane) {
		printf("no overlay plane found\n");
		return NULL;
	}

	format = choose_format(plane, op_format);
	if (!format) {
		printf("fail : no matching format found\n");
		return NULL;
	}

	fb = dp_framebuffer_config(device, format, x, y, 0, gem_fd, buf_size);
	if (!fb)
	{
		printf("fail : framebuffer create Fail \n");
		return NULL;
	}

	err = dp_framebuffer_addfb2(fb);
	if (err<0)
	{
		printf("fail : framebuffer add Fail \n");
		if (fb)
		{
			dp_framebuffer_free(fb);
		}
		return NULL;
	}

	return fb;
}

void init_scale_context(uint32_t w, uint32_t h, uint32_t s_w, uint32_t s_h,
	uint32_t f, uint32_t plane_num, struct rect crop,
	struct nx_scaler_context *s_ctx)
{

	uint32_t src_y_stride = ALIGN(w, 32);
	uint32_t src_c_stride = ALIGN(src_y_stride >> 1, 16);
	uint32_t dst_y_stride = ALIGN(s_w, 8);
	uint32_t dst_c_stride = ALIGN(dst_y_stride >> 1, 4);

	s_ctx->crop.x = crop.x;
	s_ctx->crop.y = crop.y;
	s_ctx->crop.width = crop.width;
	s_ctx->crop.height = crop.height;

	s_ctx->src_plane_num = plane_num;
	s_ctx->src_width = w;
	s_ctx->src_height = h;
	s_ctx->src_code = f;

	s_ctx->src_stride[0] = src_y_stride;
	s_ctx->src_stride[1] = src_c_stride;
	s_ctx->src_stride[2] = src_c_stride;

	s_ctx->dst_plane_num = plane_num;
	s_ctx->dst_width = s_w;
	s_ctx->dst_height = s_h;
	s_ctx->dst_code = f;

	s_ctx->dst_stride[0] = dst_y_stride;
	s_ctx->dst_stride[1] = dst_c_stride;
	s_ctx->dst_stride[2] = dst_c_stride;
}

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
				2 * (ALIGN(y_stride >> 1, 16) *
				ALIGN(h >> 1, 16));
			break;

		case V4L2_PIX_FMT_NV21:
		case V4L2_PIX_FMT_NV12:
			size = y_size + y_stride * ALIGN(h >> 1, 16);
			break;
	}
	return size;
}

#define MAX_BUFFER_COUNT	4

int scaler_test(struct dp_device *device, int drm_fd, uint32_t m, uint32_t w,
	uint32_t h, uint32_t s_w, uint32_t s_h, uint32_t f, uint32_t bus_f,
	uint32_t count, struct rect crop)
{
	struct nx_scaler_context s_ctx;
	int ret;
	int handle=0;
	int i;

	int gem_fds[MAX_BUFFER_COUNT] = { -1, };
	int dma_fds[MAX_BUFFER_COUNT] = { -1, };
	int dst_gem_fds[MAX_BUFFER_COUNT] = { -1, };
	int dst_dma_fds[MAX_BUFFER_COUNT] = { -1, };
	struct dp_framebuffer *fbs[MAX_BUFFER_COUNT] = {NULL,};

	if (f == 0)
		f = V4L2_PIX_FMT_YUV420;

	if (bus_f == 0)
		bus_f = MEDIA_BUS_FMT_YUYV8_2X8;

	init_scale_context(w, h, s_w, s_h, bus_f, 1, crop, &s_ctx);
	handle = scaler_open();
	if (handle == -1) {
		fprintf(stderr, "failed to open scaler\n");
		return -ENODEV;
	}
	int sensor_fd = nx_v4l2_open_device(nx_sensor_subdev, m);
	if (sensor_fd < 0) {
		fprintf(stderr, "failed to open camera %d sensor\n", m);
		return -1;
	}

	bool is_mipi = nx_v4l2_is_mipi_camera(m);

	int csi_subdev_fd;
	if (is_mipi) {
		csi_subdev_fd = nx_v4l2_open_device(nx_csi_subdev, m);
		if (csi_subdev_fd < 0) {
			fprintf(stderr, "failed open mipi csi\n");
			return -1;
		}
	}

	int clipper_subdev_fd = nx_v4l2_open_device(nx_clipper_subdev, m);
	if (clipper_subdev_fd < 0) {
		fprintf(stderr, "failed to open clipper_subdev %d\n", m);
		return -1;
	}

	int clipper_video_fd = nx_v4l2_open_device(nx_clipper_video, m);
	if (clipper_video_fd < 0) {
		fprintf(stderr, "failed to open clipper_video %d\n", m);
		return -1;
	}

	nx_v4l2_streamoff(clipper_video_fd, nx_clipper_video);

	ret = nx_v4l2_link(true, m, nx_clipper_subdev, 1,
			nx_clipper_video, 0);
	if (is_mipi) {
		ret = nx_v4l2_link(true, m, nx_sensor_subdev, 0, nx_csi_subdev,
				0);
		if (ret) {
			fprintf(stderr, "failed to link sensor to csi\n");
			return ret;
		}

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

	size_t alloc_size = calc_alloc_size(w, h, f);
	if (alloc_size <= 0) {
		fprintf(stderr, "invalid alloc size %lu\n", alloc_size);
		return -1;
	}

	for (i = 0; i < MAX_BUFFER_COUNT; i++) {
		int gem_fd = alloc_gem(drm_fd, alloc_size, 0);
		if (gem_fd < 0) {
			fprintf(stderr, "failed to alloc_gem\n");
			return -1;
		}

		int dma_fd = gem_to_dmafd(drm_fd, gem_fd);
		if (dma_fd < 0) {
			fprintf(stderr, "failed to gem_to_dmafd\n");
			return -1;
		}

		gem_fds[i] = gem_fd;
		dma_fds[i] = dma_fd;
	}

	size_t dst_alloc_size = calc_alloc_size(s_w, s_h, f);
	if (dst_alloc_size <= 0) {
		fprintf(stderr, "invalid alloc size %lu\n",
				dst_alloc_size);
		return -1;
	}

	for (i = 0; i < MAX_BUFFER_COUNT; i++) {
		int gem_fd = alloc_gem(drm_fd, dst_alloc_size, 0);
		if (gem_fd < 0) {
			fprintf(stderr, "failed to alloc_gem\n");
			return -1;
		}

		int dma_fd = gem_to_dmafd(drm_fd, gem_fd);
		if (dma_fd < 0) {
			fprintf(stderr, "failed to gem_to_dmafd\n");
			return -1;
		}
		struct dp_framebuffer *fb = display_buffer_init(device, s_w,
				s_h, gem_fd, static_cast<int>(dst_alloc_size));
		if (!fb) {
			printf("fail : framebuffer Init %m\n");
			ret = -1;
			return ret;
		}
		fbs[i] = fb;

		dst_gem_fds[i] = gem_fd;
		dst_dma_fds[i] = dma_fd;
	}

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

		s_ctx.src_fds[0] = dma_fds[dq_index];
		s_ctx.dst_fds[0] = dst_dma_fds[dq_index];

		ret = nx_scaler_run(handle, &s_ctx);
		if (ret == -1) {
			fprintf(stderr, "failed to scaler set & run ioctl\n");
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
		set_plane(device,fbs[dq_index],s_w,s_h);

	}

	nx_v4l2_streamoff(clipper_video_fd, nx_clipper_video);

	for (i = 0; i < MAX_BUFFER_COUNT; i++) {
		if (fbs[i])
		{
			dp_framebuffer_delfb2(fbs[i]);
			dp_framebuffer_free(fbs[i]);
		}

		if (dma_fds[i] >= 0)
			close(dma_fds[i]);
		if (gem_fds[i] >= 0)
			close(gem_fds[i]);

		if (dst_dma_fds[i] >= 0)
			close(dst_dma_fds[i]);
		if (dst_gem_fds[i] >= 0)
			close(dst_gem_fds[i]);
	}
	nx_scaler_close(handle);

	return ret;
}

int main(int argc, char *argv[])
{
	int ret, drm_fd, err;
	uint32_t m, w, h, f, bus_f, count;
	struct dp_device *device;
	int dbg_on = 0;
	uint32_t s_w, s_h;
	struct rect crop;

	crop.x = 0;
	crop.y = 0;
	crop.width = 0;
	crop.height = 0;

	dp_debug_on(dbg_on);

	ret = handle_option(argc, argv, &m, &w, &h, &s_w, &s_h, &f, &bus_f,
		&count, &crop);
	if (ret) {
		fprintf(stderr, "failed to handle_option\n");
		return ret;
	}

	printf(" open drm device \n");
	drm_fd = open_drm_device();
	if (drm_fd < 0) {
		fprintf(stderr, "failed to open_drm_device\n");
		return -1;
	}

	printf(" display device init \n");
	device = display_device_init(drm_fd);
	if (device==(struct dp_device *)NULL) {
		printf("fail : device open() %m\n");
		return -1;
	}

	err = scaler_test(device, drm_fd, m, w, h, s_w, s_h, f, bus_f, count,
			crop);
	if (err < 0) {
		fprintf(stderr, "failed to do camera_test \n");
		return -1;
	}

	dp_device_close(device);
	close(drm_fd);

	return ret;
}
