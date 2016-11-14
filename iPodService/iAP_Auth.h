#include <iAP2Packet.h>
#include <iAP2LinkRunLoop.h>
#include <iAP2Log.h>

char ucAPMajorVersion;                  //Authentication Protocol Major Version
char ucAPMinorVersion;                  //Authentication Protocol Minor Version
char ucCPIndexHigh[2];                  //Authentication Index Supported High
char ucCPIndexLow[2];                   //Authentication Index Supported Low
char ucCPDeviceID[4];                   //Device ID
char ucCPErrorCode;                     //Error Code
char ucAuthentiCS;
char ucCPSignatureVerificationStatus;   //Signature Verification Status
char ucCPAuthenticationIndex[2];        //Authentication Index
char ucCPAuthenticationControlStatus;   //Authentication Control/Status
char ucCPSignatureDataLength[2];        //Signature Data Length
char ncCPSignatureData[128];            //Signature Data (v2)
char ucCPChallengeDataLength[2];        //Challenge Data Length
char ucCPChallengeData[20];             //Challenge Data (v2)
char ucAccCertificateDataLength[2];     //AccCertificate Data Length (v2)
char ucAccCertificateData[1920];        //AccCertificate Data (v2)
//char ucAccCertificateData[1280];      //AccCertificate Data (v2) CP Chip up version(2.0C)
unsigned int unCertificateSentSize;
char ucCertificatePageNo;

// i2c 
int i2c_reset(void);
int i2c_read(unsigned long ulRegAddr, char* pBuf, unsigned int nBufLen);
int i2c_write(unsigned long ulRegAddr, char* pBuf, unsigned int nBufLen);

// CP read/write
int CP_Read(unsigned long ulRegAddr, char* pBuf, unsigned int nBufLen);
int CP_Write(unsigned long ulRegAddr, char* pBuf, unsigned int nBufLen);


int Read_CPDeviceID(void);
int Read_APMajorVersion(void);
int Read_APMinorVersion(void);
int Read_ErrorCode(void);
int Read_AuthentiCS(void);
int Read_CPSignatureDataLength(void);
int Read_CPSignatureData(void);
int Read_CPChallengeDataLength(void);
int Read_CPChallengeData(void);
int Read_AccCertificateDataLength(void);
int Read_AccCertificateData(void);
int Write_CPChallengeDataLength(void);
int Write_CPChallengeData(void);
int Write_CPAuthenticationControl(void);

int CP_Reset(void);
int CP_VersionCheck(void);
int SendDevAuthenticationInfo(void);

int RequestIdentify(void);
int Identify(void);
int GenAck(void);

int GetDevAuthenticationInfo(void);
int RetDevAuthenticationInfo(struct iAP2Link_st* link,
		                uint8_t             session);
int AckDevAuthenticationInfo(void);
int GetDevAuthenticationSign(struct iAP2Link_st* link,
		        uint8_t* data, uint32_t dataLen, uint8_t session);
int RetDevAuthenticationSign(struct iAP2Link_st* link,
				uint8_t				session);
int AckDevAuthenticationStatus(void);

int Send(struct iAP2Link_st* link, uint8_t session, uint8_t* data, uint32_t datalen, uint32_t id);


// 
void send_ident_info(struct iAP2Link_st* link,
		        uint8_t             session);
void send_ackpck(struct iAP2Link_st* link);
void req_auth_cert(struct iAP2Link_st* link,
		        uint8_t             session);
void auth_resp(struct iAP2Link_st* link,
		        uint8_t*            data,
				uint32_t            dataLen,
				uint8_t             session);

//
void start_power_updates(struct iAP2Link_st* link,
		uint8_t             session);
void power_source_updates(struct iAP2Link_st* link,
		uint8_t             session);
void start_hid(struct iAP2Link_st* link,
		uint8_t             session);
