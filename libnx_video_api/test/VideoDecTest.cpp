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

#include <nx_fourcc.h>
#include <nx_video_alloc.h>
#include <nx_video_api.h>

#include "MediaExtractor.h"
#include "CodecInfo.h"
#include "Util.h"

unsigned char streamBuffer[4*1024*1024];

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

	pMediaReader->GetCodecTagId( AVMEDIA_TYPE_VIDEO, &codecTag, &codecId  );
	v4l2CodecType = CodecIdToV4l2Type(codecId, codecTag);

	seqSize = pMediaReader->GetVideoSeqInfo( streamBuffer );

	hDec = NX_V4l2DecOpen( v4l2CodecType );
	if( hDec == NULL )
	{
		printf("Fail, NX_V4l2DecOpen().\n");
		exit(-1);
	}

	while(1)
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

		if( size == 0 )
			break;

		NX_V4L2DEC_IN decIn;
		NX_V4L2DEC_OUT decOut;

		decIn.strmBuf = streamBuffer;
		decIn.strmSize = size;
		decIn.timeStamp = timeStamp;

		vidRet = NX_V4l2DecDecodeFrame( hDec, &decIn, &decOut );
		if( VID_ERR_NONE != vidRet )
		{
			printf("Fail, NX_V4l2DecDecodeFrame().\n");
		}

        printf("Frame[%5d]: size=%6d, DecIdx=%2d, DspIdx=%2d, InTimeStamp=%7lu, outTimeStamp=%7lu, %7lu, interlace=%1d(%2d), Reliable=%3d, %3d, type = %d, %d\n",
            frameCnt, decIn.strmSize, decOut.outDecIdx, decOut.outImgIdx, decIn.timeStamp, decOut.timeStamp[FIRST_FIELD], decOut.timeStamp[SECOND_FIELD], decOut.isInterlace, decOut.topFieldFirst,
            decOut.outFrmReliable_0_100[DECODED_FRAME], decOut.outFrmReliable_0_100[DISPLAY_FRAME], decOut.picType[DECODED_FRAME], decOut.picType[DISPLAY_FRAME] );

		if( 0 <= decOut.outImgIdx )
		{
			if( fpOut ) {
				fwrite( (void*)decOut.outImg.pBuffer[0], 1, decOut.outImg.stride[0] * decOut.outImg.height, fpOut );
				fwrite( (void*)decOut.outImg.pBuffer[1], 1, decOut.outImg.stride[1] * decOut.outImg.height / 2, fpOut );
				fwrite( (void*)decOut.outImg.pBuffer[2], 1, decOut.outImg.stride[2] * decOut.outImg.height / 2, fpOut );
			}

			NX_V4l2DecClrDspFlag( hDec, NULL, decOut.outImgIdx );
		}

		frameCnt++;
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
