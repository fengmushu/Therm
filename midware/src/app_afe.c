#include "app_afe.h"
#include "app_timer.h"

uint32_t AfeGetFreq(void)
{
    static uint32_t last_micro_sec = 0;
    uint32_t msec, curr_micro_sec = jffies_to_mic();
    uint32_t Freq, OF_Count = PcntGetData(TRUE);
    /**
     * 读取脉冲计数器溢出次数;
     * 根据填充值, 折算成计数次数;  --- OF_CNT * RELOAD
     * 根据系统时钟, 计算频率;      --- Freq = OF_CNT * RELOAD / xT
     * 根据频率, 计算电压值;        --- 
    */
   if(curr_micro_sec > last_micro_sec) {
       msec = curr_micro_sec - last_micro_sec;
   } else {
       msec = (0xFFFFFFFF - last_micro_sec) + curr_micro_sec;
   }
   
   Freq = OF_Count * AFE_PCNT_RELOAD / msec; // HZ

   DBG_PRINT("AFE Freq: %u OF: %u msec: %u\r\n", Freq, OF_Count, msec);

   return Freq;
}
