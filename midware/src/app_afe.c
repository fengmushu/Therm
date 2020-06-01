#include "app_afe.h"

uint32_t AfeGetFreq(void)
{
    /**
     * 读取脉冲计数器溢出次数;
     * 根据填充值, 折算成计数次数;  --- OF_CNT * RELOAD
     * 根据系统时钟, 计算频率;      --- Freq = OF_CNT * RELOAD / xT
     * 根据频率, 计算电压值;        --- 
    */

   return 0;
}
