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

#include <linux/videodev2.h>

#include <nx_video_alloc.h>
#include <nx_video_api.h>

#include "MediaExtractor.h"
#include "CodecInfo.h"
#include "Util.h"

#include <videodev2_nxp_media.h>

#ifdef ANDROID
#include "NX_AndroidRenderer.h"
#define	WINDOW_WIDTH			800 // 1024
#define	WINDOW_HEIGHT			480 // 600
#define	NUMBER_OF_BUFFER		12
#define	MAX_NUMBER_OF_BUFFER		12
#endif

#define ENABLE_ASPECT_RATIO
#define SCREEN_WIDTH	(1080)
#define SCREEN_HEIGHT	(1920)

#if ENABLE_DRM_DISPLAY
#include <drm_fourcc.h>
#include "DrmRender.h"
#endif

//#define ENABLE_CBCR_INTERLEAVE

#define IMG_PLANE_NUM	3
#define PLANE_ID		26
#define CRTC_ID			31

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
	NX_V4L2DEC_HANDLE hDec = NULL;
#if ENABLE_DRM_DISPLAY
	DRM_DSP_HANDLE hDsp = NULL;
#endif
	uint8_t streamBuffer[4*1024*1024];
	int32_t ret, seqflg = 0;
	int32_t imgWidth = -1, imgHeight = -1;

	CMediaReader *pMediaReader = new CMediaReader();
	if (!pMediaReader->OpenFile( pAppData->inFileName))
	{
		printf("Cannot open media file(%s)\n", pAppData->inFileName);
		exit(-1);
	}
	pMediaReader->GetVideoResolution(&imgWidth, &imgHeight);

	register_signal();

	//==============================================================================
	// DISPLAY INITIALIZATION
	//==============================================================================
	{
#if ENABLE_DRM_DISPLAY
		int drmFd = open("/dev/dri/card0", O_RDWR);

		hDsp = CreateDrmDisplay(drmFd);
		DRM_RECT srcRect, dstRect;

		srcRect.x = 0;
		srcRect.y = 0;
		srcRect.width = imgWidth;
		srcRect.height = imgHeight;
		dstRect.x = 0;
		dstRect.y = 0;
		dstRect.width = imgWidth;
		dstRect.height = imgHeight;

#ifdef ENABLE_ASPECT_RATIO
		double xRatio = (double)SCREEN_WIDTH/(double)imgWidth;
		double yRatio = (double)SCREEN_HEIGHT/(double)imgHeight;

		if (xRatio > yRatio)
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

		InitDrmDisplay(hDsp, PLANE_ID, CRTC_ID, DRM_FORMAT_YUV420, srcRect, dstRect);
#endif	//	ENABLE_DRM_DISPLAY
	}

#ifdef ANDROID
	CNX_AndroidRenderer *pAndRender = new CNX_AndroidRenderer(WINDOW_WIDTH, WINDOW_HEIGHT);
	NX_VID_MEMORY_HANDLE *pMemHandle = NULL;
#endif

	uint32_t v4l2CodecType;
	int32_t fourcc = -1, codecId = -1;

	pMediaReader->GetCodecTagId(AVMEDIA_TYPE_VIDEO, &fourcc, &codecId);
	v4l2CodecType = CodecIdToV4l2Type(codecId, fourcc);

	hDec = NX_V4l2DecOpen(v4l2CodecType);
	if (hDec == NULL)
	{
		printf("Fail, NX_V4l2DecOpen().\n");
		return -1;
	}

	//==============================================================================
	// PROCESS UNIT
	//==============================================================================
	{
		int32_t bInit = false, bSeek = false;

		int frmCnt = 0, size = 0;
		uint64_t startTime, endTime, totalTime = 0;
		uint64_t prevTime = 0;
		int64_t timeStamp = -1;

		FILE *fpOut = NULL;
		int32_t prvIndex = -1;
		int32_t bIsSeek = 0;

		int32_t additionSize = 0;

		NX_V4L2DEC_SEQ_IN seqIn;
		NX_V4L2DEC_SEQ_OUT seqOut;

		NX_V4L2DEC_IN decIn;
		NX_V4L2DEC_OUT decOut;

		if (pAppData->outFileName)
		{
			fpOut = fopen(pAppData->outFileName, "wb");
			if (fpOut == NULL) {
				printf("output file open error!!\n");
				ret = -1;
				goto DEC_TERMINATE;
			}
		}

		while(!bExitLoop)
		{
			int32_t key = 0;

			if( !bInit )
			{
				additionSize = pMediaReader->GetVideoSeqInfo(streamBuffer);
			}

			if (pMediaReader->ReadStream(CMediaReader::MEDIA_TYPE_VIDEO, streamBuffer + additionSize, &size, &key, &timeStamp ) != 0)
			{
				size = 0;
			}

			if( !bInit && !key )
				continue;

			if( bSeek && !key )
				continue;

			if( !bInit )
			{
				memset(&seqIn, 0, sizeof(seqIn));
				seqIn.width     = imgWidth;
				seqIn.height    = imgHeight;
				seqIn.seqSize   = size + additionSize;
				seqIn.seqBuf    = streamBuffer;
				seqIn.timeStamp = timeStamp;

				if (v4l2CodecType == V4L2_PIX_FMT_MJPEG)
					seqIn.thumbnailMode = 0;

				ret = NX_V4l2DecParseVideoCfg(hDec, &seqIn, &seqOut);
				if (ret < 0)
				{
					printf("Fail, NX_V4l2DecParseVideoCfg()\n");
					goto DEC_TERMINATE;
				}

				seqIn.width = seqOut.width;
				seqIn.height = seqOut.height;
				seqIn.numBuffers = 3;
				seqIn.imgPlaneNum = IMG_PLANE_NUM;

#ifdef ANDROID
				seqIn.numBuffers = seqOut.minBuffers + 3;
				pAndRender->GetBuffers(seqIn.numBuffers, imgWidth, imgHeight, &pMemHandle );
				NX_VID_MEMORY_HANDLE hVideoMemory[MAX_NUMBER_OF_BUFFER];
				for( int32_t i=0 ; i<seqIn.numBuffers ; i++ )
				{
					hVideoMemory[i] = pMemHandle[i];
				}
				seqIn.pMemHandle = &hVideoMemory[0];
#endif

#ifdef ENABLE_CBCR_INTERLEAVE
				if (seqOut.imgFourCC == V4L2_PIX_FMT_YUV420M)
					seqIn.imgFormat = V4L2_PIX_FMT_NV12M;
				else if (seqOut.imgFourCC == V4L2_PIX_FMT_YUV422M)
					seqIn.imgFormat = V4L2_PIX_FMT_NV16M;
				else if (seqOut.imgFourCC == V4L2_PIX_FMT_YUV444M)
					seqIn.imgFormat = V4L2_PIX_FMT_NV24M;
				else
					seqIn.imgFormat	= seqOut.imgFourCC;
#else
				seqIn.imgFormat	= seqOut.imgFourCC;
#endif

				ret = NX_V4l2DecInit(hDec, &seqIn);
				if (ret < 0)
				{
					printf("File, NX_V4l2DecInit().\n");
					goto DEC_TERMINATE;
				}

				bInit = true;
			}

			if( bSeek )
			{
				additionSize += size;
				bSeek = false;
				continue;
			}

 			if (frmCnt == pAppData->iSeekStartFrame && pAppData->iSeekStartFrame)
			{
				printf("Seek. ( frmCnt(%d frm), seekPos(%d sec) )\n", frmCnt, pAppData->iSeekPos);
				usleep(1000000);

				pMediaReader->SeekStream( pAppData->iSeekPos * 1000 );
#if 1
				do {
					decIn.strmBuf   = 0;
					decIn.strmSize  = 0;
					decIn.timeStamp = 0;
					decIn.eos       = 1;

					ret = NX_V4l2DecDecodeFrame( hDec, &decIn, &decOut );
					if( !ret && decOut.dispIdx >= 0 )
						NX_V4l2DecClrDspFlag( hDec, NULL, decOut.dispIdx );
				} while( !ret && decOut.dispIdx >= 0 );

				if( prvIndex >= 0 )
					NX_V4l2DecClrDspFlag( hDec, NULL, prvIndex );
#else
				NX_V4l2DecFlush( hDec );
#endif
				prvIndex = -1;
				additionSize = 0;
				bSeek = true;
				frmCnt++;
				continue;
			}

			memset(&decIn, 0, sizeof(NX_V4L2DEC_IN));
			decIn.strmBuf   = streamBuffer;
			decIn.strmSize  = size + additionSize;
			decIn.timeStamp = timeStamp;
			decIn.eos       = (size == 0) ? (1) : (0);

			startTime       = NX_GetTickCount();
			ret             = NX_V4l2DecDecodeFrame(hDec, &decIn, &decOut);
			endTime         = NX_GetTickCount();
			totalTime       += (endTime - startTime);

			printf("[%5d Frm] Key=%d Size=%6d, DecIdx=%2d, DispIdx=%2d, InTimeStamp=%7llu, outTimeStamp=%7llu, Time=%6llu, interlace=%1d %1d, Reliable=%3d, %3d, type = %d, %d, UsedByte=%5d\n",
				frmCnt, key, size, decOut.decIdx, decOut.dispIdx, timeStamp, decOut.timeStamp[DISPLAY_FRAME], (endTime - startTime), decOut.interlace[DECODED_FRAME], decOut.interlace[DISPLAY_FRAME],
				decOut.outFrmReliable_0_100[DECODED_FRAME], decOut.outFrmReliable_0_100[DISPLAY_FRAME], decOut.picType[DECODED_FRAME], decOut.picType[DISPLAY_FRAME], decOut.usedByte);
			/*printf("%2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x\n",
				streamBuffer[0], streamBuffer[1], streamBuffer[2], streamBuffer[3], streamBuffer[4], streamBuffer[5], streamBuffer[6], streamBuffer[7],
				streamBuffer[8], streamBuffer[9], streamBuffer[10], streamBuffer[11], streamBuffer[12], streamBuffer[13], streamBuffer[14], streamBuffer[15]);*/

			if (ret < 0)
			{
				printf("Fail, NX_V4l2DecDecodeFrame().\n");
				break;
			}

			if (decOut.dispIdx >= 0)
			{
				if (fpOut)
				{
					int i;

					for (i=0 ; i<decOut.hImg.planes; i++)
						fwrite((void*)decOut.hImg.pBuffer[i], 1, decOut.hImg.size[i], fpOut);
				}

#if ENABLE_DRM_DISPLAY
				UpdateBuffer(hDsp, &decOut.hImg, NULL);
#endif

#ifdef ANDROID
				pAndRender->DspQueueBuffer( NULL, decOut.dispIdx );
				if( prvIndex != -1 )
				{
					pAndRender->DspDequeueBuffer(NULL, NULL);
				}
#endif

				if( prvIndex >= 0 )
				{
					ret = NX_V4l2DecClrDspFlag(hDec, NULL, prvIndex);
					if (ret < 0)
						break;
				}

				prvIndex = decOut.dispIdx;
			}

			frmCnt++;
			additionSize = 0;
		}

		if (fpOut)
			fclose(fpOut);
	}

	//==============================================================================
	// TERMINATION
	//==============================================================================
DEC_TERMINATE:
	if (hDec)
		ret = NX_V4l2DecClose(hDec);

#ifdef ANDROID
	if( pAndRender )
		delete pAndRender;
#endif

	if (pMediaReader)
		delete pMediaReader;

	printf("Decode End!!(ret = %d)\n", ret);
	return ret;
}
