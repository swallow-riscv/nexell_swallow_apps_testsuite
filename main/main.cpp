#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>

typedef struct TEST_ITEM_INFO{
	uint32_t	binNo;
	const char	appName[128];
	const char	descName[64];
	const char	args[128];
	int32_t		status;		//	0 : OK, 1 : Not tested, -1 : Failed
	uint64_t	testTime;
}TEST_ITEM_INFO;

#define DUMP 0
#define DUMP_LIMIT 100
const char *gstStatusStr[32] = {
	"FAILED",
	"OK",
	"NOT Tested"
};

//	BIN Number
enum {
	BIN_CPU_ID       =   8,

	BIN_SPI          =  10,	//	SFR and Boot
	BIN_UART         =  11,
	BIN_I2C          =  13,
	BIN_ADC          =  22,
	BIN_VPU          =  24,
	BIN_TIMER        =  30,	//	Linux System.
	BIN_INTERRUPT    =  31,	//	Almost IP using interrupt.
	BIN_RTC          =  32,

	BIN_MIPI_DSI     =  40,	//	SFR
	BIN_SDMMC        =  47,

	BIN_MCU_A        =  61,	//	DRAM
	BIN_GPIO         =  64,

	BIN_WATCH_DOG    =  67,
	BIN_L2_CACHE     =  68,	//	All Program.
	BIN_VIP_0        =  70,	//	SFR
	BIN_SCALER       =  71,	//	SFR

	BIN_VPP          = 160,	//	SFR

	// Nexell Specific Tests
	BIN_CPU_SMP      = 201,
	BIN_VIP_1        = 203,	//	SFR , MIPI CSI & VIP 1
	BIN_PWM          = 205,	//	SFR
	BIN_OTG_DEVICE   = 209,	//	USB Host & USB HSIC
	BIN_AES          = 213,	//	SFR
	BIN_CLOCK_GEN    = 214,	//	Almost IP use clock generator.
};

static TEST_ITEM_INFO gTestItems[] =
{
	{ BIN_CPU_SMP    , "/usr/bin/cpuinfo-test",     "CPU Core Test   ", "-n 1",                            1, 0 },
	{ BIN_ADC        , "/usr/bin/adc-test",         "ADC Test        ", "-m ASB",                          1, 0 },
	{ BIN_SDMMC      , "/usr/bin/mmc-test",         "MMC Test        ", "",                                1, 0 },
	{ BIN_PWM        , "/usr/bin/pwm-test",         "PWM0 Test       ", "-p 0 -r 100 -d 50",             	1, 0 },
	{ BIN_PWM        , "/usr/bin/pwm-test",         "PWM1 Test       ", "-p 1 -r 100 -d 50",             	1, 0 },
	{ BIN_PWM        , "/usr/bin/pwm-test",         "PWM2 Test       ", "-p 2 -r 100 -d 50",             	1, 0 },
	{ BIN_VIP_0      , "/usr/bin/cam_test",         "VIP Test        ", "-t 0 -s 1280,720 -r 10 -f 0 -o /run/media/mmcblk1p1/yuv420.yuv",                    1, 0 },
	{ BIN_SCALER     , "/usr/bin/scaler-test-file", "SCALER Test     ", "-i /usr/bin/input.yuv -o output.yuv -s 1280,720 -d 1280,720",                    1, 0 },
	{ BIN_GPIO       , "/usr/bin/gpio-test",        "GPIO A22        ", "-a 22",                    1, 0 },
	{ BIN_GPIO       , "/usr/bin/gpio-test",        "GPIO B0         ", "-a 32",                    1, 0 },
	{ BIN_GPIO       , "/usr/bin/gpio-test",        "GPIO C0         ", "-a 64",                    1, 0 },
	{ BIN_GPIO       , "/usr/bin/gpio-test",        "GPIO D0         ", "-a 96",                    1, 0 },
	{ BIN_GPIO       , "/usr/bin/gpio-test",        "GPIO E0         ", "-a 128",                    1, 0 },
	{ BIN_GPIO       , "/usr/bin/gpio-test",        "GPIO F31        ", "-a 191",                    1, 0 },
	{ BIN_GPIO       , "/usr/bin/gpio-test",        "GPIO G0         ", "-a 192",                    1, 0 },
	{ BIN_GPIO       , "/usr/bin/gpio-test",        "GPIO H0         ", "-a 224",                    1, 0 },
	{ BIN_OTG_DEVICE , "/usr/bin/start_adbd.sh",    "USB DEVICE      ", "",                            1, 0 },
	{ BIN_WATCH_DOG  , "/usr/bin/watchdog-test",    "WatchDog Test   ", "-T 2",                         1, 0 },
#if 0
	{ BIN_SPI        , "/usr/bin/spi-test",         "SPI0 Test        ", "-d /dev/spidev0.0", 1, 0 },
	{ BIN_UART       , "/usr/bin/uart-test",        "UART CH 1 <-> 2 ", "-p /dev/ttyAMA1 -d /dev/ttyAMA2", 1, 0 },
	{ BIN_I2C        , "/usr/bin/i2c-test",         "I2C CH 0 Test   ", "-p 0 -a 64 -m1 -r 0 -v 0x0",      1, 0 },
	{ BIN_AES        , "/usr/bin/sfr-test",         "CRYPTO Test     ", "-a 20440000 -w 7 -m 7 -t",      1, 0 },
#endif
};


static const int gTotalTestItems = sizeof(gTestItems) / sizeof(TEST_ITEM_INFO);

#define	TTY_PATH 	"/dev/ttyS0"
#define	TTY_RED_MSG		"redirected message..."
#define	TTY_RES_MSG		"restored message..."


int stdout_redirect(int fd)
{
	int org, ret;
	if (0 > fd)
		return -EINVAL;

	org = dup(STDOUT_FILENO);
	ret = dup2(fd, STDOUT_FILENO);
	if (0 > ret) {
		printf("fail, stdout dup2 %s\n", strerror(errno));
		return -1;
	}
	return org;
}

void stdout_restore(int od_fd)
{
	dup2(od_fd, STDOUT_FILENO);
}


int TestItem( TEST_ITEM_INFO *info )
{
	int ret = -1;
	char path[1024];

	printf( "========= %s start\n", info->descName );
	memset(path,0,sizeof(path));
	strcat( path, info->appName );
	strcat( path, " " );
	strcat( path, info->args );
	ret = system(path);

	if( 0 != ret )
	{
		info->status = -1;
	}
	else
	{
		info->status = 0;
	}

	printf( "========= %s test result %s\n", info->descName, (0==info->status)?"OK":"NOK" );
	return info->status;
}


int main( int argc, char *argv[] )
{
	int32_t i=0;
	int32_t error=0;

	struct timeval start, end;
	struct timeval itemStart, itemEnd;
#if (DUMP)
	int32_t num = 0;
	int dump_fd = 0;
	int std_fd = 0;
	int32_t cnt = DUMP_LIMIT;

	static char outFileName[512];
#endif

#if (DUMP)
	do{
		sprintf( outFileName, "/run/media/mmcblk0p3/TestResult_%d.txt",	num );
		num++;
	} while(!access( outFileName, F_OK) || !cnt--);
	printf("dump file : %s  \n",outFileName);
	dump_fd = open(outFileName,O_RDWR | O_CREAT | O_DIRECT | O_SYNC);
	std_fd =  stdout_redirect(dump_fd);
#endif

	printf("\n\n================================================\n");
	printf("    Start Test(%d items)\n", gTotalTestItems);
	printf("================================================\n");

	gettimeofday(&start, NULL);


	for( i=0 ; i<gTotalTestItems ; i++ )
	{
		gettimeofday(&itemStart, NULL);
		if( 0 != TestItem(&gTestItems[i]) )
		{
			error = 1;
		}
		gettimeofday(&itemEnd, NULL);
		gTestItems[i].testTime = (uint64_t) ((itemEnd.tv_sec - itemStart.tv_sec)*1000 + (itemEnd.tv_usec - itemStart.tv_usec)/1000);
		fflush(stdout);
		sync();
	}

	gettimeofday(&end, NULL);


	printf("\n    End ASB Test\n");
	printf("================================================\n");

	//	Output

	printf("================================================\n");
	printf("   Test Report (%d)msec \n", (int32_t)((end.tv_sec - start.tv_sec)*1000 + (end.tv_usec - start.tv_usec)/1000));
	printf("  binNo.  Name                      Result\n");
	printf("================================================\n");
	for( i=0 ; i<gTotalTestItems ; i++ )
	{
		printf("  %3d     %s        : %s(%d, %lldmsec)\n",
			gTestItems[i].binNo,
			gTestItems[i].descName,
			gstStatusStr[gTestItems[i].status+1], gTestItems[i].status,
			gTestItems[i].testTime );
	}
	printf("================================================\n\n\n");

	//GetECID( ecid );
	//ParseECID( ecid, &chipInfo );
	//PrintECID();
	if( 0 != error )
	{
		//	Yellow
		printf("\033[43m");
		printf("\n\n\n TEST ERROR!!!");
		printf("\033[0m\r\n");
	}
	else
	{
		//	Green
#if (DUMP)
		remove(outFileName);
#endif
		printf("\033[42m");
		printf("\n\n\n TEST SUCCESS!!!");
		printf("\033[0m\r\n");
	}
#if (DUMP)
	sync();
	close(dump_fd);
	stdout_restore(std_fd);
#endif
	return 0;
}
