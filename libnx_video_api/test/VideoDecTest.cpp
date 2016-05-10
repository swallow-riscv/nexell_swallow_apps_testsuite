//------------------------------------------------------------------------------
//
//	Copyright (C) 2016 Nexell Co. All Rights Reserved
//	Nexell Co. Proprietary & Confidential
//
//	NEXELL INFORMS THAT THIS CODE AND INFORMATION IS PROVIDED "AS IS" BASE
//  AND	WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING
//  BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS
//  FOR A PARTICULAR PURPOSE.
//
//	Module		:
//	File		:
//	Description	:
//	Author		: 
//	Export		:
//	History		:
//
//------------------------------------------------------------------------------

#include <stdio.h>

#include <sys/types.h>	//	open
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#include <unistd.h>

#include <nx_fourcc.h>
#include <nx_video_alloc.h>
#include <nx_video_api.h>

#include "MediaExtractor.h"
#include "CodecInfo.h"
#include "Util.h"

#define ENABLE_DRM_DISPLAY
#define ENABLE_ASPECT_RATIO
#define SCREEN_WIDTH	(1080)
#define SCREEN_HEIGHT	(1920)

#ifdef ENABLE_DRM_DISPLAY
#include <drm_fourcc.h>
#include "DrmRender.h"
#endif

static unsigned char streamBuffer[4*1024*1024];
static bool bExitLoop = false;

//----------------------------------------------------------------------------------------------------
//
//	Signal Handler
//
static void signal_handler( int32_t sig )
{
	printf("Aborted by signal %s (%d)..\n", (char*)strsignal(sig), sig);

	switch( sig )
	{
		case SIGINT :
			printf("SIGINT..\n"); 	break;
		case SIGTERM :
			printf("SIGTERM..\n");	break;
		case SIGABRT :
			printf("SIGABRT..\n");	break;
		default :
			break;
	}

	if( !bExitLoop )
		bExitLoop = true;
	else{
		usleep(1000000);	//	wait 1 seconds for double Ctrl+C operation
		exit(EXIT_FAILURE);
	}
}

static void register_signal( void )
{
	signal( SIGINT,  signal_handler );
	signal( SIGTERM, signal_handler );
	signal( SIGABRT, signal_handler );
}

//------------------------------------------------------------------------------
int32_t VpuDecMain( CODEC_APP_DATA *pAppData )
{
	int32_t ret;
	VPU_ERROR_E vidRet;

	int32_t seqSize = 0;
	int32_t bInit=0;
	int32_t frameCnt=0;
	int32_t size, key = 0;
	int64_t timeStamp = -1;
	int32_t codecTag=-1, codecId=-1;
	int32_t imgWidth=-1, imgHeight=-1;
	uint32_t v4l2CodecType;
	FILE *fpOut = NULL;

	NX_V4L2DEC_HANDLE hDec = NULL;


	register_signal();

	int drmFd = open("/dev/dri/card0", O_RDWR);

#ifdef ENABLE_DRM_DISPLAY
	DRM_DSP_HANDLE hDsp = CreateDrmDisplay( drmFd );
	DRM_RECT srcRect, dstRect;
#endif	//	ENABLE_DRM_DISPLAY

	SetDRMFd( drmFd );

	CMediaReader *pMediaReader = new CMediaReader();
	if( !pMediaReader->OpenFile( pAppData->inFileName ) )
	{
		printf("Cannot open media file(%s)\n", pAppData->inFileName);
		exit(-1);
	}

	pMediaReader->GetVideoResolution( &imgWidth, &imgHeight );
	if( pAppData->outFileName )
	{
		fpOut = fopen( pAppData->outFileName, "wb" );
	}


#ifdef ENABLE_DRM_DISPLAY
	srcRect.x=0;
	srcRect.y=0;
	srcRect.width=imgWidth;
	srcRect.height=imgHeight;
	dstRect.x=0;
	dstRect.y=0;
	dstRect.width=imgWidth;
	dstRect.height=imgHeight;

#ifdef ENABLE_ASPECT_RATIO
	double xRatio = (double)SCREEN_WIDTH/(double)imgWidth;
	double yRatio = (double)SCREEN_HEIGHT/(double)imgHeight;
	if( xRatio > yRatio )
	{
		dstRect.width = imgWidth * yRatio;
		dstRect.height = SCREEN_HEIGHT;
		dstRect.x = abs(SCREEN_WIDTH - dstRect.width)/2;
	}
	else
	{
		dstRect.width = SCREEN_WIDTH;
		dstRect.height = imgHeight * xRatio;
		dstRect.y = abs(SCREEN_HEIGHT - dstRect.height)/2;
	}
#endif

	InitDrmDisplay(hDsp, 17, 22, DRM_FORMAT_YUV420, dstRect, srcRect );
#endif	//	ENABLE_DRM_DISPLAY

	pMediaReader->GetCodecTagId( AVMEDIA_TYPE_VIDEO, &codecTag, &codecId  );
	v4l2CodecType = CodecIdToV4l2Type(codecId, codecTag);

	seqSize = pMediaReader->GetVideoSeqInfo( streamBuffer );

	hDec = NX_V4l2DecOpen( v4l2CodecType );
	if( hDec == NULL )
	{
		printf("Fail, NX_V4l2DecOpen().\n");
		exit(-1);
	}

	while(!bExitLoop)
	{
		// Read Stream
		if( pMediaReader->ReadStream( CMediaReader::MEDIA_TYPE_VIDEO, streamBuffer+seqSize, &size, &key, &timeStamp ) != 0 )
		{
			size = 0;
		}

		if( !bInit && !key )
		{
			continue;
		}

		if( !bInit )
		{
			/* Parse Header */
			NX_V4L2DEC_SEQ_IN seqIn;
			NX_V4L2DEC_SEQ_OUT	seqOut;

			memset( &seqIn, 0, sizeof(seqIn) );
			seqIn.seqInfo	= streamBuffer;
			seqIn.seqSize	= seqSize;
			seqIn.width		= imgWidth;
			seqIn.height	= imgHeight;

			vidRet = NX_V4l2DecParseVideoCfg( hDec, &seqIn, &seqOut );
			if( VID_ERR_NONE != vidRet )
			{
				printf("Fali, NX_V4l2DecParseVideoCfg().\n");
				goto DEC_TERMINATE;
			}

			/*
			seqIn.width = seqOut.width;
			seqIn.height = seqOut.height;
			*/

			vidRet = NX_V4l2DecInit( hDec, &seqIn );
			if( VID_ERR_NONE != vidRet )
			{
				printf("File, NX_V4l2DecInit().\n");
				goto DEC_TERMINATE;
			}

			seqSize = 0;
			bInit = 1;
		}

		NX_V4L2DEC_IN decIn;
		NX_V4L2DEC_OUT decOut;

		decIn.strmBuf = streamBuffer;
		decIn.strmSize = size;
		decIn.timeStamp = timeStamp;
		decIn.eos = (size == 0) ? 1 : 0;

		vidRet = NX_V4l2DecDecodeFrame( hDec, &decIn, &decOut );
		if( VID_ERR_NONE != vidRet )
		{
			printf("Fail, NX_V4l2DecDecodeFrame().\n");
			break;
		}

		// printf("Frame[%5d]: size=%6d, DecIdx=%2d, DspIdx=%2d, InTimeStamp=%7lu, outTimeStamp=%7lu, %7lu, interlace=%1d(%2d), Reliable=%3d, %3d, type = %d, %d\n",
		// 	frameCnt, decIn.strmSize, decOut.outDecIdx, decOut.outImgIdx, decIn.timeStamp, decOut.timeStamp[FIRST_FIELD], decOut.timeStamp[SECOND_FIELD], decOut.isInterlace, decOut.topFieldFirst,
		// 	decOut.outFrmReliable_0_100[DECODED_FRAME], decOut.outFrmReliable_0_100[DISPLAY_FRAME], decOut.picType[DECODED_FRAME], decOut.picType[DISPLAY_FRAME] );

		if( 0 <= decOut.outImgIdx )
		{
			if( fpOut ) {
				printf("Width(%d), Height(%d)\n", decOut.outImg.width, decOut.outImg.height);
				fwrite( (void*)decOut.outImg.pBuffer[0], 1, decOut.outImg.stride[0] * decOut.outImg.height, fpOut );
				fwrite( (void*)decOut.outImg.pBuffer[1], 1, decOut.outImg.stride[1] * decOut.outImg.height / 2, fpOut );
				fwrite( (void*)decOut.outImg.pBuffer[2], 1, decOut.outImg.stride[2] * decOut.outImg.height / 2, fpOut );
			}

#ifdef ENABLE_DRM_DISPLAY
			UpdateBuffer( hDsp, &decOut.outImg, NULL );
#endif

			vidRet = NX_V4l2DecClrDspFlag( hDec, NULL, decOut.outImgIdx );
			if( VID_ERR_NONE != vidRet )
				break;
		}

		frameCnt++;
		
		if( decIn.eos == 1 )
			break;
	}
	ret = 0;

DEC_TERMINATE:
	if( pMediaReader )
	{
		delete pMediaReader;
	}

	if( fpOut )
	{
		fclose(fpOut);
	}

	if( hDec )
	{
		NX_V4l2DecClose( hDec );
	}

	printf("Decode End!!\n");
	return ret;
}
