#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/signal.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <poll.h>

// socket programming header.
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <unistd.h>
//////////////////////

#include "uevent.h"

#include "DbgMsg.h"
#include "gpio.h"
#include "iAP_Auth.h"

#include <iAP2Packet.h>
#include <iAP2LinkRunLoop.h>
#include <iAP2Log.h>

#include "NX_Semaphore.h"

#define ERROR_PROCESS
//////////////////////////////////////////////////////////////////////////////////
#define DEF_PORT			13512
#define BACKLOG				10     /* how many pending connections queue will hold */

#define IPOD_UEVENT_STRING		"change@/devices/virtual/iuihid/iuihid"
#define IPOD_STATE_FILE			"/dev/iuihid"
#define IPOD_USBCONFIG_STRING		"change@/devices/virtual/android_usb/android0"
#define IPOD_USBCONFIG_STATUS		"/sys/devices/virtual/android_usb/android0/state"
#define IUIHID_DEV_FILE			"/dev/iuihid"
#define IAP_DEV_FILE			"/dev/android_iap"
#define REGNET_SYSFS			"/sys/class/regnet/regnet/regnet_dev"

#define USB_HOST_DEVICE_CHANGE		PAD_GPIO_B + 20          // avn
#define USB_OTG_POWER			PAD_GPIO_ALV + 5

typedef enum
{
	IPC_TYPE_UEVENT= 0,
	IPC_TYPE_MAX
} IPC_TYPE;

typedef enum
{
	IAP2_STATE_NONE		= 0,
	IAP2_STATE_READY	= 1,
	IAP2_STATE_LINK		= 2,
	IAP2_STATE_CONNECTED	= 3,
	IAP2_STATE_CLOSE	= 4,
	IAP2_STATE_ERROR	= 5,
	IAP2_STATE_MAX
} IAP2_STATE;

typedef enum
{
	EVT_IUIHUI_CONNECT	= 0,
	EVT_IUIHUI_DISCONNECT	= 1,
	EVT_USB_CONNECT		= 2,
	EVT_USB_DISCONNECT	= 3,
	EVT_MSG_MAX
} EVENT_MSG_TYPE;

typedef struct PhaseStateFunc {
        int phase;
        int (*func)(int msg);
} PhaseStateFunc ;


int iap2Func_PhaseEnter(int msg);
int iap2Func_PhaseReady(int msg);
int iap2Func_PhaseLink(int msg);
int iap2Func_PhaseConnected(int msg);
int iap2Func_PhaseClose(int msg);
int iap2Func_PhaseError(int msg);

PhaseStateFunc gPhaseStateFunc[IAP2_STATE_MAX] =
{
        {IAP2_STATE_NONE,		iap2Func_PhaseEnter},
        {IAP2_STATE_READY,		iap2Func_PhaseReady},
        {IAP2_STATE_LINK,		iap2Func_PhaseLink},
        {IAP2_STATE_CONNECTED,          iap2Func_PhaseConnected},
        {IAP2_STATE_CLOSE,		iap2Func_PhaseClose},
        {IAP2_STATE_ERROR,		iap2Func_PhaseError},
};

typedef struct IPC_SOCKET
{
	uint8_t type;
	uint8_t length;
	uint8_t msg;
	uint8_t dev;
	uint8_t log[16];
}IPC_SOCKET;

int time_ipod_service_start =0;
int time_ipod_service_stop =0;

iAP2LinkRunLoop_t* linkRunLoop = 0;
struct iui_packet {
	char *buf;
	unsigned int size;
};

typedef struct
{
	pthread_t m_monitor;
	pthread_t m_manager;
	pthread_t m_init_cp;

	NX_SEMAPHORE *pSem_CPInit;
	int socketId;
} MANAGER_IAP;

typedef struct
{
	pthread_t m_thread_chkstat;
	pthread_t m_thread_recv;

//	iAP2LinkRunLoop_t* linkRunLoop ;
	iAP2PacketSYNData_t synParam;

	int connect_done;
	int fd_iuihid;
	int fd_iap;
	int g_exit ;
	int g_detectSuccess ;
	int g_iapWriteError ;

	int m_curState ;
	int m_errState ;
#ifdef ERROR_PROCESS
	int timer;
#endif
} IAP2_CTRL;

typedef struct IAP_HANDLER
{
	int iap_ver;

	// handler - manager thread
	MANAGER_IAP handle;
	IAP2_CTRL iap2;
	//

} IAP_HANDLER;
IAP_HANDLER iapCtrl;

void recv_packet_handle(struct iAP2Link_st* link, uint8_t* data, uint32_t dataLen, uint8_t session);
void initialize_iap2_handler(void);
void iap2SyncConfigurationData(iAP2PacketSYNData_t *synParam);
void evtProcess_disconnect(void);

////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////////
int GetTickCountMs(void)
{
        struct timeval tv;
        struct timezone zv;
	int time, time_sec, time_ms;
        gettimeofday(&tv, &zv);

	time_sec = (tv.tv_sec%100);
	time_ms = (tv.tv_usec/1000);
	time = (time_sec*1000) + time_ms;

	return time;
}

int32_t GetPorcessId( const char *pAppName )
{
        char cmd[512], result[512];
        sprintf( cmd, "ps | grep %s", pAppName );

        FILE *fp = NULL;
        int32_t pid = -1;

        // USER/PID/PPID/VSIZE/RSS/WCHAN/PC/??/NAME
        fp = popen( cmd, "r" );
        fseek( fp, 0, SEEK_SET );
        fscanf( fp, "%*s %d %*d %*d %*d %*s %*s %*s %*s", &pid );
        pclose( fp );

        if( 0 > pid )
                return pid;

        fp = popen( cmd, "r" );
        fseek( fp, 0, SEEK_SET );
        NxDbgMsg(DBG_VBS, "------------------------------------------------------------------------------------------\n");
        NxDbgMsg(DBG_VBS, "USER     PID   PPID  VSIZE  RSS     WCHAN    PC        NAME\n");
        while( fgets(result, 512, fp) )
        {
                NxDbgMsg(DBG_VBS, "%s", result);
        }
        NxDbgMsg(DBG_VBS, "------------------------------------------------------------------------------------------\n");
        pclose( fp );

        return pid;
}


void KillProcess( int32_t pid )
{
        if( pid > 0 )
        {
                char cmd[512];
                sprintf( cmd, "kill -9 %d", pid );
                system( cmd );
        }
}

void RunProcess(const char *pAppName, const char *pOption)
{
        int32_t pid;
        char cmd[512];
        if( (pid=GetPorcessId(pAppName)) > 0 )
        {
                KillProcess(pid);
        }
        sprintf( cmd, "%s %s", pAppName, pOption );
        system( cmd );
}

/////////////////////////////////////////////////////////////////////////

int regnet(void)
{
	int fd, len;
	char buf[40];

	fd = open(REGNET_SYSFS, O_WRONLY);
	if (fd < 0) {
		perror("register ncm net");
		return fd;
	}

	len = snprintf(buf, sizeof(buf), "%d", 1);
	write(fd, buf, len);
	close(fd);

	return 0;
}

void recv_thread_handle(iAP2LinkRunLoop_t* linkRunLoop, char* buffer, size_t bufSize)
{
	char * buf = 0;
	iAP2Packet_t* pck;
	BOOL IsDetect;

	buf = buffer;

	pck = iAP2PacketCreateEmptyRecvPacket(linkRunLoop->link);
	iAP2PacketParseBuffer(buf, bufSize, pck, 1024, &IsDetect, NULL, NULL);
	while(!iAP2PacketIsComplete(pck));

	iAP2LinkRunLoopHandleReadyPacket(linkRunLoop, pck);
}


static void SendPacket (struct iAP2Link_st* link, iAP2Packet_t*       packet)
{
	int i = 0;
	int ret = 0;
	if(0 > iapCtrl.iap2.fd_iap) {
                NxDbgMsg(DBG_DEBUG, "[IAP_HANDLE] %s() line[%d] ERROR fd_iap", __func__, __LINE__);
	}

	packet->pckData->chk = iAP2PacketCalcHeaderChecksum(packet);
	packet->dataChecksum = iAP2PacketCalcPayloadChecksum(packet);

	ret = write(iapCtrl.iap2.fd_iap, packet->pckData, packet->packetLen);
	if(ret < 0) iapCtrl.iap2.g_iapWriteError = 1;
}

void SentCB(struct iAP2Link_st* link, void* context)
{
	; //	NxDbgMsg(DBG_INFO, "%s[%d] SentCB\n", __func__, __LINE__);
}

static BOOL Receive (struct iAP2Link_st* link, uint8_t* data, uint32_t dataLen, uint8_t session)
{
	//printf("+++++++++%s %d++++++++++\n", __func__, __LINE__);
	recv_packet_handle(link, data, dataLen, session);
	return TRUE;
}

static void SendDetect (struct iAP2Link_st* link, BOOL bBad)
{
	int ret;

	if(0 > iapCtrl.iap2.fd_iap)
	{
		NxDbgMsg(DBG_DEBUG, "[IAP_HANDLE]%s() line[%d] ERROR fd_iap\n", __func__, __LINE__);
	}

	if (!bBad)
	{
		ret = write(iapCtrl.iap2.fd_iap, &kIap2PacketDetectData, kIap2PacketDetectDataLen);
	}
	else
	{
		ret = write(iapCtrl.iap2.fd_iap, &kIap2PacketDetectBadData, kIap2PacketDetectBadDataLen);
	}

	NxDbgMsg(DBG_DEBUG, "[IAP_HANDLE]%s() ret:%d \n", __func__, ret);
	if(ret > 0) iapCtrl.iap2.g_detectSuccess = 1;
	else iapCtrl.iap2.g_detectSuccess = -1;
}

static void ConnectedCB (struct iAP2Link_st* link, BOOL bConnected)
{
	NxDbgMsg(DBG_INFO, "%s[%d] connect state: %s\n", __func__, __LINE__, bConnected ? "connected" : "disconnected");
}


static void signal_handler(int sig)
{
	char cmdString[1024] = { 0, };

	printf("Aborted by signal %s (%d)...\n", (char*)strsignal(sig), sig);

	switch (sig)
	{
	case SIGINT:
		printf("SIGINT..\n");   break;
	case SIGTERM:
		printf("SIGTERM..\n");  break;
	case SIGABRT:
		printf("SIGABRT..\n");  break;
	default:
		break;
	}

	iapCtrl.iap2.g_exit = 1;

	iAP2LinkRunLoopDetached(linkRunLoop);
	iAP2LinkRunLoopDelete(linkRunLoop);
#if 0
	sprintf(cmdString, "ndc network destroy 100");
	system(cmdString);
#endif
	NxDbgMsg(DBG_INFO, "===== DEVICE => HOST\n");
	gpio_set_value(USB_HOST_DEVICE_CHANGE, 1);
	gpio_unexport(USB_HOST_DEVICE_CHANGE);

	close(iapCtrl.iap2.fd_iuihid);
	close(iapCtrl.iap2.fd_iap);
}

static void register_signal(void)
{
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);
	signal(SIGABRT, signal_handler);
}

int32_t GetIPodLinkStatus()
{
        usleep(1000);
        return ( 0 == access(IPOD_STATE_FILE, F_OK) );
}

int32_t GetAndroidUsbStatus()
{
        int32_t fd = -1;
        char value[16];

	memset(value, 0, 16);
        if (0 > (fd = open(IPOD_USBCONFIG_STATUS, O_RDONLY))) {
                return -1;
        }

        if (0 >= read(fd, (char*)&value[0], 10)) {
                close(fd);
                return -1;
        }

        if (fd > 0)
                close(fd);

	if(strncmp("CONFIGURED", value, 10) == 0) return 1;
	else return 0;
}


int GetIPodDeviceMode(void)
{
#if 0
        char val[128] = { 0, };
        int nval = 0;
        if (property_get("persist.nx.ipod.device.mode", val, NULL) != 0)
	{
		return 99;
        }
        else
        {
                nval = atoi(val);
        }

        return nval;
#endif
	return 2;
}

void recv_packet_handle(struct iAP2Link_st* link, uint8_t* data, uint32_t dataLen, uint8_t session)
{
	int i = 0, ret = 0;
	uint16_t rcvPktRsp = (data[4]<<8 | data[5]);
	uint8_t flag_connect = data[7];

	switch(rcvPktRsp)
	{
		case 0x1d00:
			NxDbgMsg(DBG_INFO, "%s[%d] Start Identification\n", __func__, __LINE__);
			send_ident_info(link, session);
			break;
		case 0x1d02:
			NxDbgMsg(DBG_INFO, "%s[%d] Identification Accepted\n", __func__, __LINE__);
			send_ackpck(link);
			break;
		case 0xaa00:
			NxDbgMsg(DBG_INFO, "%s[%d] Request Authentication Certificate\n", __func__, __LINE__);
			req_auth_cert(link, session);
			break;
		case 0xaa02:
			NxDbgMsg(DBG_INFO, "%s[%d] Request Authentication Challenge\n", __func__, __LINE__);
			auth_resp(link, data, dataLen, session);
			break;
		case 0xaa04:
			NxDbgMsg(DBG_INFO, "%s[%d] Authentication Failed\n", __func__, __LINE__);
			send_ackpck(link);
			break;
		case 0xaa05:
			NxDbgMsg(DBG_INFO, "%s[%d] Authentication Succeeded\n", __func__, __LINE__);
			send_ackpck(link);
			start_power_updates(link, session);
			power_source_updates(link, session);
			start_hid(link, session);
			regnet();

			char cmdString[1024];
			sprintf(cmdString, "ifconfig usb0 up");
			ret = system(cmdString);
			if (ret < 0) {
				printf("usb0 up failed.\n");
				return ;
			}
#if 0
			sprintf(cmdString, "ndc network create 100;ndc network interface add 100 usb0;"
				"ndc network route add 100 usb0 fe80::983b:fcff:fe66:0/64;ndc network default set 100");
			ret = system(cmdString);
			if (ret < 0) {
				printf("usb0 add route failed.\n");
				return ;
			}
#endif
			break;
		case 0xae01:
			NxDbgMsg(DBG_INFO, "%s[%d] Power Update [%x][%x]\n", __func__, __LINE__, data[7], flag_connect);
			send_ackpck(link);

			// jimmy modified. seperate last packet.
			// no use polling --> use Semaphore.
			if(flag_connect == 0x5)
			{
				iapCtrl.iap2.connect_done = 1;
				gPhaseStateFunc[iapCtrl.iap2.m_curState].func(5); 
			}
			break;
		default:
			NxDbgMsg(DBG_WARN, "%s[%d] Unknown Packet Data [0x%x]\n", __func__, __LINE__, rcvPktRsp);
			break;
	}
}

///////////////////////////////////////////////////////////////////////////
// iap2 connection : Action functions
///////////////////////////////////////////////////////////////////////////
int cpChip_init(void)
{
	while(1)
	{
		NX_PendSem(iapCtrl.handle.pSem_CPInit);

		NxDbgMsg(DBG_INFO, "[CPInit] Start");
		CP_Reset();
		CP_VersionCheck();
		NxDbgMsg(DBG_INFO, "[CPInit] End");
	}
}

int do_rollchange(void)
{
        int n = 0, i = 0, ret = 0;
        struct iui_packet packet;

        iapCtrl.iap2.fd_iap = open(IAP_DEV_FILE, O_RDWR|O_NDELAY);
        if (0 > iapCtrl.iap2.fd_iap)
        {
                NxErrMsg("ERROR!!!!!!!!!!!!!!!!!!! path %s %d\n", IAP_DEV_FILE, __LINE__);
                return -1;
        }
        if (gpio_export(USB_HOST_DEVICE_CHANGE) != 0)
        {
                printf("gpio open fail\n");
                ret = -ENODEV;
        }

        NxDbgMsg(DBG_INFO, "===== %s() DEVICE => HOST (GPIO TO HIGH)\n", __func__);
        gpio_dir_out(USB_HOST_DEVICE_CHANGE);
        gpio_set_value(USB_HOST_DEVICE_CHANGE, 1);

        iapCtrl.iap2.fd_iuihid = open( IUIHID_DEV_FILE, O_RDWR|O_NDELAY );
        if(0 > iapCtrl.iap2.fd_iuihid) {
                NxErrMsg("ERROR!!!!!!!!!!!!!!!!!!! path %s %d\n", IUIHID_DEV_FILE, __LINE__);
                return -1;
        }
        ioctl(iapCtrl.iap2.fd_iuihid, 4, (unsigned int)&packet);
        close(iapCtrl.iap2.fd_iuihid);

        return 0;
}

int run_iap2link_start(void)
{
	int ret = 0;

	// cp chip init
	NX_PostSem(iapCtrl.handle.pSem_CPInit);

	// do Apple Roll-Change
	ret = do_rollchange();

	return ret;
}

void iap2LinkClose(void)
{

        gpio_set_value(USB_HOST_DEVICE_CHANGE, 1);
        gpio_unexport(USB_HOST_DEVICE_CHANGE);

        close(iapCtrl.iap2.fd_iuihid);
        close(iapCtrl.iap2.fd_iap);
	NxDbgMsg(DBG_INFO, "%s() handle Close \n", __func__ );

}

void process_recvpacket_thread(iAP2LinkRunLoop_t* linkRunLoop)
{
        int readSize;
        char buf[4096] = { 0 };

        while (!iapCtrl.iap2.g_exit)
        {
                if(iapCtrl.iap2.connect_done==1) break;
		if(iapCtrl.iap2.g_iapWriteError == 1)
		{
			NxDbgMsg(DBG_DEBUG, "%s[%d] ERROR(WR) Line", __func__, __LINE__);
			iapCtrl.iap2.g_exit = 1;
			break;
		}

                readSize = read(iapCtrl.iap2.fd_iap, buf, 1024);
		if(readSize < 0 || readSize > 1024)
		{
			if(iapCtrl.iap2.connect_done == 1) break;

			NxDbgMsg(DBG_DEBUG, "%s() ReadSize ERROR size(%d)", __func__, readSize);
			iapCtrl.iap2.g_exit = 1;
			break;

		}
                recv_thread_handle(linkRunLoop, buf, readSize);

                usleep(1000);
        }

        NxDbgMsg(DBG_DEBUG, "%s() exit. \n", __func__);
	NxDbgMsg(DBG_DEBUG, "********************************************** >>  run time (%d)ms\n",
			GetTickCountMs() - time_ipod_service_start );
}

void process_chkstatus_thread(iAP2LinkRunLoop_t* linkRunLoop)
{
	int i = 0, ret = 0;
	char cmdString[1024];

	sprintf(cmdString, "ping6 -W 2 -c 1 -q fe80::983b:fcff:fe66:e846%%usb0 > /dev/null");
	do {
		ret = system(cmdString);
	} while (ret != 0);

	while (!iapCtrl.iap2.g_exit && !iapCtrl.iap2.g_iapWriteError)
	{
		sprintf(cmdString, "ping6 -W 1 -c 1 -L -q ff02::1%%usb0 > /dev/null");
		ret = system(cmdString);
		if (ret == 256) 
		{
			NxDbgMsg(DBG_INFO,"usb is disconnected!!!!!!!!!\n");
			time_ipod_service_stop = GetTickCountMs();
			// cleanup the link. 
			iAP2LinkRunLoopDetached(linkRunLoop);
			iAP2LinkRunLoopDelete(linkRunLoop);
			break;
		}

		// TODO:: 
		usleep(100);
	}


	iapCtrl.iap2.g_exit = 1;
#if 0
	evtProcess_disconnect();
#else
	iapCtrl.iap2.m_curState = IAP2_STATE_CLOSE;
	gPhaseStateFunc[iapCtrl.iap2.m_curState].func(0); 
#endif

	NxDbgMsg(DBG_DEBUG, "%s() exit\n", __func__);
}

int doConnectIAP2packet(void)
{
        void *context = NULL;
        iAP2PacketSYNData_t synParam;

        // Initial Data
        synParam.version = 1;
        synParam.maxOutstandingPackets = 5;
        synParam.maxRetransmissions = 30;
        synParam.maxCumAck = 3;
        synParam.maxPacketSize = 4096; //4096
        synParam.retransmitTimeout = 2000;
        synParam.cumAckTimeout = 22;
        synParam.numSessionInfo = 3;

        synParam.sessionInfo[0].id = 10;
        synParam.sessionInfo[0].type = 0x0;
        synParam.sessionInfo[0].version = 2;
        synParam.sessionInfo[1].id = 11;
        synParam.sessionInfo[1].type = 0x1;
        synParam.sessionInfo[1].version = 1;
        synParam.sessionInfo[2].id = 12;
        synParam.sessionInfo[2].type = 0x2;
        synParam.sessionInfo[2].version = 1;

        linkRunLoop = iAP2LinkRunLoopCreateAccessory(
                        &synParam, context,
                        SendPacket, Receive, ConnectedCB, SendDetect, TRUE, (u_int8_t)64, NULL);


	#if 0
	iap2SyncConfigurationData(&iapCtrl.iap2.synParam);

        linkRunLoop = iAP2LinkRunLoopCreateAccessory(
			 &iapCtrl.iap2.synParam, context,
                        SendPacket, Receive, ConnectedCB, SendDetect, TRUE, (u_int8_t)64, NULL);
	#endif

        pthread_create(&iapCtrl.iap2.m_thread_recv, NULL, (void *)&process_recvpacket_thread, linkRunLoop);

        iAP2LinkRunLoopAttached(linkRunLoop);

	NxDbgMsg(DBG_DEBUG, "%s() linkRunloop:0x%x", __func__, linkRunLoop);
	return 0;
}

///////////////////////////////////////////////////////////////////////////
// iap2 connection : state functions
///////////////////////////////////////////////////////////////////////////
#ifdef ERROR_PROCESS
int iap2Func_PhaseError(int msg);
void func_error_connect(void)
{
	NxDbgMsg(DBG_INFO, "%s() Time out \n", __func__);
        Ux_KillTimer(iapCtrl.iap2.timer);
        iapCtrl.iap2.timer = -1;

	iap2Func_PhaseError(0);

}

void func_error_link(void)
{
	NxDbgMsg(DBG_INFO, "%s() Time out \n", __func__);
        Ux_KillTimer(iapCtrl.iap2.timer);
        iapCtrl.iap2.timer = -1;

	//iap2Func_PhaseError(0);
	iapCtrl.iap2.m_curState = IAP2_STATE_NONE;
	iapCtrl.iap2.g_exit = 1;

	// handle close
	iap2LinkClose();
	initialize_iap2_handler();
}
#endif

int iap2Func_PhaseEnter(int msg)
{
	int ret;

	NxDbgMsg(DBG_INFO, "%s() State Enter, msg(%d) \n", __func__, msg );
	if(msg != 0) return -1;

	time_ipod_service_start = GetTickCountMs();
	iapCtrl.iap2.g_exit = 0;
	iapCtrl.iap2.connect_done = 0;

	ret = run_iap2link_start();
	if(ret == 0)
	{
		iapCtrl.iap2.m_curState = IAP2_STATE_READY;
	}
	else
	{
		iapCtrl.iap2.m_errState = IAP2_STATE_READY;
		iapCtrl.iap2.m_curState = IAP2_STATE_ERROR;
	}

	return ret;
}

int iap2Func_PhaseReady(int msg)
{
	NxDbgMsg(DBG_INFO, "%s() State Ready, msg(%d) \n", __func__, msg );

	gpio_set_value(USB_HOST_DEVICE_CHANGE, 0);
	iapCtrl.iap2.m_curState = IAP2_STATE_LINK;

#ifdef ERROR_PROCESS
	// TODO:: add time-out wait for android0 connect.
	Ux_SetTimer(&iapCtrl.iap2.timer, 1000, func_error_link, NULL);
#endif


	return 0;
}

int iap2Func_PhaseLink(int msg)
{
	int ret;
	NxDbgMsg(DBG_INFO, "%s() State Link, msg(%d) \n", __func__,msg );

#ifdef ERROR_PROCESS
        Ux_KillTimer(iapCtrl.iap2.timer);
        iapCtrl.iap2.timer = -1;
#endif

	if(msg == EVT_USB_CONNECT) // successed Role-Change iAP2
	{
		iapCtrl.iap2.m_curState = IAP2_STATE_CONNECTED;
		doConnectIAP2packet();
		NxDbgMsg(DBG_INFO, "%s() State Link, msg(%d)  go next\n", __func__,msg );

		#ifdef ERROR_PROCESS
		// TODO :: check whther connected or not.
		Ux_SetTimer(&iapCtrl.iap2.timer, 2000, func_error_connect, NULL);
		#endif
	}

	return 0;
}

int iap2Func_PhaseConnected(int msg)
{
	NxDbgMsg(DBG_INFO, "%s() State Connected, msg(%d), msg", __func__, msg );

	#ifdef ERROR_PROCESS
        Ux_KillTimer(iapCtrl.iap2.timer);
        iapCtrl.iap2.timer = -1;
	#endif

	if(msg == EVT_USB_CONNECT) //ignored.. why occure two times this uevent?  
	{
		NxDbgMsg(DBG_INFO, "%s() Ignored. State Connected, msg(%d), msg", __func__, msg );
		return 0;
	}

	NxDbgMsg(DBG_DEBUG, "********************************************** run time (%d)ms\n",
			GetTickCountMs() - time_ipod_service_start );
#if 0
	// run 'carplay' app.
        RunProcess("mdnsd", "");
        system("am start -a android.intent.action.MAIN -n com.ect.smartcarsync/com.ect.smartcarsync.MainActivity");
#endif
	// check the link-disconnect .
        pthread_create(&iapCtrl.iap2.m_thread_chkstat, NULL,
		       (void*)&process_chkstatus_thread, linkRunLoop) ;

	return 0;
}

int iap2Func_PhaseClose(int msg)
{
	int pid;
	NxDbgMsg(DBG_INFO, "%s() State Close msg(%d)\n", __func__, msg );
	iapCtrl.iap2.m_curState = IAP2_STATE_NONE;
	iapCtrl.iap2.g_exit = 1;
#if 0
	// destory network
        char cmdString[1024] = { 0, };
        sprintf(cmdString, "ndc network destroy 100");
        system(cmdString);

	// notify to carplay. 
	system("am broadcast -a com.nexell.avn.Notify.iPodEjected");
	// kill mdnsd
        if( (pid=GetPorcessId("mdnsd")) > 0 )
        {
                KillProcess(pid);
        }
#endif
	// handle close
	iap2LinkClose();
	initialize_iap2_handler();

	NxDbgMsg(DBG_INFO, "****************************** close time(%d)\n",
		 GetTickCountMs()-time_ipod_service_stop) ;
	return 0;
}

int iap2Func_PhaseError(int msg)
{
	NxDbgMsg(DBG_INFO, "%s() State Error, msg(%d)\n", __func__, msg );

	iapCtrl.iap2.m_curState = IAP2_STATE_CLOSE;
	gPhaseStateFunc[iapCtrl.iap2.m_curState].func(0); 

	return 0;
}


void Manager_iPodService(void)
{
        int sockfd , newfd;  /* listen on sock_fd, new connection on sockId*/
        struct sockaddr_in my_addr;    /* my address information */
        struct sockaddr_in their_addr; /* connector's address information */
        int sin_size;
        int numbytes;
        int ret;
        int optValue = 0x01;
	IPC_SOCKET ipcPkt;

        if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
        {
                NxDbgMsg(DBG_INFO, "%s() error Socket\n", __func__);
		return ;
        }
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&optValue, sizeof(optValue));

        my_addr.sin_family = AF_INET;         /* host byte order */
        my_addr.sin_port = htons(DEF_PORT);     /* short, network byte order */
        my_addr.sin_addr.s_addr = INADDR_ANY; /* auto-fill with my IP */
        bzero(&(my_addr.sin_zero), 8);        /* zero the rest of the struct */
        if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr))  == -1)
        {
                NxDbgMsg(DBG_INFO, "%s() error bind\n", __func__);
		return ;
        }

        if (listen(sockfd, BACKLOG) == -1)
        {
                NxDbgMsg(DBG_INFO, "%s() error listen\n", __func__);
		return ;
        }

        sin_size = sizeof(struct sockaddr_in);
        NxDbgMsg(DBG_INFO, "%s() sinzie:%d, wait for accept\n",__func__, sin_size);

        if ((newfd= accept(sockfd, (struct sockaddr *)&their_addr, &sin_size)) == -1)
        {
		NxDbgMsg(DBG_INFO, "%s(): error accept\n", __func__);
		return ;
        }

        NxDbgMsg(DBG_INFO, "%s() Started socketId(%d)\n", __func__, newfd);
	int err;
	while(1)
	{
		if ((numbytes = read(newfd, &ipcPkt, sizeof(IPC_SOCKET))) <= 0)
		{
			NxDbgMsg(DBG_INFO, "%s() recv error ret(%d)\n", __func__, numbytes);
			continue;
		}
		NxDbgMsg(DBG_INFO, ">>>>> %s state(%d) type:%d, msg:%d, dev:%d\n",
				 __func__, iapCtrl.iap2.m_curState, ipcPkt.type, ipcPkt.msg, ipcPkt.dev);

		err = gPhaseStateFunc[iapCtrl.iap2.m_curState].func(ipcPkt.msg); 
#ifdef ERROR_PROCESS
	//if(err != 0) // TODO::
	//	err = gPhaseStateFunc[iapCtrl.iap2.m_curState].func(ipcPkt.msg);
#endif
		usleep(1000);
	}

}

void evtProcess_disconnect(void)
{
	IPC_SOCKET ipcPkt;

	iapCtrl.iap2.m_curState = IAP2_STATE_CLOSE;

	ipcPkt.type = IPC_TYPE_UEVENT;
	ipcPkt.msg = 0; //
	ipcPkt.dev = GetIPodDeviceMode();

	write(iapCtrl.handle.socketId, &ipcPkt, sizeof(IPC_SOCKET));
}

void evtProcess_usbConfig(void)
{
	IPC_SOCKET ipcPkt;

	ipcPkt.type = IPC_TYPE_UEVENT;
	if(GetAndroidUsbStatus() == 1)
		ipcPkt.msg	= EVT_USB_CONNECT; //
	else
		ipcPkt.msg	= EVT_USB_DISCONNECT; //
	ipcPkt.dev = GetIPodDeviceMode();

	write(iapCtrl.handle.socketId, &ipcPkt, sizeof(IPC_SOCKET));
}

void evtProcess_iPodUevent(void)
{
	IPC_SOCKET ipcPkt;

	ipcPkt.type = IPC_TYPE_UEVENT;
	if(GetIPodLinkStatus() == 0)
		ipcPkt.msg	= EVT_IUIHUI_DISCONNECT;
	else
		ipcPkt.msg	= EVT_IUIHUI_CONNECT;

	ipcPkt.dev = GetIPodDeviceMode();

	write(iapCtrl.handle.socketId, &ipcPkt, sizeof(IPC_SOCKET));
}

void ipod_aud_service(void)
{
	if(GetIPodLinkStatus() == 0)
	{
	NxDbgMsg(DBG_INFO, "******************* **************************************\n");
	NxDbgMsg(DBG_INFO, "******************* finish ipod_audio \n");
	NxDbgMsg(DBG_INFO, "******************* **************************************\n");
#if 0
        system("am broadcast -a com.nexell.avn.Notify.iPodEjected");
        usleep(100*1000);
        KillProcess(GetPorcessId("ipod_audio_server"));
        KillProcess(GetPorcessId("com.nexell.avn.nxipodaudioplayer"));
#endif
	}
	else
	{
	NxDbgMsg(DBG_INFO, "******************* **************************************\n");
	NxDbgMsg(DBG_INFO, "******************* run ipod_audio \n");
	NxDbgMsg(DBG_INFO, "******************* **************************************\n");
#if 0
	RunProcess("ipod_audio_server", " &");
       	system("am start -a android.intent.action.MAIN -n com.nexell.avn.nxipodaudioplayer/com.nexell.avn.nxipodaudioplayer.IPodAudPlayerMainActivity");
#endif
	}

}

void Monitor_iPodLink(void)
{
        int sockfd ;  /* listen on sock_fd, new connection on new_fd */
        struct sockaddr_in my_addr;    /* my address information */
        struct sockaddr_in their_addr; /* connector's address information */
        int sin_size;
        int numbytes;
        int ret;
        int isRun = 1;
	IPC_SOCKET ipcPkt;

        if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
        {
		NxDbgMsg(DBG_INFO, "SocketReceiver: error Socket  \n");
        }

        my_addr.sin_family = AF_INET;         /* host byte order */
        my_addr.sin_port = htons(DEF_PORT);     /* short, network byte order */
        my_addr.sin_addr.s_addr = INADDR_ANY; /* auto-fill with my IP */
        bzero(&(my_addr.sin_zero), 8);        /* zero the rest of the struct */

        while(connect(sockfd, (struct sockaddr *)&my_addr, sizeof(my_addr)) == -1)
        {
                NxDbgMsg(DBG_INFO, "SocketSender: error connect\n");
        }
        NxDbgMsg(DBG_INFO, "%s() socket connected, socketId(%d) \n", __func__, sockfd);
	iapCtrl.handle.socketId = sockfd;


	// uevent check
        struct pollfd fds;
        static char desc[4096];

        uevent_init();
        fds.fd = uevent_get_fd();
        fds.events = POLLIN;

	// start monitor
	while(1)
	{
                int32_t err = poll(&fds, 1, 10); // 10ms checking? can do ?
		if(err <= 0) continue;

                if (fds.revents & POLLIN)
                {
			int32_t len = uevent_next_event(desc, sizeof(desc) - 1);
			int dev_mode = GetIPodDeviceMode();
			if(len < 1)    continue;

#if 1 // added ipod_audio_server start.
			if(dev_mode == 1) // IAP1
			{
				NxDbgMsg(DBG_INFO, ">>>>>>>>>>>>> uevent (%s) ms\n", desc);
				if (strncmp(desc, IPOD_UEVENT_STRING, strlen(IPOD_UEVENT_STRING)) == 0)
					ipod_aud_service();

				continue;
			}

			if(dev_mode != 2)
			{
				continue;
			}
#endif

		if(iapCtrl.iap2.m_curState == IAP2_STATE_CONNECTED)
			NxDbgMsg(DBG_INFO, ">>>>>>>>>>>>> uevent (%s) ms\n", desc);

			if (strncmp(desc, IPOD_UEVENT_STRING, strlen(IPOD_UEVENT_STRING)) == 0)
			{
NxDbgMsg(DBG_INFO, "---------- get uevent (%d) ms\n", GetTickCountMs());
				evtProcess_iPodUevent();
			}
			if (strncmp(desc, IPOD_USBCONFIG_STRING, strlen(IPOD_USBCONFIG_STRING)) == 0)
			{
				evtProcess_usbConfig();
			}
                }
	}
}

void iap2SyncConfigurationData(iAP2PacketSYNData_t *synParam)
{
        // Initial Data
        synParam->version = 1;
        synParam->maxOutstandingPackets = 5;
        synParam->maxRetransmissions = 30;
        synParam->maxCumAck = 3;
        synParam->maxPacketSize = 4096; //4096
        synParam->retransmitTimeout = 2000;
        synParam->cumAckTimeout = 22;
        synParam->numSessionInfo = 3;

        synParam->sessionInfo[0].id = 10;
        synParam->sessionInfo[0].type = 0x0;
        synParam->sessionInfo[0].version = 2;
        synParam->sessionInfo[1].id = 11;
        synParam->sessionInfo[1].type = 0x1;
        synParam->sessionInfo[1].version = 1;
        synParam->sessionInfo[2].id = 12;
        synParam->sessionInfo[2].type = 0x2;
        synParam->sessionInfo[2].version = 1;
}

void initialize_iap2_handler(void)
{
	iapCtrl.iap2.m_thread_chkstat = 0;
	iapCtrl.iap2.m_thread_recv = 0;

//	iapCtrl.iap2.linkRunLoop = NULL;
	linkRunLoop = 0;
	iap2SyncConfigurationData(&iapCtrl.iap2.synParam);

	iapCtrl.iap2.fd_iuihid = -1;
	iapCtrl.iap2.fd_iap = -1;

	iapCtrl.iap2.g_exit = 0;
	iapCtrl.iap2.connect_done = 0;
	iapCtrl.iap2.g_detectSuccess = 0;
	iapCtrl.iap2.g_iapWriteError = 0;

	iapCtrl.iap2.m_curState = IAP2_STATE_NONE;
	iapCtrl.iap2.m_errState = 0;
#ifdef ERROR_PROCESS
	iapCtrl.iap2.timer = -1;
#endif
}

void initialize_ctrl(void)
{
	initialize_iap2_handler();

	// init - iap2SyncPacket init data.
	iap2SyncConfigurationData(&iapCtrl.iap2.synParam);
#ifdef ERROR_PROCESS
	// Timer utils
        Ux_InitTimer();
        Ux_CreateTimer();
#endif
}

int main( int argc, char **argv )
{

	register_signal();
	/*---
	Thread Manager - Manage iPod Service .
	Thread Monitor - Monitor iPodLink Status and Run proper each process.
	Thread CP Init - Need CP(Apple) init When Start iPod Service

	* use semaphore ::
	* use socket :: comminicate with each other thread.
	---*/

	initialize_ctrl();

	// semaphre CPInit : do CPChip Initialize.
        iapCtrl.handle.pSem_CPInit= NX_CreateSem(0, 1);

	// thread manager.
        pthread_create(&iapCtrl.handle.m_manager, NULL, (void *)&Manager_iPodService, NULL);
	// thread monitor .
        pthread_create(&iapCtrl.handle.m_monitor, NULL, (void *)&Monitor_iPodLink, NULL);
	// thread CP Init .
        pthread_create(&iapCtrl.handle.m_init_cp, NULL, (void *)&cpChip_init, NULL);


	// finish
	while(1) sleep(1);

	return 0;
}

