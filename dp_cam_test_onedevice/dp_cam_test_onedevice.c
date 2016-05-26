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

#include <fcntl.h>
#include <string.h>

#include <sys/ioctl.h>

#include <drm_fourcc.h>
#include "xf86drm.h"
#include "dp.h"
#include "dp_common.h"

#include <linux/videodev2.h>
#include "media-bus-format.h"
#include "nexell_drmif.h"
#include "nx-drm-allocator.h"
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

	if (select > (size - 1)) {
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


struct dp_device *dp_device_init(int fd)
{
	struct dp_device *device;
	int err;


	err = drmSetClientCap(fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);
	if (err < 0)
		DP_ERR("drmSetClientCap() failed: %d %m\n", err);

	device = dp_device_open(fd);
	if (!device) {
		DP_ERR("fail : device open() %m\n");
		return NULL;
	}

	return device;
}

int dp_plane_update(struct dp_device *device, struct dp_framebuffer *fb,
		    uint32_t w, uint32_t h)
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
	if (err < 0) {
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

	fb = dp_framebuffer_config(device, format, x, y, 0, gem_fd,
				   calc_alloc_size(x,y,format));
	if (!fb) {
		DP_ERR("fail : framebuffer create Fail \n");
		return NULL;
	}

	err = dp_framebuffer_addfb2(fb);
	if (err < 0) {
		DP_ERR("fail : framebuffer add Fail \n");
		if (fb)
			dp_framebuffer_free(fb);
		return NULL;
	}

	return fb;
}

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

static int camera_test(struct dp_device *device, int drm_fd, uint32_t w,
		       uint32_t h, uint32_t count, char *path)
{
	int ret;
	uint32_t buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	struct v4l2_format v4l2_fmt;
	struct v4l2_requestbuffers req;
	size_t alloc_size;
	int gem_fds[MAX_BUFFER_COUNT] = { -1, };
	int dma_fds[MAX_BUFFER_COUNT] = { -1, };
	struct dp_framebuffer *fbs[MAX_BUFFER_COUNT] = {NULL,};
	int i;

	/* pixel format */
	/* TODO: get format from command line */
	uint32_t f = V4L2_PIX_FMT_YUV420;

	int video_fd = open(path, O_RDWR);
	if (video_fd < 0) {
		fprintf(stderr, "failed to open video device %s\n", path);
		return -ENODEV;
	}

	/* set format */
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

	bzero(&req, sizeof(req));
	req.count = count;
	req.memory = V4L2_MEMORY_DMABUF;
	req.type = buf_type;
	ret = ioctl(video_fd, VIDIOC_REQBUFS, &req);
	if (ret) {
		fprintf(stderr, "failed to reqbuf\n");
		return ret;
	}

	alloc_size = calc_alloc_size(w, h, f);
	if (alloc_size <= 0) {
		fprintf(stderr, "invalid alloc size %lu\n", alloc_size);
		return -EINVAL;
	}


	for (i = 0; i < MAX_BUFFER_COUNT; i++) {
		struct dp_framebuffer *fb;
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

		fb = dp_buffer_init(device, w, h, gem_fd);
		if (!fb) {
			DP_ERR("fail : framebuffer Init %m\n");
			ret = -1;
			return ret;
		}

		fbs[i] = fb;
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

		ret = v4l2_qbuf(video_fd, dq_index, buf_type,
				V4L2_MEMORY_DMABUF, dma_fds[dq_index],
				alloc_size);
		if (ret) {
			fprintf(stderr, "failed qbuf index %d\n", dq_index);
			return ret;
		}

		dp_plane_update(device,fbs[dq_index], w, h);
	}

	/* stream off */
	ioctl(video_fd, VIDIOC_STREAMOFF, &buf_type);

	close(video_fd);

	/* free buffers */
	for (i = 0; i < MAX_BUFFER_COUNT; i++) {
		if (fbs[i])
			dp_framebuffer_delfb2(fbs[i]);
		/* if (dma_fds[i] >= 0) */
		/* 	close(dma_fds[i]); */
		if (gem_fds[i] >= 0)
			close(gem_fds[i]);
	}

	return ret;
}

/* dp_cam_test_onedevice -w 1280 -h 720 -c 1000 -d /dev/video6 */
int main(int argc, char *argv[])
{
	int ret, drm_fd, err;
	uint32_t w, h, count;
	char dev_path[64] = {0, };
	struct dp_device *device;
	int dbg_on = 0;

	dp_debug_on(dbg_on);

	ret = handle_option(argc, argv, &w, &h, &count, dev_path);
	if (ret) {
		DP_ERR("failed to handle_option\n");
		return ret;
	}

	printf("camera device path ==> %s\n", dev_path);

	drm_fd = open("/dev/dri/card0",O_RDWR);
	if (drm_fd < 0) {
		DP_ERR("failed to open_drm_device\n");
		return -1;
	}

	device = dp_device_init(drm_fd);
	if (!device) {
		DP_ERR("fail : device open() %m\n");
		return -1;
	}

	err = camera_test(device, drm_fd, w, h, count, dev_path);
	if (err < 0) {
		DP_ERR("failed to do camera_test \n");
		return -1;
	}

	dp_device_close(device);
	close(drm_fd);

	return ret;
}

