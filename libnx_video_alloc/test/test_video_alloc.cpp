#include <stdio.h>
#include <ctype.h>	//	isprint
#include <memory.h>	//	memset
#include <nx_video_alloc.h>


static void HexDump( const void *data, int32_t size )
{
	int32_t i=0, offset = 0;
	char tmp[32];
	static char lineBuf[1024];
	const uint8_t *_data = (const uint8_t*)data;
	while( offset < size )
	{
		sprintf( lineBuf, "%08lx :  ", (unsigned long)offset );
		for( i=0 ; i<16 ; ++i )
		{
			if( i == 8 ){
				strcat( lineBuf, " " );
			}
			if( offset+i >= size )
			{
				strcat( lineBuf, "   " );
			}
			else{
				sprintf(tmp, "%02x ", _data[offset+i]);
				strcat( lineBuf, tmp );
			}
		}
		strcat( lineBuf, "   " );

		//     Add ACSII A~Z, & Number & String
		for( i=0 ; i<16 ; ++i )
		{
			if( offset+i >= size )
			{
				break;
			}
			else{
				if( isprint(_data[offset+i]) )
				{
					sprintf(tmp, "%c", _data[offset+i]);
					strcat(lineBuf, tmp);
				}
				else
				{
					strcat( lineBuf, "." );
				}
			}
		}

		strcat(lineBuf, "\n");
		printf( "%s", lineBuf );
		offset += 16;
	}
}


int main(int argc, char *argv[])
{
	//	Test 1D Allocation
	if(0)
	{
		int32_t size = 1024;
		NX_MEMORY_INFO *pMemInfo = NX_AllocateMemory( size, 1024 );
		if( pMemInfo )
		{
			printf("File Handle = %d\n", pMemInfo->fd );

			if( 0 != NX_MapMemory( pMemInfo ) )
			{
				printf("Memory Mapping Failed\n");
				return -1;
			}
			char *pBuf = (char *)pMemInfo->pBuffer;
			for( int32_t i=0 ; i <size ; i++ )
			{
				pBuf[i] = i % 256;
			}

			HexDump( pBuf, size );

			NX_UnmapMemory( pMemInfo );
			NX_FreeMemory( pMemInfo );
		}
	}

	//	Test Video Memory Allocation 2 Plane
	if(1)
	{
		int32_t width = 64;
		int32_t height = 32;
		int32_t planes = 3;

		NX_VID_MEMORY_INFO *pVidMem = NX_AllocateVideoMemory( width, height, planes, 0, 1024 );

		if( pVidMem )
		{
			printf("File Handle = %d\n", pVidMem->fd[0] );

			if( 0 != NX_MapVideoMemory( pVidMem ) )
			{
				printf("Video Memory Mapping Failed\n");
				return -1;
			}

			for( int32_t j=0 ; j < pVidMem->planes ; j++ )
			{
				printf("\n================== Planes %d (%d) ===================\n", j, pVidMem->size[j]);
				char *pBuf = (char *)(pVidMem->pBuffer[j]);
				for( int32_t i=0 ; i < pVidMem->size[j] ; i++ )
				{
					pBuf[i] = i % 256;
				}
				HexDump( pBuf, pVidMem->size[j] );
			}

			NX_UnmapVideoMemory( pVidMem );
			NX_FreeVideoMemory( pVidMem );
		}
	}

	return 0;
}
