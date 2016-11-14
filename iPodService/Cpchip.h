struct ipodIoParam_t {
	unsigned long ulRegAddr;
	char* pBuf;
	unsigned int nBufLen;
};

bool SysIpodCpchipReset(void);
void SysIpodCpchipRead(struct ipodIoParam_t* param);
void SysIpodCpchipWrite(struct ipodIoParam_t* param);

