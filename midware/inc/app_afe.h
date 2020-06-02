#ifndef __APP_AFE_H__
#define __APP_AFE_H__

#include "app.h"
#include "pcnt.h"

#define AFE_PCNT_RELOAD     100

#ifdef __cplusplus
extern "C"
{
#endif

///< 单脉冲计数器
extern void AppPcntInit(void);

extern uint32_t PcntGetData(boolean_t reset);
extern uint32_t AfeGetFreq(void);

extern const uint8_t bmpSpO2[];
extern const uint8_t bmpRbpm[];
extern const uint8_t bmpBatt[];
extern const uint8_t bmpPulse[];

#ifdef __cplusplus
}
#endif

#endif // __APP_AFE_H__