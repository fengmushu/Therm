#ifndef __APP_RTC_H__
#define __APP_RTC_H__

#define time_after(a,b)         ((long)((b) - (a)) < 0)
#define time_before(a,b)        time_after(b,a)
#define time_after_eq(a,b)      ((long)((a) - (b)) >= 0)
#define time_before_eq(a,b)     time_after_eq(b,a)

extern uint32_t g_jiffies;

void AppRtcInit(void);
void AppRtcFeed(void);

#endif /* __APP_RTC_H__ */