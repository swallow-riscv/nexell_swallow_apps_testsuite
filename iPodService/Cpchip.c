#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>

#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include "Cpchip.h"
#include "gpio.h"
#include "DbgMsg.h"

#if 0
#define CP_DEVID	0x20				// tnn
#define CP_DEVPORT	0				// tnn
#define CP_RESET	PAD_GPIO_E + 9			// tnn
#endif
#define CP_DEVID	0x22				// avn
#define CP_DEVPORT	1				// avn
#define CP_RESET	PAD_GPIO_C + 28			// avn
static unsigned char cp_devid = 0x20;

#define kMFiAuthRetryDelayMics			10000 // 5 ms. -> 10ms

// I2C Linux device handle
int g_i2cFile;

// open the Linux device
void i2cOpen(int port)
{
	char path[20];

	snprintf(path, 19, "/dev/i2c-%d", port);
	g_i2cFile = open(path, O_RDWR);

	if (g_i2cFile < 0) {
		perror("i2cOpen fail");
		exit(1);
	}
}

// close the Linux device
void i2cClose()
{
	close(g_i2cFile);
}

// set the I2C slave address for all subsequent I2C device transfers
void i2cSetAddress(unsigned char address)
{
	if (ioctl(g_i2cFile, I2C_SLAVE, address) < 0) {
		perror("i2cSetAddress");
		exit(1);
	}
}

static bool i2c_write(char regaddr, const void* write_buf, int len)
{
	unsigned char addr = cp_devid;                 /* The I2C slave address */
	int w_len = 0, r_len = 0, i = 0;
	char reg = regaddr;
	char* buf = (char *)malloc(len+1);

	buf[0] = (reg >> 0) & 0xFF;
	memcpy(&buf[1], write_buf, len);
	w_len  = len + 1;

	if (write(g_i2cFile, buf, w_len) != w_len) {
		perror("i2cWrite");
		return false;
	}
	//usleep(1000); //lesc0.

	return true;
}

static bool i2c_read(char regaddr, void* recv_buf, int len)
{
	unsigned char addr = cp_devid;                 /* The I2C slave address */
	int w_len = 0, r_len = 0, i = 0;
	char reg = regaddr;
	char* buf = (char *)malloc(len);
	buf = (char *)recv_buf;

	/* 1 setp : write [ADDR, REG] */
	buf[0] = (reg >> 0) & 0xFF;
	w_len  = 1;
	r_len  = len;

	if (write(g_i2cFile, buf, w_len) != w_len) {
		perror("i2cRead set register");
		return false;
	}
	if (read(g_i2cFile, buf, r_len) != r_len) {
		perror("i2cRead read value");
		return false;
	}

	//usleep(1000); //lesc0

	return true;
}

bool SysIpodCpchipReset(void)
{
	bool ret = 0;
	unsigned int devId = 0;

	if (gpio_export(CP_RESET)!=0) {
		printf("gpio open fail\n");
	}

	i2cOpen(CP_DEVPORT);

	// set slave address of I2C device
	i2cSetAddress(cp_devid >> 1);

	if (i2c_read(0x04, (int8_t*)&devId, 4) && (devId == 0x20000)) {
		printf("devID %x , i2c addr %x\n", devId, cp_devid);
	} else {
		cp_devid = 0x22;
		i2cSetAddress(cp_devid >> 1);
		if (i2c_read(0x04, (int8_t*)&devId, 4) && (devId == 0x20000)) {
			printf("devID %x , i2c addr %x\n", devId, cp_devid);
		}
		else
		{
			printf("devID error\n");
			ret = 1;
		}
			
	}
	i2cClose();

	gpio_unexport(CP_RESET);

	return ret;
}

#if 1
void SysIpodCpchipRead(struct ipodIoParam_t* param)
{
	int value = 0;
	int retry=0;

	i2cOpen(CP_DEVPORT);
	// set slave address of I2C device
	i2cSetAddress(cp_devid >> 1);
	for(retry=0; retry<5; retry++)
	{
		value = i2c_read((char)param->ulRegAddr, param->pBuf, param->nBufLen);

		if(value == 0)
		{
		//	NxErrMsg("%s READ:0x%x (0x%x, 0x%x, len:%d) ", 
		//		__func__, value, (char)param->ulRegAddr, param->pBuf[0], param->nBufLen);
		;
		}
		else 
		{
			break;
		}
		usleep(10*1000);
	}

	i2cClose();
}
#else
void SysIpodCpchipRead(struct ipodIoParam_t* param)
{
	int i = 0;

	//printf("====%s %x %d\n", __func__, param->ulRegAddr, param->nBufLen);

	i2cOpen(CP_DEVPORT);

	// set slave address of I2C device
	i2cSetAddress(cp_devid >> 1);

	for (i = 0; i < 5; i++) 
	{
		if (i2c_read((char)param->ulRegAddr, param->pBuf, param->nBufLen))
			break;
		NxErrMsg("%s Err %d (0x%x, 0x%x, len:%d) ", __func__, i, (char)param->ulRegAddr, param->pBuf[0], param->nBufLen);
		usleep(kMFiAuthRetryDelayMics);
	}

	if (i == 5) {
		NxErrMsg("%s[%d] fail.\n", __func__, __LINE__);
	}

	i2cClose();
}
#endif

void SysIpodCpchipWrite(struct ipodIoParam_t* param)
{
	int i = 0;

	//printf("====%s %x %d\n", __func__, param->ulRegAddr, param->nBufLen);

	i2cOpen(CP_DEVPORT);

	// set slave address of I2C device
	i2cSetAddress(cp_devid >> 1);

	for (i = 0; i < 5; i++) {
		if (i2c_write((char)param->ulRegAddr, param->pBuf, param->nBufLen))
			break;
		NxErrMsg("%s[%d] Err %d\n", __func__, __LINE__, i);
		usleep(kMFiAuthRetryDelayMics);
	}

	if (i == 5) {
		NxErrMsg("%s[%d] fail.\n", __func__, __LINE__);
	}

	i2cClose();
}
