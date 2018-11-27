/*
 * Copyright (c) 2016 Nexell Co., Ltd.
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
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <linux/media-bus-format.h>

#include <nx-v4l2.h>
#include <nx-alloc.h>
#include <nx-scaler.h>

//------------------------------------------------------------------------------
#ifndef ALIGN
#define ALIGN(x, a) (((x) + (a)-1) & ~((a)-1))
#endif

#define MAX_BUFFER_COUNT	2

static int32_t SaveOutputImage(NX_MEMORY_HANDLE hImg, const char *fileName)
{
	FILE *fd = fopen(fileName, "wb");
	int32_t width = hImg->width;
	int32_t height = hImg->height;
	uint8_t *pDst;
	int32_t count = 0;

	if (NULL == fd) {
		printf("Failed to open %s file\n", fileName);
		return -1;
	}

	printf("Save %s file, planes:%d\n", fileName, hImg->planes);
	for(int32_t i = 0; i < hImg->planes; i++ )
	{
		pDst = (uint8_t *)hImg->pBuffer[i];
		for (int32_t j = 0; j < height; j++)
		{
			count = fwrite(pDst + hImg->stride[i] * j, 1, hImg->stride[i], fd);
			if (count != hImg->stride[i])
				printf("failed to write %d data:%d\n", width, count);
		}
		if(i == 0) {
			width /= 2;
			height /= 2;
		}
	}

	fclose(fd);

	return 0;
}


static int32_t DoScaling(NX_MEMORY_HANDLE hIn, NX_MEMORY_HANDLE hOut,
		int32_t cropX, int32_t cropY, int32_t cropWidth, int32_t cropHeight)
{
	struct nx_scaler_context scalerCtx;
	int32_t ret;
	int32_t hScaler = scaler_open();

	if (hScaler < 0) {
		printf("[%s] failed to open scaler:%d\n", __func__, hScaler);
		return hScaler;
	}

	memset(&scalerCtx, 0, sizeof(struct nx_scaler_context));

	// scaler crop
	scalerCtx.crop.x = cropX;
	scalerCtx.crop.y = cropY;
	scalerCtx.crop.width = cropWidth;
	scalerCtx.crop.height = cropHeight;

	// scaler src
	scalerCtx.src_plane_num = 3;
	scalerCtx.src_width     = hIn->width;
	scalerCtx.src_height    = hIn->height;
	scalerCtx.src_code      = MEDIA_BUS_FMT_YUYV8_2X8;
	scalerCtx.src_fds[0]    = hIn->sharedFd[0];
	scalerCtx.src_fds[1]    = hIn->sharedFd[1];
	scalerCtx.src_fds[2]    = hIn->sharedFd[2];
	scalerCtx.src_stride[0] = hIn->stride[0];
	scalerCtx.src_stride[1] = hIn->stride[1];
	scalerCtx.src_stride[2] = hIn->stride[2];

	// scaler dst
	scalerCtx.dst_plane_num = 3;
	scalerCtx.dst_width     = hOut->width;
	scalerCtx.dst_height    = hOut->height;
	scalerCtx.dst_code      = MEDIA_BUS_FMT_YUYV8_2X8;
	scalerCtx.dst_fds[0]    = hOut->sharedFd[0];
	scalerCtx.dst_fds[1]    = hOut->sharedFd[1];
	scalerCtx.dst_fds[2]    = hOut->sharedFd[2];
	scalerCtx.dst_stride[0] = hOut->stride[0];
	scalerCtx.dst_stride[1] = hOut->stride[1];
	scalerCtx.dst_stride[2] = hOut->stride[2];

	ret = nx_scaler_run(hScaler, &scalerCtx);
	nx_scaler_close(hScaler);
	return ret;
}

static int camera_test(uint32_t type, NX_MEMORY_HANDLE *hMem,
		NX_MEMORY_HANDLE *hOutMem, uint32_t w, uint32_t h,
		uint32_t x, uint32_t y, uint32_t cropWidth, uint32_t cropHeight,
		uint32_t count, uint32_t f, char *strOutFile)
{
	int ret = 0, video_fd;
	uint32_t i;
	char path[20] = {0, };
	char data;
	int loop_count = count, size = 0, dq_index = 0;
	NX_MEMORY_HANDLE *hOut;

	if (type == nx_clipper_video) {
		strcpy(path, "/dev/video6");
		data = 0x66;
		printf("Clipper Camera Test - %s\n", path);
	} else {
		strcpy(path, "/dev/video7");
		data = 0x33;
		printf("Decimator Camera Test - %s\n", path);
	}

	video_fd = open(path, O_RDWR);
	if (video_fd < 0) {
		printf("failed to open video device %s\n", path);
		return -1;
	}
	printf("video_fd:%d\n", video_fd);
	/* set format */
	ret = nx_v4l2_set_format(video_fd, type, w, h, f);
	if (ret) {
		printf("failed to set format:%d\n", ret);
		goto close;
	}
	printf("set_format done\n");

	ret = nx_v4l2_reqbuf(video_fd, type, MAX_BUFFER_COUNT);
	if (ret) {
		printf("failed to reqbuf: %d\n", ret);
		goto close;
	}

	size = hMem[0]->size[0] + hMem[0]->size[1] + hMem[0]->size[2];
	/* qbuf */
	for (i = 0; i < MAX_BUFFER_COUNT; i++) {
		ret = nx_v4l2_qbuf(video_fd, type, 1, i, &hMem[i]->sharedFd[0],
				&size);
		if (ret) {
			printf("failed qbuf index %d\n", i);
			goto finish;
		}
		printf("qbuf:%d done, size:%d\n", i, size);
	}

	/* stream on */
	printf("start stream\n");
	ret = nx_v4l2_streamon(video_fd, type);
	if (ret) {
		printf("failed to streamon:%d\n", ret);
		goto finish;
	}
	printf("start stream done\n");

	while (loop_count--) {
		ret = nx_v4l2_dqbuf(video_fd, type, 1, &dq_index);
		if (ret) {
			printf("failed to dqbuf: %d\n", ret);
			goto stop;
		}
		printf("Dqbuf:%d done\n", dq_index);
		if (cropWidth && cropHeight) {
			ret = DoScaling(hMem[dq_index], hOutMem[dq_index], x, y, cropWidth, cropHeight);
			if (ret) {
				printf("Error: DoScaling !!\n");
				goto stop;
			}
			hOut = &hOutMem[dq_index];
		} else
			hOut = &hMem[dq_index];

		if (count == 1)
			break;
		ret = nx_v4l2_qbuf(video_fd, type, 1, dq_index,
				&hMem[dq_index]->sharedFd[0], &size);
		if (ret) {
			printf("failed qbuf index %d\n", ret);
			goto stop;
		}
	}

	if (0 != SaveOutputImage(*(NX_MEMORY_HANDLE*)hOut, (const char *)strOutFile))
	{
		printf("Error : SaveOutputImage !!\n");
		goto stop;
	}

stop:
	/* stream off */
	printf("stop stream\n");
	ret = nx_v4l2_streamoff(video_fd, type);
	if (ret) {
		printf("failed to streamoff:%d\n", ret);
		return ret;
	}

finish:
	printf("req buf 0\n");
	ret = nx_v4l2_reqbuf(video_fd, type, 0);
	if (ret) {
		printf("failed to reqbuf:%d\n", ret);
		return ret;
	}

close:
	close(video_fd);
	return 0;
}

static void print_usage(const char *appName)
{
	printf(
		"Usage : %s [options] -f [file] , [M] = mandatory, [O] = option\n"
		"  common options :\n"
		"     -t [type]		          [M]  : 0 - clipper, 1 - decimator\n"
		"     -s [width],[height]         [M]  : input image's size\n"
		"     -c [x],[y],[width],[height] [O]  : scaling : crop x, y, width, hegith\n"
		"				       : mandatory to use scaling\n"
		"     -d [width],[height]         [O]  : output image's size\n"
		"				       : mandatory to use scaling\n"
		"     -r [count]		  [O]  : repeat count, default:1\n"
		"     -o [filename]		  [M]  : output file name\n"
		"     -f [format]		  [M]  : 0 - YUV420 1 - YUV422p\n"
		"     -h : help\n"
		" =========================================================================================================\n\n",
		appName);
	printf(
		" Example :\n"
		"     #> %s -t 0 -s 1280,720 -r 10 -f 0 -o result.yuv\n"
		"     #> %s -t 0 -s 1280,720 -c 0,0,640,480 -d 1280,720 -r 10 -f 0 -o result.yuv\n"
		, appName);

}

int32_t main(int32_t argc, char *argv[])
{
	int32_t opt, ret, i, f = 0, format, type, t = 0;
	int32_t inWidth, inHeight, count = 1;
	int32_t outWidth = 0, outHeight = 0;
	int32_t cropWidth = 0, cropHeight = 0, x = 0, y = 0;
	NX_MEMORY_HANDLE hMem[MAX_BUFFER_COUNT] = {0, };
	NX_MEMORY_HANDLE hOutMem[MAX_BUFFER_COUNT] = {0, };
	char *strOutFile = NULL;

	printf("======camera test application=====\n");
	while (-1 != (opt = getopt(argc, argv, "t:s:c:d:r:h:o:f:")))
	{
		switch (opt)
		{
			case 't':
				sscanf(optarg, "%d", &t);
				break;
			case 's':
				sscanf(optarg, "%d,%d", &inWidth, &inHeight);
				break;
			case 'c':
				sscanf(optarg, "%d,%d,%d,%d", &x, &y,
						&cropWidth, &cropHeight);
				break;
			case 'd':
				sscanf(optarg, "%d,%d", &outWidth, &outHeight);
				break;
			case 'r':
				sscanf(optarg, "%d", &count);
				break;
			case 'h':
				print_usage(argv[0]);
				return 0;
			case 'o':
				strOutFile = strdup(optarg);
				break;
			case 'f':
				sscanf(optarg, "%d", &f);
				break;
			default:
				break;
		}
	}

	if(0>=inWidth || 0>=inHeight)
	{
		printf("Error : Invalid arguments!!!");
		print_usage(argv[0]);
		return -1;
	}

	if (!outWidth || !outHeight) {
		outWidth = inWidth;
		outHeight = inHeight;
	}

	if ((x + cropWidth > inWidth) || (y + cropHeight > inHeight))
	{
		printf("Error : Invalid arguments!!!");
		print_usage(argv[0]);
		return -1;
	}

	if (!t)
		type = nx_clipper_video;
	else
		type = nx_decimator_video;

	printf("device:%s in [%d:%d] crop [%d:%d:%d:%d] out [%d:%d] repeat:%d max buffer:%d outFile:%s f:%d\n",
			(t) ? "decimator":"clipper", inWidth, inHeight, x, y, cropWidth, cropHeight,
			outWidth, outHeight, count, MAX_BUFFER_COUNT, strOutFile, f);

	if (f == 0) {
		format = V4L2_PIX_FMT_YUV420;
		printf("format = YUV420\n");
	} else if (f == 1) {
		format = V4L2_PIX_FMT_YUYV;
		printf("format = YVU422P, YUYV\n");
	}

	/* Allocate Image */
	for (i = 0; i < MAX_BUFFER_COUNT; i++) {
		int planes = 3;

		if (format == V4L2_PIX_FMT_YUYV)
			planes = 1;

		hMem[i] = NX_AllocateMemory(inWidth, inHeight, planes, format, 4096);
		if (hMem[i] == NULL) {
			printf("hMem is NULL for %d\n", i);
			return -1;
		}
		if (0 != NX_MapMemory(hMem[i])) {
			printf("failed to map hInMem for %d\n", i);
			return -1;
		}
		if (outWidth && outHeight) {
			hOutMem[i] = NX_AllocateMemory(outWidth, outHeight, planes, format, 4096);
			if (hOutMem[i] == NULL) {
				printf("hOutMem is NULL for %d\n", i);
				return -1;
			}
			if (0 != NX_MapMemory(hOutMem[i])) {
				printf("failed to map hInMem for %d\n", i);
				return -1;
			}
		}
	}

	ret = camera_test(type, hMem, hOutMem, inWidth, inHeight,
			x, y, cropWidth, cropHeight, count, format, strOutFile);
	if (ret) {
		printf("Error : camera test:%d !!\n", ret);
		goto ErrorExit;
	}

	printf("Camera Test Done!!\n");

	for (i = 0; i < MAX_BUFFER_COUNT; i++) {
		if (hMem[i])
			NX_FreeMemory(hMem[i]);
		if (hOutMem[i])
			NX_FreeMemory(hOutMem[i]);
	}

ErrorExit:
    return 0;
}
