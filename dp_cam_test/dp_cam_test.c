#include <fcntl.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#include <drm_fourcc.h>
#include "xf86drm.h"
#include "dp.h"
#include "dp_common.h"

#include <linux/videodev2.h>
#include "media-bus-format.h"
#include "nexell_drmif.h"
#include "nx-v4l2.h"

#include "option.h"

#ifndef ALIGN
#define ALIGN(x, a) (((x) + (a) - 1) & ~((a) - 1))
#endif

#define MAX_BUFFER_COUNT	4

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
//	DRM_FORMAT_NV24,    /* NV24 : non-subsampled Cr:Cb plane */
//	DRM_FORMAT_NV42,    /* NV42 : non-subsampled Cb:Cr plane */

	/* 3 buffer */
	#if 0
    DRM_FORMAT_YUV410, 	/* YUV9 : 4x4 subsampled Cb (1) and Cr (2) planes */
    DRM_FORMAT_YVU410,	/* YVU9 : 4x4 subsampled Cr (1) and Cb (2) planes */
    DRM_FORMAT_YUV411,	/* YU11 : 4x1 subsampled Cb (1) and Cr (2) planes */
    DRM_FORMAT_YVU411,	/* YV11 : 4x1 subsampled Cr (1) and Cb (2) planes */
    #endif
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
	DP_DBG("[%s] format = %u, size = %d \n ",__func__,f,size);

	return size;
}

static uint32_t choose_format(struct dp_plane *plane, int select)
{
	uint32_t format;
	int size = ARRAY_SIZE(dp_formats);

	if (select > (size-1)) {
		DP_ERR("fail : not support format index (%d) over size %d\n",
			select, size);
		return -EINVAL;
	}
	format = dp_formats[select];

	if (!dp_plane_supports_format(plane, format)) {
		DP_ERR("fail : not support %s\n", dp_forcc_name(format));
		return -EINVAL;
	}

	DP_LOG("format: %s\n", dp_forcc_name(format));
	return format;
}

struct dp_device * dp_device_init(int fd)
{
	struct dp_device *device;
	int err;

	err = drmSetClientCap(fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);
	if (err < 0) {
		DP_ERR("drmSetClientCap() failed: %d %m\n", err);
	}

	device = dp_device_open(fd);
	if (!device) {
		DP_ERR("fail : device open() %m\n");
		return NULL;
	}

	return device;
}

int dp_plane_update(struct dp_device *device, struct dp_framebuffer *fb, uint32_t w, uint32_t h)
{
	int d_idx = 0/*overlay*/, p_idx = 0, err;
	struct dp_plane *plane;

	plane = dp_device_find_plane_by_index(device,
						      d_idx, p_idx);

	if (!plane) {
		DP_ERR("no overlay plane found\n");
		return -EINVAL;
	}
	err = dp_plane_set(plane, fb, 0,0, w, h,0,0,w,h);
	if(err<0) {
		DP_ERR("set plane failed \n");
		return -EINVAL;
	}

	return 1;
}

struct dp_framebuffer * dp_buffer_init(struct dp_device *device, int  x, int y, int gem_fd)
{
	struct dp_framebuffer *fb = NULL;
	int d_idx = 0, p_idx = 0, op_format = 8/*YUV420*/;
	struct dp_plane *plane;

	uint32_t format;
	int err;

	plane = dp_device_find_plane_by_index(device,
					      d_idx, p_idx);
	if (!plane) {
		DP_ERR("no overlay plane found\n");
		return NULL;
	}

	/*
	 * set plane format
	 */
	format = choose_format(plane, op_format);
	if (!format) {
		DP_ERR("fail : no matching format found\n");
		return NULL;
	}

	DP_DBG("format is %d\n",format);

	fb = dp_framebuffer_config(device, format, x, y, 0, gem_fd, calc_alloc_size(x,y,format));
	if (!fb)
	{
		DP_ERR("fail : framebuffer create Fail \n");
		return NULL;
	}	
	err = dp_framebuffer_addfb2(fb);
	if (err<0)
	{
		DP_ERR("fail : framebuffer add Fail \n");
		if (fb)
		{
			dp_framebuffer_free(fb);
		}
		return NULL;
	}

	return fb;
}


int camera_test(struct dp_device *device, int drm_fd, uint32_t m, uint32_t w,
		uint32_t h, uint32_t sw, uint32_t sh, uint32_t f,
		uint32_t bus_f, uint32_t count)
{
	int ret;
	int gem_fds[MAX_BUFFER_COUNT] = { -1, };
	int dma_fds[MAX_BUFFER_COUNT] = { -1, };
	struct dp_framebuffer *fbs[MAX_BUFFER_COUNT] = {NULL,};

	DP_DBG("m: %d, w: %d, h: sw : %d, sh : %d, f: %d, bus_f: %d, c: %d\n",
	       m, w, h, sw, sh, f, bus_f, count);

	// workaround code
	if (f == 0)
		f = V4L2_PIX_FMT_YUV420;

	switch (bus_f) {
	case 0:
		bus_f = MEDIA_BUS_FMT_YUYV8_2X8;
		break;
	case 1:
		bus_f = MEDIA_BUS_FMT_UYVY8_2X8;
		break;
	case 2:
		bus_f = MEDIA_BUS_FMT_VYUY8_2X8;
		break;
	case 3:
		bus_f = MEDIA_BUS_FMT_YVYU8_2X8;
		break;
	};

	int sensor_fd = nx_v4l2_open_device(nx_sensor_subdev, m);
	if (sensor_fd < 0) {
		DP_ERR("failed to open camera %d sensor\n", m);
		return -1;
	}

	bool is_mipi = nx_v4l2_is_mipi_camera(m);

	int csi_subdev_fd;
	if (is_mipi) {
		csi_subdev_fd = nx_v4l2_open_device(nx_csi_subdev, m);
		if (csi_subdev_fd < 0) {
			DP_ERR("failed open mipi csi\n");
			return -1;
		}
	}

	int clipper_subdev_fd = nx_v4l2_open_device(nx_clipper_subdev, m);
	if (clipper_subdev_fd < 0) {
		DP_ERR("failed to open clipper_subdev %d\n", m);
		return -1;
	}

	int clipper_video_fd = nx_v4l2_open_device(nx_clipper_video, m);
	if (clipper_video_fd < 0) {
		DP_ERR("failed to open clipper_video %d\n", m);
		return -1;
	}

	nx_v4l2_streamoff(clipper_video_fd, nx_clipper_video);

	ret = nx_v4l2_link(true, m, nx_clipper_subdev, 1,
			   nx_clipper_video, 0);

	if (is_mipi) {
		// link sensor to mipi csi
		ret = nx_v4l2_link(true, m, nx_sensor_subdev, 0, nx_csi_subdev,
				   0);
		if (ret) {
			DP_ERR("failed to link sensor to csi\n");
			return ret;
		}

		// link mipi csi to clipper subdev
		ret = nx_v4l2_link(true, m, nx_csi_subdev, 1, nx_clipper_subdev,
				   0);
		if (ret) {
			DP_ERR("failed to link csi to clipper\n");
			return ret;
		}
	} else {
		ret = nx_v4l2_link(true, m, nx_sensor_subdev, 0,
				   nx_clipper_subdev, 0);
		if (ret) {
			DP_ERR("failed to link sensor to clipper\n");
			return ret;
		}
	}

	ret = nx_v4l2_set_format(sensor_fd, nx_sensor_subdev, w, h,
				bus_f);
	if (ret) {
		DP_ERR("failed to set_format for sensor\n");
		return ret;
	}

	if (is_mipi) {
		ret = nx_v4l2_set_format(csi_subdev_fd, nx_csi_subdev, w, h, f);
		if (ret) {
			DP_ERR("failed to set_format for csi\n");
			return ret;
		}
	}

	ret = nx_v4l2_set_format(clipper_subdev_fd, nx_clipper_subdev, w, h,
				bus_f);
	if (ret) {
		DP_ERR("failed to set_format for clipper subdev\n");
		return ret;
	}

	ret = nx_v4l2_set_format(clipper_video_fd, nx_clipper_video, w, h, f);
	if (ret) {
		DP_ERR("failed to set_format for clipper video\n");
		return ret;
	}

	// set crop
	ret = nx_v4l2_set_crop(clipper_subdev_fd, nx_clipper_subdev, 0, 0, w,
			       h);
	if (ret) {
		DP_ERR("failed to set_crop for clipper subdev\n");
		return ret;
	}

	if (sw > 0 && sh > 0) {
		ret = nx_v4l2_set_crop(clipper_video_fd, nx_clipper_video,
				0, 0, sw, sh);
		if (ret) {
			DP_ERR("failed to set_crop for clipper subdev\n");
			return ret;
		}
	}

	ret = nx_v4l2_reqbuf(clipper_video_fd, nx_clipper_video,
			     MAX_BUFFER_COUNT);
	if (ret) {
		DP_ERR("failed to reqbuf\n");
		return ret;
	}

	size_t alloc_size = calc_alloc_size(w, h, f);
	if (alloc_size <= 0) {
		DP_ERR("invalid alloc size %lu\n", alloc_size);
		return -1;
	}

	int i;

	for (i = 0; i < MAX_BUFFER_COUNT; i++) {
		int gem_fd = nx_alloc_gem(drm_fd, alloc_size, 0);
		if (gem_fd < 0) {
			DP_ERR("failed to alloc_gem\n");
			return -1;
		}

		int dma_fd = nx_gem_to_dmafd(drm_fd, gem_fd);
		if (dma_fd < 0) {
			DP_ERR("failed to gem_to_dmafd\n");
			return -1;
		}

		struct dp_framebuffer *fb = dp_buffer_init(device, w,h,gem_fd);
		if (!fb) {
			DP_ERR("fail : framebuffer Init %m\n");
			ret = -1;
			return ret;
		}
		fbs[i] = fb;
		gem_fds[i] = gem_fd;
		dma_fds[i] = dma_fd;
	}
	// qbuf
	for (i = 0; i < MAX_BUFFER_COUNT; i++) {
		ret = nx_v4l2_qbuf(clipper_video_fd, nx_clipper_video, 1, i,
				   &dma_fds[i], (int *)&alloc_size);
		if (ret) {
			DP_ERR("failed qbuf index %d\n", i);
			return ret;
		}
	}

	ret = nx_v4l2_streamon(clipper_video_fd, nx_clipper_video);
	if (ret) {
		DP_ERR("failed to streamon\n");
		return ret;
	}

	int loop_count = count;
	while (loop_count--) {
		int dq_index;

		ret = nx_v4l2_dqbuf(clipper_video_fd, nx_clipper_video, 1,
				    &dq_index);
		if (ret) {
			DP_ERR("failed to dqbuf\n");
			return ret;
		}

		ret = nx_v4l2_qbuf(clipper_video_fd, nx_clipper_video, 1,
				   dq_index, &dma_fds[dq_index],
				   (int *)&alloc_size);
		if (ret) {
			DP_ERR("failed qbuf index %d\n", dq_index);
			return ret;
		}
		ret = dp_plane_update(device,fbs[dq_index],w,h);
	/*	if (ret) {
			DP_ERR("failed plane update \n");
			return ret;
		}*/
	}

	nx_v4l2_streamoff(clipper_video_fd, nx_clipper_video);


	// free buffers
	for (i = 0; i < MAX_BUFFER_COUNT; i++) {

		if (fbs[i])
		{
			dp_framebuffer_delfb2(fbs[i]);
		}

		if (dma_fds[i] >= 0)
			close(dma_fds[i]);
		if (gem_fds[i] >= 0)
			nx_free_gem(drm_fd, gem_fds[i]);
	}
	return ret;
}

int main(int argc, char *argv[])
{
	int ret, drm_fd, err;
	uint32_t m, w, h, f, bus_f, count;
	struct dp_device *device;
	int dbg_on = 0;
	uint32_t sw = 0, sh = 0;

	dp_debug_on(dbg_on);

	ret = handle_option(argc, argv, &m, &w, &h, &sw, &sh, &f, &bus_f,
			&count);
	if (ret) {
		DP_ERR("failed to handle_option\n");
		return ret;
	}

	drm_fd = open("/dev/dri/card0",O_RDWR);
	if (drm_fd < 0) {
		DP_ERR("failed to open_drm_device\n");
		return -1;
	}

	device = dp_device_init(drm_fd);
	if (device==(struct dp_device *)NULL) {
		DP_ERR("fail : device open() %m\n");
		return -1;
	}

	err = camera_test(device, drm_fd, m, w, h, sw, sh, f, bus_f, count);
	if (err < 0) {
		DP_ERR("failed to do camera_test \n");
		return -1;
	}

	dp_device_close(device);
	close(drm_fd);

	return ret;
}

