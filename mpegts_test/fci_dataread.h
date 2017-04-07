/*****************************************************************************
 Copyright(c) 2012 FCI Inc. All Rights Reserved

 File name : fci_dataread.h

 Description :
*******************************************************************************/

#ifndef __FCI_DATAREAD_H__
#define __FCI_DATAREAD_H__

#ifdef __cplusplus
extern "C" {
#endif

extern int ptcheck_start(int hDevice, int size, int ch_num);
extern int ptcheck_stop(int hDevice);
extern int data_sig_start(int hDevice);
extern int data_sig_stop(int hDevice);
extern int data_dump_start(int hDevice);
extern int data_dump_stop(int hDevice);

#ifdef __cplusplus
}
#endif

#endif /* __FCI_DATAREAD_H__ */

