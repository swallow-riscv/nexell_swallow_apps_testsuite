#include "ux_timer.h"

//===================================================
// Constant
//===================================================
#define MY_TIMER_SIGNAL  SIGRTMIN
#define ONE_MSEC_TO_NSEC 1000000
#define ONE_SEC_TO_NSEC  1000000000

#define ONE_MSEC_TO_SEC 1000

//===================================================
// M A C R O 
//===================================================
#define CHECK_NULL(ptr, rtn) \
{ \
 if ( !(ptr)) { \
  printf("%s is NULL\n", #ptr); \
  return rtn; \
 } \
}

//===================================================
// Timer Manager Class
//===================================================
static void timer_handler(int sig, siginfo_t *si, void *context);
typedef struct {
 	int m_IsUsed;
 	timer_t   m_TmId;
 	TimerHandler m_TmFn;
 	void*   m_UserPtr;
}TimerInfo;


int   m_TimerNum;  
TimerInfo m_TmInfoArr[MAX_TIMER_NUM];
int m_IsCallbacking;

int findNewKey();
int setSignal();
int setTimer(int *key, unsigned long resMSec, TimerHandler tmFn, void *userPtr);
int delTimer(int key);
int isCallbacking();
void callback(int key);
void clear();


int setSignal()
{
 	struct sigaction sa;

 	sa.sa_flags  = SA_SIGINFO;
 	sa.sa_sigaction = timer_handler;

 	sigemptyset(&sa.sa_mask);

 	if(sigaction(MY_TIMER_SIGNAL, &sa, NULL) == -1)
 	{
  		printf("Err:sigaction");
 	}

 	return 1;
}

int setTimer(int *key, unsigned long intv, TimerHandler tmFn, void *userPtr)
{
 	timer_t timerId;
 	struct sigevent sigEvt;
 	/* Set Timer Interval */
	unsigned long mili_intv;
 	unsigned long nano_intv;
 	struct itimerspec its;

 	/* Save Timer Inforamtion */
 	TimerInfo tm;

 	if (m_TimerNum >= MAX_TIMER_NUM)
 	{
  		printf("Err:Too many timer\n");
  		return 0;
 	}

 	*key = findNewKey();

 	/* Create Timer */
 	memset(&sigEvt, 0x00, sizeof(sigEvt));
 	sigEvt.sigev_notify    = SIGEV_SIGNAL;
 	sigEvt.sigev_signo    = MY_TIMER_SIGNAL;
 	sigEvt.sigev_value.sival_int = *key;

 	if(timer_create(CLOCK_REALTIME, &sigEvt, &timerId) != 0)
 	{
  		printf("Err:timer_create\n");
 	}

#if 0
 	nano_intv =(unsigned long)( intv * ONE_MSEC_TO_NSEC );
 	// initial expiration
 	its.it_value.tv_sec  = nano_intv / ONE_SEC_TO_NSEC;
 	its.it_value.tv_nsec  = nano_intv % ONE_SEC_TO_NSEC;

 	// timer interval
 	its.it_interval.tv_sec = its.it_value.tv_sec;
 	its.it_interval.tv_nsec = its.it_value.tv_nsec;
#else
 	its.it_value.tv_sec  = intv / 1000;
 	mili_intv = intv % 1000;
	nano_intv = mili_intv * (unsigned long)1000000;
 	its.it_value.tv_nsec  = nano_intv;

 	its.it_interval.tv_sec  = intv / 1000;
 	mili_intv = intv % 1000;
	nano_intv = mili_intv * (unsigned long)1000000;
 	its.it_interval.tv_nsec  = nano_intv;
	printf("%s() (%d)(%d)  (%d)(%d)\n", __func__, its.it_value.tv_sec, its.it_value.tv_nsec, its.it_interval.tv_sec, its.it_interval.tv_nsec);
#endif

 	if(timer_settime(timerId, 0, &its, NULL) != 0)
 	{
  		printf("Err:timer_settimer\n");
 	}


 	tm.m_TmId   = timerId;
 	tm.m_TmFn  = tmFn;
 	tm.m_UserPtr  = userPtr;

 	m_TmInfoArr[*key] = tm;
 	m_TmInfoArr[*key].m_IsUsed  = 1;

 	m_TimerNum++;

 	return 1;
}

int delTimer(int key)
{
 	TimerInfo tm;

 	if (m_TimerNum <= 0)
 	{
  		printf("Err:Timer not exist\n");
  		return 0;
 	}

 	if (m_TmInfoArr[key].m_IsUsed == 1)
 	{
  		tm = m_TmInfoArr[key];
 	}
 	else
 	{
  		printf("Err:Timer key(%d) not used\n", key);
  		return 0;
 	}

 	if(timer_delete(tm.m_TmId) != 0)
 	{
  		printf("Err:timer_delete");
 	}

 	m_TmInfoArr[key].m_IsUsed = 0;
 	m_TimerNum--;

 	return 1;
}

void callback(int key)
{
 	TimerInfo tm;

 	tm = m_TmInfoArr[key];
 	m_IsCallbacking = 1;

 	(tm.m_TmFn)(key, tm.m_UserPtr);

 	m_IsCallbacking = 0;
}

int isCallbacking()
{
 	return m_IsCallbacking;
}

void clear()
{
	int i;
 	for (i = 0; i < MAX_TIMER_NUM; i++)
 	{
  		if (m_TmInfoArr[i].m_IsUsed == 1)
   			delTimer(i);
 	}

 	m_TimerNum  = 0;
}

int findNewKey()
{
	int i;
 	for (i = 0; i < MAX_TIMER_NUM; i++)
 	{
  		if (m_TmInfoArr[i].m_IsUsed == 0)
   		return i;
 	}
	return i;
}

//===================================================
// USER API
//===================================================
int Ux_InitTimer(void)
{
	m_TimerNum = 0;
	m_IsCallbacking = 0;
	memset(&m_TmInfoArr[0], 0, MAX_TIMER_NUM*sizeof(TimerInfo));

	return 0;
}

int Ux_CreateTimer(void)
{
 	return setSignal();
}

int Ux_SetTimer(int *key, unsigned long resMSec, TimerHandler tmFn, void *userPtr)
{
	int rtn;
 	CHECK_NULL(key, 0);
 	CHECK_NULL(tmFn, 0);

 	rtn = setTimer(key, resMSec, tmFn, userPtr);

 	if (rtn != 1)
 	{
  		printf("set_timer(%p,%lu,%p,%p) fail\n", key, resMSec, tmFn, userPtr);
  		return 0;
 	}

 	return 1;
}

int Ux_KillTimer(int key)
{
 	int rtn;

 	rtn = delTimer(key);
 	if (rtn != 1)
 	{
  		printf("delete_timer(%d) fail\n", key);
  		return 0;
 	}

 	return 1;
}

int Ux_DestroyTimer(void)
{
 	if (isCallbacking() == 1)
 	{
  		printf("Can't destroy timer in callback function.\n");
  		return 0;
 	}
 	clear();

 	return 1;
}

//===================================================
// Timer Handler 
//===================================================
static void timer_handler(int sig, siginfo_t *si, void *context)
{
 	int key = si->si_value.sival_int;
 	callback(key);
}

