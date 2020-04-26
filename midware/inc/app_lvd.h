#ifndef __APP_LVD_H__
#define __APP_LVD_H__

void AppVolMonitorCfg(int en_lvd_threshold);
void AppVolMonitorInit(void);
void Lvd_IRQHandler(void);

#endif 