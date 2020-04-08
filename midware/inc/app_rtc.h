#ifndef __APP_RTC_H__
#define __APP_RTC_H__

extern uint32_t g_jiffies;

void AppRtcInit(void);
void AppRtcFeed(void);

#endif /* __APP_RTC_H__ */