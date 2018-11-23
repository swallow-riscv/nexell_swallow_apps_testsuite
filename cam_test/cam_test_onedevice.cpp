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

//------------------------------------------------------------------------------
#ifndef ALIGN
#define ALIGN(x, a) (((x) + (a)-1) & ~((a)-1))
#endif

#define MAX_BUFFER_COUNT	2

static int32_t VerifyOutputImage(NX_MEMORY_HANDLE srcImg, char data)
{
	int32_t src_width = srcImg->width;
	int32_t src_height = srcImg->height;
	uint8_t *pSrc;

	printf("[%s]\n", __func__);

	for( int32_t i=0 ; i<3 ;i++ )
	{
		pSrc = (uint8_t *)srcImg->pBuffer[i];
		for (int32_t j = 0; j < src_height; j++)
		{
			for (int32_t h = 0; h < src_width; h++)
			{
				if (*pSrc != data) {
					printf("src[%x] dst[%x] is not matched\n", *pSrc, data);
					return -1;
				}
				pSrc++;
			}
			pSrc += srcImg->stride[i] - src_width;
		}
		if( i == 0 )
		{
			src_width /= 2;
			src_height /= 2;
		}
	}

	return 0;
}

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

static int camera_test(uint32_t type, NX_MEMORY_HANDLE *hMem, uint32_t w, uint32_t h,
		uint32_t count, uint32_t f, char *strOutFile)
{
	int ret = 0, video_fd;
	uint32_t i;
	char path[20] = {0, };
	char data;
	int loop_count = count, size = 0, dq_index = 0;

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
		if (0 != SaveOutputImage(hMem[dq_index], (const char *)strOutFile))
		{
			printf("Error : SaveOutputImage !!\n");
			goto stop;
		}

		if (count == 1)
			break;
		ret = nx_v4l2_qbuf(video_fd, type, 1, dq_index,
				&hMem[dq_index]->sharedFd[0], &size);
		if (ret) {
			printf("failed qbuf index %d\n", ret);
			goto stop;
		}
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
		"     -c [count]		  [O]  : repeat count, default:1\n"
		"     -o [filename]		  [M]  : output file name\n"
		"     -f [format]		  [M]  : 0 - YUV420 1 - YUV422p\n"
		"     -h : help\n"
		" =========================================================================================================\n\n",
		appName);
	printf(
		" Example :\n"
		"     #> %s -s 1920,1080 -c 10\n"
		, appName);

}

int32_t main(int32_t argc, char *argv[])
{
	int32_t opt, ret, i, f = 0, format, type, t = 0;
	int32_t inWidth, inHeight, count = 1;
	char *path;
	NX_MEMORY_HANDLE hMem[MAX_BUFFER_COUNT] = {0, };
	char *strOutFile = NULL;

	printf("======camera test application=====\n");
	printf("format 0:YUV420 1:YUV422p\n");
	while (-1 != (opt = getopt(argc, argv, "t:s:c:h:o:f:")))
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

	if (!t)
		type = nx_clipper_video;
	else
		type = nx_decimator_video;

	printf("device:%s width:%d height:%d repeat count:%d max buffer:%d outFile:%s f:%d\n",
			(t) ? "decimator":"clipper", inWidth, inHeight, count, MAX_BUFFER_COUNT,
			strOutFile, f);

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
	}

	ret = camera_test(type, hMem, inWidth, inHeight, count, format,
			strOutFile);
	if (ret) {
		printf("Error : camera test:%d !!\n", ret);
		goto ErrorExit;
	}

	printf("Camera Test Done!!\n");

	for (i = 0; i < MAX_BUFFER_COUNT; i++) {
		if (hMem[i])
			NX_FreeMemory(hMem[i]);
	}

ErrorExit:
    return 0;
}
