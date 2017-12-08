#include <stdint.h>
#include <stdio.h>
#include <linux/ion.h>
#include <linux/videodev2.h>
#include <linux/videodev2_nxp_media.h>
#include <videodev2_nxp_media.h>
#include <linux/media-bus-format.h>
#include <nx_video_alloc.h>
#include <nx-scaler.h>


// int scaler_open(void);
// int nx_scaler_run(int handle, struct nx_scaler_context *s_ctx);
// void nx_scaler_close(int handle);

//------------------------------------------------------------------------------
#ifndef ALIGN
#define ALIGN(x, a) (((x) + (a)-1) & ~((a)-1))
#endif
static int32_t LoadInputImage(NX_VID_MEMORY_HANDLE hImg, const char *fileName)
{
	FILE *fd = fopen(fileName, "rb");
	int32_t width = hImg->width;
	int32_t height = hImg->height;
	int32_t stride = hImg->stride[0];
	uint8_t *pDst;

	if( NULL == fd )
		return -1;

	for( int32_t i=0 ; i<3 ;i++ )
	{
		pDst = (uint8_t *)hImg->pBuffer[i];
		for (int32_t j = 0; j < height; j++)
		{
			if (width != fread(pDst, 1, width, fd))
			{
				printf("Read failed!! (j=%d, width=%d) \n", j, width);
				fclose(fd);
				return -1;
			}
			pDst += hImg->stride[i];
		}
		if( i == 0 )
		{
			width /= 2;
			height /= 2;
		}
	}
	fclose(fd);

	return 0;
}

static int32_t DoScaling(NX_VID_MEMORY_HANDLE hIn, NX_VID_MEMORY_HANDLE hOut, int32_t cropX, int32_t cropY, int32_t cropWidth, int32_t cropHeight)
{
	struct nx_scaler_context scalerCtx;
	int32_t ret;
	int32_t hScaler = scaler_open();
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
	return 0;
}

static int32_t SaveOutputImage(NX_VID_MEMORY_HANDLE hImg, const char *fileName)
{
	FILE *fd = fopen(fileName, "wb");
	int32_t width = hImg->width;
	int32_t height = hImg->height;
	uint8_t *pDst;

	if (NULL == fd)
		return -1;

	for( int32_t i=0 ; i<3 ; i++ )
	{
		pDst = (uint8_t *)hImg->pBuffer[i];
		for (int32_t j = 0; j < height; j++)
		{
			fwrite(pDst + hImg->stride[i] * j, 1, width, fd);
		}
		if( i==0 )
		{
			width /= 2;
			height /= 2;
		}
	}

	fclose(fd);

	return 0;
}

//------------------------------------------------------------------------------
static void print_usage(const char *appName)
{
	printf(
		"Usage : %s [options] -i [input file] , [M] = mandatory, [O] = Optional \n"
		"  common options :\n"
		"     -i [input file name]        [M]  : input media file name (When is camera encoder, the value set NULL\n"
		"     -o [output file name]       [M]  : output file name\n"
		"     -s [width],[height]         [M]  : input image's size\n"
		"     -d [width],[height]         [M]  : destination image's size\n"
		"     -c [x],[y],[width],[height] [O]  : input crop information\n"
		"     -h : help\n"
		" ===================================================================================================================\n\n",
		appName);
	printf(
		" Example :\n"
		"     #> %s -i [input filename] -o [output filename] -s 1920,1080 -d 1280,720\n",
		"     #> %s -i [input filename] -o [output filename] -s 1920,1080 -d 1280,720 -c 0,0,640,480\n",
		appName, appName);
}

int32_t main(int32_t argc, char *argv[])
{
    int32_t opt;
    char *strInFile = NULL;
	char *strOutFile = NULL;
	int32_t inWidth, inHeight;
	int32_t outWidth, outHeight;
	int32_t cropX=0, cropY=0, cropWidth=0, cropHeight=0;

    while (-1 != (opt = getopt(argc, argv, "i:o:hc:d:s:")))
    {
        switch (opt)
        {
        case 'i':
			strInFile = strdup(optarg);
			break;
		case 'o':
			strOutFile = strdup(optarg);
			break;
		case 's':
			sscanf(optarg, "%d,%d", &inWidth, &inHeight);
			break;
		case 'd':
			sscanf(optarg, "%d,%d", &outWidth, &outHeight);
			break;
		case 'c':
			sscanf(optarg, "%d,%d,%d,%d", &cropX, &cropY, &cropWidth, &cropHeight);
			break;
        case 'h':
            print_usage(argv[0]);
            return 0;
        default:
            break;
        }
    }

	if( NULL==strInFile || NULL==strOutFile || 0>=inWidth || 0>=inHeight || 0>=outWidth || 0>=outHeight )
	{
		printf("Error : Invalid arguments!!!");
		print_usage(argv[0]);
		return -1;
	}

	if( cropWidth==0 && cropHeight==0 )
	{
		cropX = 0;
		cropY = 0;
		cropWidth = inWidth;
		cropHeight = inHeight;
	}

	//	Allocate Input Image
	NX_VID_MEMORY_HANDLE hInMem;
	hInMem = NX_AllocateVideoMemory(inWidth, inHeight, 3, V4L2_PIX_FMT_YUV420, 4096); //	page table align
	NX_MapVideoMemory(hInMem);
	//	Allocate Output Image
	NX_VID_MEMORY_HANDLE hOutMem;
	hOutMem = NX_AllocateVideoMemory(outWidth, outHeight, 3, V4L2_PIX_FMT_YUV420, 4096); //	page table align
	NX_MapVideoMemory(hOutMem);

	printf("Allocation Done!!\n");

	if (0 != LoadInputImage(hInMem, (const char *)strInFile))
	{
		printf( "Error : LoadInputImage !!!\n" );
		goto ErrorExit;
	}
	printf("LoadInputImage Done!!\n");
	if (0 != DoScaling(hInMem, hOutMem, cropX, cropY, cropWidth, cropHeight))
	{
		printf("Error : DoScaling !!!\n");
		goto ErrorExit;
	}
	printf("DoScaling Done!!\n");
	if (0 != SaveOutputImage(hOutMem, (const char *)strOutFile))
	{
		printf("Error : SaveOutputImage !!\n");
		goto ErrorExit;
	}
	printf("Done!!!\n");

ErrorExit:
    return 0;
}
