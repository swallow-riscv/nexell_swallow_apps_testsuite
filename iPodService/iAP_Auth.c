#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>

#include "Cpchip.h"
#include "iAP_Auth.h"

extern void SentCB(struct iAP2Link_st* link, void* context);

int i2c_reset(void)
{
    SysIpodCpchipReset();
	return 0;
}

int i2c_read(unsigned long ulRegAddr, char* pBuf, unsigned int nBufLen)
{
    struct ipodIoParam_t param;
	param.ulRegAddr = ulRegAddr;
	param.pBuf = pBuf;
	param.nBufLen = nBufLen;
	SysIpodCpchipRead(&param);
	return 0;
}

int i2c_write(unsigned long ulRegAddr, char* pBuf, unsigned int nBufLen)
{
    struct ipodIoParam_t param;
	param.ulRegAddr = ulRegAddr;
	param.pBuf = pBuf;
	param.nBufLen = nBufLen;
	SysIpodCpchipWrite(&param);
	return 0;
}

// CP read/write
int CP_Read(unsigned long ulRegAddr, char* pBuf, unsigned int nBufLen)
{
	char buf_tmp;
	i2c_read(0x00, &buf_tmp, 1);
	i2c_read(ulRegAddr, pBuf, nBufLen);
	return 0;
}

int CP_Write(unsigned long ulRegAddr, char* pBuf, unsigned int nBufLen)
{
    char buf_tmp;
	i2c_read(0x00, &buf_tmp, 1);
	i2c_write(ulRegAddr, pBuf, nBufLen);
	return 0;

}

int CP_Reset(void)
{
	i2c_reset();
	return 0;
}

int CP_VersionCheck(void)
{
    //printf("%s\n", __func__);
	Read_CPDeviceID();
	Read_CPDeviceID(); //kupark : Ã³À½ ºÎÆÃ½Ã "ÀåÄ¡ ¸®¼Â"ÀÌ ¿©ÀüÈ÷ ³ª¿Â´Ù ÇÏ¿© ´Ù½Ã Ãß°¡ÇÕ´Ï´Ù.
	Read_CPDeviceID();

	Read_APMajorVersion();
	Read_APMinorVersion();
	return 0;
}

int Read_CPDeviceID(void)
{
	char buf[4];
	unsigned int size = 4;
	CP_Read(0x04, buf, size);
	memcpy(ucCPDeviceID, buf, 4);
	//printf("%s(%x %x %x %x)\n", __func__, ucCPDeviceID[0], ucCPDeviceID[1], ucCPDeviceID[2], ucCPDeviceID[3]);
	return 0;
}

int Read_APMajorVersion(void)
{
    char buf[1];
	unsigned int size = 1;
	CP_Read(0x02, buf, size);
	memcpy(&ucAPMajorVersion, buf, 1);
	//printf("%s(%x)\n", __func__, ucAPMajorVersion);
	return 0;
}

int Read_APMinorVersion(void)
{
	char buf[1];
	unsigned int size = 1;
	CP_Read(0x03, buf, size);
	memcpy(&ucAPMinorVersion, buf, 1);
	//printf("%s(%x)\n", __func__, ucAPMinorVersion);
	return 0;
}

int Read_ErrorCode(void)
{
	sleep(100);//aa
	char buf[1];
	unsigned int size = 1;
	CP_Read(0x05, buf, size);
	memcpy(&ucCPErrorCode, buf, 1);
	printf("ucCPErrorCode-A = 0x%02X\n",buf[0]);
	printf("ucCPErrorCode-B = 0x%02X\n",ucCPErrorCode);
	return 0;
}
							
int Read_AuthentiCS(void)
{
    char buf[1];
	unsigned int size = 1;
	CP_Read(0x10, buf, size);
	memcpy(&ucAuthentiCS, buf, 1);
	//printf("Read_AuthentiCS = 0x%02X\n",ucAuthentiCS);
	return ucAuthentiCS;
}

int Read_CPSignatureDataLength(void)
{
    char buf[2];
	unsigned int size = 2;
	CP_Read(0x11, buf, size);
	memcpy(ucCPSignatureDataLength, buf, 2);
	//printf("Read_ucCPSignatureDataLength[0] = 0x%02X\n",ucCPSignatureDataLength[0]);
	//printf("Read_ucCPSignatureDataLength[1] = 0x%02X\n",ucCPSignatureDataLength[1]);
	return 0;
}

int Read_CPSignatureData(void)
{
    int i;
	char buf[128];
	unsigned int size = 128;
	CP_Read(0x12, buf, size);
	memcpy(ncCPSignatureData, buf, 128);
	//for(i=0; i<128; i++)
	//	printf("ncCPSignatureData[%d] = 0x%02X\n",i,ncCPSignatureData[i]);
	return 0;
}

int Read_CPChallengeDataLength(void)
{
    char buf[2];
	unsigned int size = 2;
	CP_Read(0x20, buf, size);
	memcpy(ucCPChallengeDataLength, buf, 2);
	printf("ucCPChallengeDataLength[0] = 0x%02X\n",ucCPChallengeDataLength[0]);
	printf("ucCPChallengeDataLength[1] = 0x%02X\n",ucCPChallengeDataLength[1]);
	return 0;
}

int Read_CPChallengeData(void)
{
	int i;
    char buf[20];
	unsigned int size = 20;
	CP_Read(0x21, buf, size);
	memcpy(ucCPChallengeData, buf, 20);
	for(i=0; i<20; i++)
		printf("ucCPChallengeData[%d] = 0x%02X\n",i,ucCPChallengeData[i]);
	return 0;
}

int Read_AccCertificateDataLength(void)
{
    char buf[2];
	unsigned int size = 2;

	CP_Read(0x30, buf, size);
	memcpy(ucAccCertificateDataLength, buf, 2);
	//printf("ucAccCertificateDataLength[0] = 0x%02X\n",ucAccCertificateDataLength[0]);
	//printf("ucAccCertificateDataLength[1] = 0x%02X\n",ucAccCertificateDataLength[1]);
	return 0;
}

int Read_AccCertificateData(void)
{
    char buf[128];
	int i, size;

	size = (((ucAccCertificateDataLength[0]<<8) & 0xFF00) |
			(ucAccCertificateDataLength[1] & 0x00FF));

	for (i = 0; i <= (size/128); i++) {
		CP_Read(0x31 + i, buf, 128);
		memcpy(&ucAccCertificateData[i*128], buf, 128);
									    }
	//printf("Read_AccCertificateData size = %d\n",sizeof(ucAccCertificateData));
	return 0;
}

int Write_CPChallengeDataLength(void)
{
    char buf[2] = {0x00, 0x14};
    unsigned short size = 2;
    CP_Write(0x20, buf, size);
    return 0;
}

int Write_CPChallengeData(void)
{
    int i;
	char buf[20];
	unsigned int size = 20;
	for (i = 0; i < 20; i++)
		buf[i] = ucCPChallengeData[i];
	CP_Write(0x21, buf, size);
	return 0;
}

int Write_CPAuthenticationControl(void)
{
    char buf[1] = {0x01};
    unsigned int size = 1;
    CP_Write(0x10, buf, size);
	return 0;
}

int SendDevAuthenticationInfo(void);

int RequestIdentify(void);
int Identify(void);
int GenAck(void);

int GetDevAuthenticationInfo(void)
{
	Read_AccCertificateDataLength();
	Read_AccCertificateData();
	return 0;
}

int RetDevAuthenticationInfo(struct iAP2Link_st* link,
		        uint8_t             session)
{
    int length = ucAccCertificateDataLength[0]<<8 | ucAccCertificateDataLength[1];
	//printf("%s length=%d\n", __func__, length);
	char data[1024];
	int n = 0;

    unCertificateSentSize = 0;
	ucCertificatePageNo = 0;

	data[n++] = ucAPMajorVersion;
	data[n++] = ucAPMinorVersion;
	data[n++] = ucCertificatePageNo;
	data[n++] = ((length + 1)/500);	

	Send(link, session, ucAccCertificateData, length, 0xaa01);
	return 0;
}

int AckDevAuthenticationInfo(void);
int GetDevAuthenticationSign(struct iAP2Link_st* link,
		        uint8_t* data, uint32_t dataLen, uint8_t session)
{
    int i;

	for (i = 0; i < 20; i++) {
		ucCPChallengeData[i] = data[i+10];
	}

    ucCPChallengeDataLength[0] = 0x00;
    ucCPChallengeDataLength[1] = 0x14;

	Write_CPChallengeDataLength();
	Write_CPChallengeData();
	Write_CPAuthenticationControl();
    while((Read_AuthentiCS() & 0xF0) != 0x10) {
		usleep(100000);
	}
	Read_CPSignatureDataLength();
	Read_CPSignatureData();
	RetDevAuthenticationSign(link, session);
	return 0;
}

int AckDevAuthenticationStatus(void);

int RetDevAuthenticationSign(struct iAP2Link_st* link,
				uint8_t				session)
{
	printf("%s\n", __func__);
	Send(link, session, ncCPSignatureData, 128, 0xaa03);
	return 0;
}


int Send(struct iAP2Link_st* link,
		        uint8_t             session, uint8_t* payload, uint32_t payloadLen, uint32_t id)
{
    iAP2Packet_t* pck;
	int ret = 0;
	uint16_t checkSumLen = (payloadLen > 0 ? kIAP2PacketChksumLen : 0);

	pck = iAP2PacketCreateEmptySendPacket(link);

	pck->packetLen = kIAP2PacketHeaderLen + 10 + payloadLen + checkSumLen;;
	pck->pckData->sop1   = kIAP2PacketSYNC;
	pck->pckData->sop2   = kIAP2PacketSOP;
	pck->pckData->len1   = IAP2_HI_BYTE(pck->packetLen);
	pck->pckData->len2   = IAP2_LO_BYTE(pck->packetLen);
	pck->pckData->ctl    = kIAP2PacketControlMaskACK;
	pck->pckData->seq = link->recvAck + 1;
	pck->pckData->ack = link->recvSeq;
	pck->pckData->sess   = session;
	pck->pckData->chk    = 0; 
	pck->pckData->data[0]    = 0x40; 
	pck->pckData->data[1]    = 0x40; 
	pck->pckData->data[2]    = IAP2_HI_BYTE(payloadLen + 10);
	pck->pckData->data[3]    = IAP2_LO_BYTE(payloadLen + 10);
	pck->pckData->data[4]    = IAP2_HI_BYTE(id); 
	pck->pckData->data[5]    = IAP2_LO_BYTE(id); 
	pck->pckData->data[6]    = IAP2_HI_BYTE(payloadLen + 4);
	pck->pckData->data[7]    = IAP2_LO_BYTE(payloadLen + 4);
	pck->pckData->data[8]    = 0; 
	pck->pckData->data[9]    = 0; 

    if (payload != NULL && payloadLen > 0) 
    {    
        memcpy (&pck->pckData->data[10], payload, payloadLen);
    }   

	pck->pckData->chk = iAP2PacketCalcHeaderChecksum(pck);
	pck->dataChecksum = iAP2PacketCalcPayloadChecksum(pck);
	printf("%s : len:%d, hchs:%x, pchs:%x\n", __func__, pck->packetLen, pck->pckData->chk, pck->dataChecksum);
	ret = iAP2LinkQueueSendDataPacket(link, pck, session, link->context, SentCB);
	return 0;
}

static char test_pck[306] = {0xFF, 0x5A, 0x01, 0x32, 0x40, 0x75, 0x5D, 0x0A,
						  0xC0, 0x40, 0x40, 0x01, 0x28, 0x1D, 0x01, 0x00,
						  0x15, 0x00, 0x00, 0x43, 0x61, 0x72, 0x70, 0x6C,
						  0x61, 0x79, 0x20, 0x48, 0x65, 0x61, 0x64, 0x55,
						  0x6E, 0x69, 0x74, 0x00, 0x00, 0x0C, 0x00, 0x01,
						  0x46, 0x56, 0x57, 0x38, 0x30, 0x31, 0x30, 0x00,
						  0x00, 0x1D, 0x00, 0x02, 0x43, 0x56, 0x54, 0x45,
						  0x20, 0x45, 0x6C, 0x65, 0x63, 0x74, 0x72, 0x6F,
						  0x6E, 0x69, 0x63, 0x73, 0x2E, 0x43, 0x6F, 0x2E,
						  0x2C, 0x4C, 0x74, 0x64, 0x00, 0x00, 0x13, 0x00,
						  0x03, 0x41, 0x42, 0x43, 0x2D, 0x30, 0x31, 0x32,
						  0x33, 0x34, 0x35, 0x36, 0x37, 0x39, 0x39, 0x00,
						  0x00, 0x0A, 0x00, 0x04, 0x31, 0x2E, 0x30, 0x2E,
						  0x30, 0x00, 0x00, 0x0A, 0x00, 0x05, 0x31, 0x2E,
						  0x30, 0x2E, 0x30, 0x00, 0x00, 0x24, 0x00, 0x06,
						  0x68, 0x00, 0x68, 0x02, 0x68, 0x03, 0xFF, 0xFB,
						  0x4C, 0x00, 0x4C, 0x02, 0x4C, 0x03, 0x4C, 0x05,
						  0x4C, 0x06, 0x4C, 0x07, 0x4C, 0x08, 0x50, 0x00,
						  0x50, 0x02, 0xAE, 0x00, 0xAE, 0x02, 0xAE, 0x03,
						  0x00, 0x12, 0x00, 0x07, 0x68, 0x01, 0xFF, 0xFA,
						  0xFF, 0xFC, 0x4C, 0x01, 0x4C, 0x04, 0x50, 0x01,
						  0xAE, 0x01, 0x00, 0x05, 0x00, 0x08, 0x02, 0x00,
						  0x06, 0x00, 0x09, 0x00, 0x00, 0x00, 0x07, 0x00,
						  0x0C, 0x65, 0x6E, 0x00, 0x00, 0x07, 0x00, 0x0D,
						  0x65, 0x6E, 0x00, 0x00, 0x23, 0x00, 0x10, 0x00,
						  0x06, 0x00, 0x00, 0x00, 0x01, 0x00, 0x10, 0x00,
						  0x01, 0x69, 0x41, 0x50, 0x48, 0x6F, 0x73, 0x74,
						  0x43, 0x6F, 0x6D, 0x70, 0x00, 0x00, 0x04, 0x00,
						  0x02, 0x00, 0x05, 0x00, 0x03, 0x01, 0x00, 0x28,
						  0x00, 0x12, 0x00, 0x06, 0x00, 0x00, 0x00, 0x02,
						  0x00, 0x19, 0x00, 0x01, 0x69, 0x41, 0x50, 0x4D,
						  0x65, 0x64, 0x69, 0x61, 0x50, 0x42, 0x52, 0x65,
						  0x6D, 0x6F, 0x74, 0x65, 0x43, 0x6F, 0x6D, 0x70,
						  0x00, 0x00, 0x05, 0x00, 0x02, 0x01, 0x00, 0x23,
						  0x00, 0x16, 0x00, 0x06, 0x00, 0x00, 0x00, 0x01,
						  0x00, 0x11, 0x00, 0x01, 0x4C, 0x6F, 0x63, 0x61,
						  0x74, 0x69, 0x6F, 0x6E, 0x4E, 0x61, 0x6D, 0x65,
						  0x00, 0x00, 0x04, 0x00, 0x11, 0x00, 0x04, 0x00,
						  0x12, 0x3F};

void send_ident_info(struct iAP2Link_st* link,
		uint8_t             session)
{
	iAP2Packet_t* pck;
	int i = 0, ret = 0;

	pck = iAP2PacketCreateEmptySendPacket(link);

	//pck->pckData = (iAP2PacketData_t*)&test_pck;
	memcpy(pck->pckData, &test_pck, sizeof(test_pck));
	pck->packetLen = sizeof(test_pck);

	pck->pckData->seq = link->recvAck + 1;
	pck->pckData->ack = link->recvSeq;
	pck->pckData->chk = iAP2PacketCalcHeaderChecksum(pck);
	pck->dataChecksum = iAP2PacketCalcPayloadChecksum(pck);

	printf("%s : len:%d, hchs:%x, pchs:%x\n", __func__, pck->packetLen, pck->pckData->chk, pck->dataChecksum);
	ret = iAP2LinkQueueSendDataPacket(link, pck, session, link->context, SentCB);
}

void send_ackpck(struct iAP2Link_st* link)
{
	iAP2Packet_t* pck;

	/* Send ACK */
    pck = iAP2PacketCreateACKPacket (link,
                                     link->sentSeq,
                                     link->recvSeq,
                                     NULL,
                                     0,
                                     0);

	iAP2LinkSendPacket (link, pck, FALSE, "Accessory:SendACK");
}

void req_auth_cert(struct iAP2Link_st* link,
		uint8_t             session)
{
	printf("+++++++++%s %d Request Authentication Certificate ++++++++++\n", __func__, __LINE__);

	GetDevAuthenticationInfo();
	RetDevAuthenticationInfo(link, session);
}

void auth_resp(struct iAP2Link_st* link,
		uint8_t*            data,
		uint32_t            dataLen,
		uint8_t             session)
{
	printf("+++++++++%s %d Authentication Response ++++++++++\n", __func__, __LINE__);

	GetDevAuthenticationSign(link, data, dataLen, session);
}

void start_power_updates(struct iAP2Link_st* link,
		uint8_t             session)
{
	uint8_t data[14];
	int n = 0;
	data[n++] = 0x40;
	data[n++] = 0x40;
	data[n++] = 0x00;
	data[n++] = 0x0e;
	data[n++] = 0xae;
	data[n++] = 0x00;
	data[n++] = 0x00;
	data[n++] = 0x04;

	data[n++] = 0x00;
	data[n++] = 0x00;
	data[n++] = 0x00;
	data[n++] = 0x04;
	data[n++] = 0x00;
	data[n++] = 0x01;
	iAP2LinkQueueSendData(link, data, sizeof(data), session, link->context, SentCB);
}

void power_source_updates(struct iAP2Link_st* link,
		uint8_t             session)
{
	uint8_t data[17];
	int n = 0;
	data[n++] = 0x40;
	data[n++] = 0x40;
	data[n++] = 0x00;
	data[n++] = 0x11;
	data[n++] = 0xae;
	data[n++] = 0x03;
	data[n++] = 0x00;
	data[n++] = 0x06;

	data[n++] = 0x00;
	data[n++] = 0x00;
	data[n++] = 0x03;
	data[n++] = 0xe8;
	data[n++] = 0x00;
	data[n++] = 0x05;
	data[n++] = 0x00;
	data[n++] = 0x01;

	data[n++] = 0x01;
	iAP2LinkQueueSendData(link, data, sizeof(data), session, link->context, SentCB);
}

void start_hid(struct iAP2Link_st* link,
		uint8_t             session)
{
	uint8_t data[76];
	int n = 0;
	data[n++] = 0x40;
	data[n++] = 0x40;
	data[n++] = 0x00;
	data[n++] = 0x4c;
	data[n++] = 0x68;
	data[n++] = 0x00;
	data[n++] = 0x00;
	data[n++] = 0x06;

	data[n++] = 0x00;
	data[n++] = 0x00;
	data[n++] = 0x00;
	data[n++] = 0x02;
	data[n++] = 0x00;
	data[n++] = 0x06;
	data[n++] = 0x00;
	data[n++] = 0x01;

	data[n++] = 0x1f;
	data[n++] = 0xf7;
	data[n++] = 0x00;
	data[n++] = 0x06;
	data[n++] = 0x00;
	data[n++] = 0x02;
	data[n++] = 0x00;
	data[n++] = 0x88;

	data[n++] = 0x00;
	data[n++] = 0x05;
	data[n++] = 0x00;
	data[n++] = 0x03;
	data[n++] = 0x21;
	data[n++] = 0x00;
	data[n++] = 0x2f;
	data[n++] = 0x00;

	data[n++] = 0x04;
	data[n++] = 0x05;
	data[n++] = 0x0c;
	data[n++] = 0x09;
	data[n++] = 0x01;
	data[n++] = 0xa1;
	data[n++] = 0x01;
	data[n++] = 0x15;

	data[n++] = 0x00;
	data[n++] = 0x25;
	data[n++] = 0x01;
	data[n++] = 0x75;
	data[n++] = 0x01;
	data[n++] = 0x95;
	data[n++] = 0x0a;
	data[n++] = 0x09;

	data[n++] = 0xb0;
	data[n++] = 0x09;
	data[n++] = 0xb1;
	data[n++] = 0x09;
	data[n++] = 0xb5;
	data[n++] = 0x09;
	data[n++] = 0xb6;
	data[n++] = 0x09;

	data[n++] = 0xb9;
	data[n++] = 0x09;
	data[n++] = 0xbc;
	data[n++] = 0x09;
	data[n++] = 0xbe;
	data[n++] = 0x09;
	data[n++] = 0xca;
	data[n++] = 0x09;

	data[n++] = 0xcb;
	data[n++] = 0x09;
	data[n++] = 0xcd;
	data[n++] = 0x81;
	data[n++] = 0x02;
	data[n++] = 0x75;
	data[n++] = 0x06;
	data[n++] = 0x95;

	data[n++] = 0x01;
	data[n++] = 0x81;
	data[n++] = 0x03;
	data[n++] = 0xc0;

	iAP2LinkQueueSendData(link, data, sizeof(data), session, link->context, SentCB);
}
