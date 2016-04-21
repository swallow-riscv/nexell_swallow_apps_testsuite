#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>		    //	getopt & optarg
#include <stdlib.h>         //	atoi
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>		//	gettimeofday
#include <math.h>

#include <linux/videodev2.h>
#include <linux/videodev2_nxp_media.h>

#include <nx_video_alloc.h>
#include <nx_fourcc.h>

#include <nx_video_api.h>

#include "Util.h"

static int32_t LoadImage( uint8_t *pSrc, int32_t w, int32_t h, NX_VID_MEMORY_INFO *pImg )
{
	int32_t i, j;
	uint8_t *pDst/*, *pCb, *pCr*/;

	//	Copy Lu
	pDst = (uint8_t *)pImg->pBuffer[0];
	for( i=0 ; i<h ; i++ )
	{
		memcpy(pDst, pSrc, w);
		pDst += pImg->stride[0];
		pSrc += w;
	}

	//pCb = pSrc;
	//pCr = pSrc + w*h/4;

	/*switch( pImg->format )
	{
		case FOURCC_NV12:
		{
			uint8_t *pCbCr;
			pDst = (uint8_t*)pImg->cbVirAddr;
			for( i=0 ; i<h/2 ; i++ )
			{
				pCbCr = pDst + pImg->cbStride*i;
				for( j=0 ; j<w/2 ; j++ )
				{
					*pCbCr++ = *pCb++;
					*pCbCr++ = *pCr++;
				}
			}
			break;
		}
		case FOURCC_NV21:
		{
			uint8_t *pCrCb;
			pDst = (uint8_t*)pImg->cbVirAddr;
			for( i=0 ; i<h/2 ; i++ )
			{
				pCrCb = pDst + pImg->cbStride*i;
				for( j=0 ; j<w/2 ; j++ )
				{
					*pCrCb++ = *pCr++;
					*pCrCb++ = *pCb++;
				}
			}
			break;
		}
		case FOURCC_MVS0:
		case FOURCC_YV12:
		case FOURCC_IYUV:*/
		{
			for(j=1 ; j<3 ; j++)
			{
				pDst = (uint8_t *)pImg->pBuffer[j];
				for( i=0 ; i<h/2 ; i++ )
				{
					memcpy(pDst, pSrc, w/2);
					pDst += pImg->stride[j];
					pSrc += w/2;
				}
			}
			/*break;
		}*/
	}
	return 0;
}

#define MAX_BUFFER_NUM		8

int32_t VpuEncMain( CODEC_APP_DATA *pAppData )
{
	NX_V4L2ENC_HANDLE hEnc = NULL;
	NX_V4L2ENC_PARAM encParam;

	int32_t i, ret = -1;
	NX_VID_MEMORY_HANDLE hMem[MAX_BUFFER_NUM];
	NX_VID_MEMORY_HANDLE pImage;
	int32_t index = 0;

	int32_t frameCnt = 0;
	int32_t readSize = 0;
	uint8_t *pSrcBuf = NULL;
	uint8_t *pSeqData;
	int32_t seqSize;

	//	Input Image
	int32_t inWidth = pAppData->width;
	int32_t inHeight = pAppData->height;

	//	In/Out/Log File Open
	FILE *fdIn = fopen( pAppData->inFileName, "rb" );
	FILE *fdOut = fopen( pAppData->outFileName, "wb" );

	if( fdIn == NULL || fdOut == NULL )
	{
		printf("Fail, Input or Output file open.\n");
		return -1;;
	}

	if( !pAppData->inFileName )
	{
		printf("Fail, Input File Name.\n");
		return -1;
	}

	if( pAppData->codec == 0 )
	{
		pAppData->codec = V4L2_PIX_FMT_H264;
	}
	else if( pAppData->codec == 1 )
	{
		pAppData->codec = V4L2_PIX_FMT_MPEG4;
	}
	else if( pAppData->codec == 2 )
	{
		pAppData->codec = V4L2_PIX_FMT_H263;
	}
	// else if( pAppData->codec == 3 )
	// {
	// 	pAppData->codec = NX_JPEG_ENC;
	// }
	else
	{
		printf("Fail, Not Supported Codec.\n");
		goto ENC_TERMINATE;
	}

	hEnc = NX_V4l2EncOpen( pAppData->codec );
	if( NULL == hEnc )
	{
		printf("Fail, NX_V4l2EncOpen().\n");
		goto ENC_TERMINATE;
	}

	memset( &encParam, 0, sizeof(encParam) );
	encParam.width		= pAppData->width;
	encParam.height		= pAppData->height;
	encParam.bitrate 	= pAppData->kbitrate;
	encParam.initialQp	= pAppData->qp;

	ret = NX_V4l2EncInit( hEnc, &encParam );
	if( VID_ERR_NONE != ret )
	{
		printf("Fail, NX_V4l2EncInit().\n");
		goto ENC_TERMINATE;
	}

	ret = NX_V4l2EncGetSeqInfo( hEnc, &pSeqData, &seqSize );
	if( VID_ERR_NONE != ret )
	{
		printf("Fail, NX_V4l2EncGetSeqInfo().\n");
		goto ENC_TERMINATE;
	}
	
	// Write Sequence Data
	if( NULL != fdOut )
	{
		fwrite( (void *)pSeqData, 1, seqSize, fdOut );
	}

	// Allocate Input Buffer
	pSrcBuf = (uint8_t*)malloc( inWidth * inHeight * 3 / 2 );

	// Allocate Output buffer
	for( i = 0; i < MAX_BUFFER_NUM; i++ )
	{
		hMem[i] = NX_AllocateVideoMemory( inWidth, inHeight, 3, 0, 4096 );
		if( hMem[i] )
		{
			if( 0 != NX_MapVideoMemory( hMem[i] ) )
			{
				printf("Fail, NX_MapVideoMemory().\n");
				goto ENC_TERMINATE;
			}
		}
		else
		{
			printf("Fail, NX_AllocateVideoMemory().\n");
			goto ENC_TERMINATE;
		}
	}

	while(1)
	{
		NX_V4L2ENC_IN encIn;
		NX_V4L2ENC_OUT encOut;

		pImage = hMem[index];
		index = (index + 1) % MAX_BUFFER_NUM;

		if( fdIn )
		{
			readSize = fread(pSrcBuf, 1, inWidth*inHeight*3/2, fdIn);
			if( readSize != inWidth*inHeight*3/2 || readSize == 0 )
			{
				printf("End of Sttream.\n");
				break;
			}
		}
		
		LoadImage( pSrcBuf, inWidth, inHeight, pImage );

		memset( &encIn, 0, sizeof(encIn) );
		encIn.pImage = pImage;

		NX_V4l2EncEncodeFrame( hEnc, &encIn, &encOut );

		if( fdOut && encOut.bufSize > 0 )
		{
			fwrite( (void *)encOut.outBuf, 1, encOut.bufSize, fdOut );
		}

		printf("[%d] Encoded Frame. ( %d )\n", frameCnt, encOut.bufSize);
		frameCnt++;
	}

	ret = 0;

ENC_TERMINATE:
	if( pSrcBuf )
	{
		free( pSrcBuf );
	}

	for( i = 0; i < MAX_BUFFER_NUM; i++ )
	{
		if( hMem[i] )
		{
			NX_FreeVideoMemory( hMem[i] );
		}
	}

	if( fdIn )
	{
		fclose( fdIn );
	}

	if( fdOut )
	{
		fclose( fdOut );
	}

	if( hEnc )
	{
		NX_V4l2EncClose( hEnc );	
	}

	printf("Encoder End!!\n" );
	return 0;
}