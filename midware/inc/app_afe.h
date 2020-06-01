#ifndef __APP_AFE_H__
#define __APP_AFE_H__

#include "app.h"
#include "pcnt.h"

#define AFE_PCNT_RELOAD     100

///< 单脉冲计数器
extern void AppPcntInit(void);

extern uint32_t PcntGetData(boolean_t reset);

#endif // __APP_AFE_H__