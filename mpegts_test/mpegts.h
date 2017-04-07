#ifndef __MPEGTS_H__
#define	__MPEGTS_H__

#define TS_PACKET_SIZE		(188)   // Fixed, (CFG_MPEGTS_WORDCNT*4*16*8)
#define TS_PACKET_NUM		(80)	// Fixed, Maximum 80
#define TS_PAGE_NUM		(36)	// Variable

#define TS_PAGE_SIZE		(TS_PACKET_NUM * TS_PACKET_SIZE)

#define PAGE_SIZE 		4096
#define ALIGN(x, a)		(((x) + (a) - 1) & ~((a) - 1))
#define PAGE_ALIGN(addr) 	ALIGN(addr, PAGE_SIZE)

//------------------------------------------------------------------------------
//
//  Return values
//
#define ETS_NOERROR     0
#define ETS_WRITEBUF    1   /* copy_to_user or copy_from_user   */
#define ETS_READBUF     2   /* memory alloc */
#define ETS_FAULT       3   /* copy_to_user or copy_from_user   */
#define ETS_ALLOC       4   /* memory alloc */
#define ETS_RUNNING     5
#define ETS_TYPE        6
#define ETS_TIMEOUT     7

#endif
