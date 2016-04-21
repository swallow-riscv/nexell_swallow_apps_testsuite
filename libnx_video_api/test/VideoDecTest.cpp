#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>		    //	getopt & optarg

#include <sys/stat.h>
#include <sys/ioctl.h>

#include <nx_video_alloc.h>
#include <nx_fourcc.h>

#include "MediaExtractor.h"
#include "CodecInfo.h"
#include "Util.h"

#include <nx_video_api.h>

//#include <sys/mman.h>

unsigned char streamBuffer[4*1024*1024];

int32_t VpuDecMain( CODEC_APP_DATA *pAppData )
{
	int32_t ret;
	VPU_ERROR_E vidRet;

	int32_t seqSize = 0;
	int32_t bInit=0;
	int32_t frameCount=0;
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

	pMediaReader->GetVideoResolution(&imgWidth, &imgHeight);
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
        printf("failed to open video decoder device\n");
		exit(-1);
	}

	while(1)
	{
		//	ReadStream
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
			// Header Parser
			NX_V4L2DEC_SEQ_IN seqIn;
			NX_V4L2DEC_SEQ_OUT	seqOut;

			memset( &seqIn, 0, sizeof(seqIn) );
			seqIn.seqInfo	= streamBuffer;
			seqIn.seqSize	= seqSize;
			seqIn.width		= imgWidth;			// optional
			seqIn.height	= imgHeight;

			vidRet =NX_V4l2DecParseVideoCfg( hDec, &seqIn, &seqOut );
			if ( vidRet != VID_ERR_NONE )
			{
				printf("Parser Fail\n");
				goto DEC_TERMINATE;
			}

			// seqIn.width = seqOut.width;
			// seqIn.height = seqOut.height;
			
			vidRet = NX_V4l2DecInit( hDec, &seqIn );
			if( vidRet != VID_ERR_NONE )
			{
				printf("VPU Initialize Failed!!!\n");
				goto DEC_TERMINATE;
			}

			seqSize = 0;
			//size = 0;

			bInit = 1;
		}
		if( size == 0 )
			break;

		NX_V4L2DEC_IN decIn;
		NX_V4L2DEC_OUT decOut;

		decIn.strmBuf = streamBuffer;
		decIn.strmSize = size;
		NX_V4l2DecDecodeFrame( hDec, &decIn, &decOut );
		if( 0 <= decOut.outImgIdx )
		{
			if( fpOut ) {
				fwrite( (void*)decOut.outImg.pBuffer[0], 1, decOut.outImg.stride[0] * decOut.outImg.height, fpOut );
				fwrite( (void*)decOut.outImg.pBuffer[1], 1, decOut.outImg.stride[1] * decOut.outImg.height / 2, fpOut );
				fwrite( (void*)decOut.outImg.pBuffer[2], 1, decOut.outImg.stride[2] * decOut.outImg.height / 2, fpOut );
			}

			NX_V4l2DecClrDspFlag( hDec, NULL, decOut.outImgIdx );
		}

		frameCount++;
	}
	ret = 0;

	//==============================================================================
	// TERMINATION
	//==============================================================================
DEC_TERMINATE:
	if( hDec ) NX_V4l2DecClose( hDec );
	if( fpOut ) fclose(fpOut);

	return ret;
}
