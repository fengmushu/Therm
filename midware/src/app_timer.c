#include <limits.h>
#include <stdint.h>

#include "app.h"
#include "app_timer.h"
#include "rtc.h"
#include "timer3.h"
#include "bt.h"
#include "rng.h"
#include "sysctrl.h"

#define TIMER3_PCLK_DIV         (256)

static stc_tim3_mode0_cfg_t timer3_reg_cfg;

static uint8_t              timer3_oneshot = 0;
static void                 *timer3_callback_data;
static void                 (*timer3_irq_callback)(void *priv);

// NOTE: do not change the name, weak function
void Tim3_IRQHandler(uint8_t param)
{
    uint16_t cnt;

    if (TRUE == Tim3_GetIntFlag(Tim3UevIrq)) {
        Tim3_ClearIntFlag(Tim3UevIrq);

        if (timer3_irq_callback != NULL) {
            // __disable_irq();
            timer3_irq_callback(timer3_callback_data);
            // __enable_irq();
        }

        if (timer3_oneshot)
            Tim3_M0_Stop();
    }
}

// due to limited resources, we may calc once and save the result
uint16_t timer3_tick_calc(uint32_t ms)
{
    uint32_t pclk = Sysctrl_GetPClkFreq(); // REVIEW: will PCLK change?!
    uint32_t tick_sec = pclk / TIMER3_PCLK_DIV; // div 1000 here will lose precise
    uint32_t tick;

    tick = tick_sec * ms / 1000;

    if (tick > USHRT_MAX)
        return USHRT_MAX;

    return tick;
}

// auto reload mode:
// count from ARR value to 0xffff
// then reset counter to ARR value, restart couting again
// one tick (count) period in sec = PCLK_HZ / DIV
void timer3_set(uint16_t tick,
                uint8_t oneshot,
                void (irq_callback)(void *),
                void *cb_data)
{        
    uint16_t arr_value;

    // prevent existing timer triggering again
    Tim3_M0_Stop();

    arr_value = (0xFFFF - tick);

    timer3_oneshot = oneshot;
    timer3_callback_data = cb_data;
    timer3_irq_callback = irq_callback;

    Tim3_M0_ARRSet(arr_value);
    Tim3_M0_Cnt16Set(arr_value);
    Tim3_ClearIntFlag(Tim3UevIrq);
    Tim3_Mode0_EnableIrq();
    EnableNvic(TIM3_IRQn, IrqLevel3, TRUE);
}

void timer3_start(void)
{
    Tim3_M0_Stop();
    Tim3_M0_Run();
}

void timer3_stop(void)
{
    Tim3_M0_Stop();
}

void timer3_init(void)
{
    Sysctrl_SetPeripheralGate(SysctrlPeripheralTim3, TRUE);

    timer3_reg_cfg.enWorkMode = Tim3WorkMode0;
    timer3_reg_cfg.enCT       = Tim3Timer;
    timer3_reg_cfg.enPRS      = Tim3PCLKDiv256;
    timer3_reg_cfg.enCntMode  = Tim316bitArrMode;
    timer3_reg_cfg.bEnTog     = FALSE;
    timer3_reg_cfg.bEnGate    = FALSE;
    timer3_reg_cfg.enGateP    = Tim3GatePositive;

    Tim3_Mode0_Init(&timer3_reg_cfg);
}

/*******************************************************************************
 * BT1中断服务函数
 ******************************************************************************/
static volatile uint32_t jffies = 0;

#define SYS_TICK_MAP    (0x10000 / HZ)
#define SYS_TICK_AAR    (0x10000 - SYS_TICK_MAP)

///< sec * HZ
static inline uint32_t sys_tick_fixup(void)
{
    if (Bt_M0_Cnt16Get(TIM0) >= (uint16_t)SYS_TICK_AAR)
    {
        return  (jffies  + (Bt_M0_Cnt16Get(TIM0) - (uint16_t)SYS_TICK_AAR) / SYS_TICK_MAP);
    }
    return jffies;
}

void Tim0Int(void)
{
    if(TRUE == Bt_GetIntFlag(TIM0, BtUevIrq))
    {
        ///< TODO:
        jffies ++;
        Bt_ClearIntFlag(TIM0, BtUevIrq);
    }
}

uint32_t jffies_to_msc(void) //毫秒
{
    return (sys_tick_fixup() * 1000 / HZ);
}

uint32_t jffies_to_mic(void) //微秒
{
    return jffies_to_msc() * 1000;
}

uint32_t jffies_to_sec(void)
{
    return (sys_tick_fixup() / HZ);
}

void basic_timer_init(void)
{
    stc_bt_mode0_config_t     stcBtBaseCfg;

    DDL_ZERO_STRUCT(stcBtBaseCfg);

    Sysctrl_SetPeripheralGate(SysctrlPeripheralBaseTim, TRUE); //Base Timer外设时钟使能

    stcBtBaseCfg.enWorkMode = BtWorkMode0;                  //定时器模式
    stcBtBaseCfg.enCT       = BtTimer;                      //定时器功能，计数时钟为内部PCLK
    stcBtBaseCfg.enPRS      = BtPCLKDiv64;                  //PCLK/64
    stcBtBaseCfg.enCntMode  = Bt16bitArrMode;               //自动重载16位计数器/定时器
    stcBtBaseCfg.bEnTog     = FALSE;
    stcBtBaseCfg.bEnGate    = FALSE;
    stcBtBaseCfg.enGateP    = BtGatePositive;
    
    stcBtBaseCfg.pfnTim0Cb  = Tim0Int;                      //中断函数入口
    
    Bt_Mode0_Init(TIM0, &stcBtBaseCfg);                     //TIM0 的模式0功能初始化

    Bt_M0_ARRSet(TIM0, SYS_TICK_AAR);                      //设置重载值(周期 = 0x10000 - ARR)

    Bt_ClearIntFlag(TIM0,BtUevIrq);                         //清中断标志   
    Bt_Mode0_EnableIrq(TIM0);                               //使能TIM0中断(模式0时只有一个中断)
    EnableNvic(TIM0_IRQn, IrqLevel3, TRUE);                 //TIM0 中断使能
    Bt_M0_Run(TIM0);                                        //TIM0 运行

    /* 随机数发生器 */
    ///< 打开RNG模块时钟门控
    Sysctrl_SetPeripheralGate(SysctrlPeripheralRng, TRUE);

    ///< 上电第一次随机数生成并获取
    Rng_Init();

    ///< 生成一次随机数
    Rng_Generate();

    DBG_PRINT("TIM0 clk: %u HZ. RNG: %u, %u\r\n", SYS_TICK_AAR, Rng_GetData0(), Rng_GetData1());
}
