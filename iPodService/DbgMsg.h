//------------------------------------------------------------------------------
//
//	Copyright (C) 2015 Nexell Co. All Rights Reserved
//	Nexell Co. Proprietary & Confidential
//
//	NEXELL INFORMS THAT THIS CODE AND INFORMATION IS PROVIDED "AS IS" BASE
//  AND	WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING
//  BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS
//  FOR A PARTICULAR PURPOSE.
//
//	Module		: AVN Monitor Application
//	File		: DbgMsg.h
//	Description	: 
//	Author		: Seong-O Park(ray@nexell.co.kr)
//	Export		:
//	History		:
//
//------------------------------------------------------------------------------

#ifndef __DBGMSG_H__
#define	__DBGMSG_H__

#include <stdio.h>

#ifndef NX_DTAG
#define NX_DTAG "iPodCarplay" //	Set Default TAG name to iAP
#endif

#define DBG_VBS			2	// ANDROID_LOG_VERBOSE
#define DBG_DEBUG		3	// ANDROID_LOG_DEBUG
#define	DBG_INFO		4	// ANDROID_LOG_INFO
#define	DBG_WARN		5	// ANDROID_LOG_WARN
#define	DBG_ERR			6	// ANDROID_LOG_ERROR 
#define DBG_DISABLE		9

#define	DEBUG_LEVEL		DBG_DEBUG
//#define DEBUG_FUNC

#ifdef ANDROID

#include <android/log.h>
#define DBG_PRINT			__android_log_print
#define NxTrace(...)		do {DBG_PRINT(DBG_VBS, NX_DTAG, __VA_ARGS__);}while(0)
#define NxDbgMsg(A, ...)	do {										\
								if( DEBUG_LEVEL<= A ) {		    \
									DBG_PRINT(A, NX_DTAG, __VA_ARGS__);	\
								}										\
							} while(0)
#define NxErrMsg(...)		DBG_PRINT(DBG_ERR, NX_DTAG, __VA_ARGS__);

#else	//	Linux
#define DBG_PRINT			printf
#define NxTrace(...)		do {DBG_PRINT(DBG_VBS, NX_DTAG, __VA_ARGS__);}while(0)
#define NxDbgMsg(A, ...)	do {							\
								if( DEBUG_LEVEL <= A ) {	\
									DBG_PRINT(NX_DTAG);		\
									DBG_PRINT(__VA_ARGS__);	\
								}							\
							} while(0)
#define NxErrMsg(...)		do{													\
							DBG_PRINT( "%s%s(%d) : %s",							\
									NX_DTAG, __FILE__, __LINE__, "Error: " );	\
							DBG_PRINT(__VA_ARGS__);								\
						}while(0)
#endif

//	for short function name in Android
#ifdef ANDROID
#ifdef __func__
#undef __func__
#endif
#define __func__ __FUNCTION__
#endif

#ifdef DEBUG_FUNC
#define	FUNC_IN()		NxDbgMsg(DBG_VBS, "%s  IN\n", __func__);
#define	FUNC_OUT()		NxDbgMsg(DBG_VBS, "%s  OUT\n", __func__);
#else
#define	FUNC_IN()		do{}while(0)
#define	FUNC_OUT()		do{}while(0)
#endif

#endif	//	__DBGMSG_H__
