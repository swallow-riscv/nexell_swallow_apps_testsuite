#include <ctype.h>
#include <unistd.h>

#include <linux/videodev2.h>


//  FFMPEG Headers
#ifdef __cplusplus
 #define __STDC_CONSTANT_MACROS
 #ifdef _STDINT_H
  #undef _STDINT_H
 #endif
 # include <stdint.h>
#endif

#ifdef __cplusplus
extern "C"{
#endif
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/pixdesc.h"
#include "libavdevice/avdevice.h"
#ifdef __cplusplus
}
#endif

#ifndef MKTAG
#define MKTAG(a,b,c,d) (a | (b << 8) | (c << 16) | (d << 24))
#endif


/*int fourCCToCodStd(unsigned int fourcc)
{
	int codStd = -1;
	unsigned int i;

	char str[5];

	str[0] = toupper((char)fourcc);
	str[1] = toupper((char)(fourcc>>8));
	str[2] = toupper((char)(fourcc>>16));
	str[3] = toupper((char)(fourcc>>24));
	str[4] = '\0';

	for(i=0; i<sizeof(codstd_tab)/sizeof(codstd_tab[0]); i++)
	{
		if (codstd_tab[i].fourcc == (unsigned int)MKTAG(str[0], str[1], str[2], str[3]))
		{
			codStd = codstd_tab[i].codStd;
			break;
		}
	}

	return codStd;
}

int codecIdToCodStd(int codec_id)
{
	int codStd = -1;
	unsigned int i;

	for(i=0; i<sizeof(codstd_tab)/sizeof(codstd_tab[0]); i++)
	{
		if (codstd_tab[i].codec_id == codec_id)
		{
			codStd = codstd_tab[i].codStd;
			break;
		}
	}
	return codStd;
}

int codecIdToFourcc(int codec_id)
{
	int fourcc = 0;
	unsigned int i;

	for(i=0; i<sizeof(codstd_tab)/sizeof(codstd_tab[0]); i++)
	{
		if (codstd_tab[i].codec_id == codec_id)
		{
			fourcc = codstd_tab[i].fourcc;
			break;
		}
	}
	return fourcc;
}*/

unsigned int CodecIdToV4l2Type( int codecId, unsigned int fourcc )
{
	unsigned int v4l2CodecType = -1;

	if( codecId == CODEC_ID_MPEG4 || codecId == CODEC_ID_FLV1 )
	{
		v4l2CodecType = V4L2_PIX_FMT_MPEG4;
	}
	/*else if( codecId == CODEC_ID_MSMPEG4V3 )
	{
		switch( fourcc )
		{
			case MKTAG('D','I','V','3'):
			case MKTAG('M','P','4','3'):
			case MKTAG('M','P','G','3'):
			case MKTAG('D','V','X','3'):
			case MKTAG('A','P','4','1'):
				vpuCodecType = NX_DIV3_DEC;
				break;
			default:
				vpuCodecType = NX_MP4_DEC;
				break;
		}
	}*/
	else if( codecId == CODEC_ID_H263 || codecId == CODEC_ID_H263P || codecId == CODEC_ID_H263I )
	{
		v4l2CodecType = V4L2_PIX_FMT_H263;
	}
	else if( codecId == CODEC_ID_H264 )
	{
		v4l2CodecType = V4L2_PIX_FMT_H264;
	}
	else if( codecId == CODEC_ID_MPEG2VIDEO )
	{
		v4l2CodecType = V4L2_PIX_FMT_MPEG2;
	}
	/*else if( (codecId == CODEC_ID_WMV3) || (codecId == CODEC_ID_VC1) )
	{
		vpuCodecType = NX_VC1_DEC;
	}
	else if( (codecId == CODEC_ID_RV30) || (codecId == CODEC_ID_RV40) )
	{
		vpuCodecType = NX_RV_DEC;
	}
	else if( codecId == CODEC_ID_THEORA )
	{
		vpuCodecType = NX_THEORA_DEC;
	}
	else if( codecId == CODEC_ID_VP8 )
	{
		v4l2CodecType = V4L2_PIX_FMT_VP8;
	}*/
	else if( codecId == CODEC_ID_MJPEG )
	{
		v4l2CodecType = V4L2_PIX_FMT_MJPEG;
	}
	/*else if( codecId == AV_CODEC_ID_HEVC )
	{
		vpuCodecType = NX_HEVC_DEC;
	}*/
	else
	{
		printf("Cannot support codecid(%d) (0x %x) \n", codecId, codecId );
		exit(-1);
	}
	
	return v4l2CodecType;
}