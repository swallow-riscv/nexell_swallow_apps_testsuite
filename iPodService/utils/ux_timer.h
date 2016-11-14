#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#define MAX_TIMER_NUM 1000

// [ param ]
// key: unique timer key, output parameter.
// intv: interval, millisecond.
// tmFn: the handler want to be called when timer expired.
// userPtr: user specific data (pointer). default is NULL. 
//   You can receive this data when TimerHandler be called. (see TimerHandler param)
typedef void (*TimerHandler)(int key, void *userPtr);

int Ux_InitTimer(void);
int Ux_CreateTimer(void);
int Ux_SetTimer(int *key, unsigned long resMSec, TimerHandler tmFn, void *userPtr);
int Ux_KillTimer(int key);
int Ux_DestroyTimer(void);
