#include "app.h"
#include "gpio.h"
#include "spi.h"
#include "app_spi.h"

void sys_spi_init(void)
{
    stc_gpio_cfg_t stcGpioCfg;
    stc_spi_config_t SPIConfig;

    Sysctrl_SetPeripheralGate(SysctrlPeripheralGpio, TRUE);
    Sysctrl_SetPeripheralGate(SysctrlPeripheralSpi0, TRUE); //SPI0

    DDL_ZERO_STRUCT(stcGpioCfg);

    stcGpioCfg.enDir = GpioDirOut;
    Gpio_Init(M_SPI0_MOSI_PORT, M_SPI0_MOSI_PIN, &stcGpioCfg);      //MOSI
    Gpio_SetAfMode(M_SPI0_MOSI_PORT, M_SPI0_MOSI_PIN, GpioAf6);
    Gpio_Init(M_SPI0_CS_PORT, M_SPI0_CS_PIN, &stcGpioCfg);          //CS
    Gpio_SetAfMode(M_SPI0_CS_PORT, M_SPI0_CS_PIN, GpioAf1);
    Gpio_Init(M_SPI0_CLK_PORT, M_SPI0_CLK_PIN, &stcGpioCfg);        //SCK
    Gpio_SetAfMode(M_SPI0_CLK_PORT, M_SPI0_CLK_PIN, GpioAf1);

    Gpio_Init(M_SPI0_DCS_PORT, M_SPI0_DCS_PIN, &stcGpioCfg);        //DCS
    Gpio_SetAfMode(M_SPI0_DCS_PORT, M_SPI0_DCS_PIN, GpioAf0);
    Gpio_Init(M_SPI0_RESET_PORT, M_SPI0_RESET_PIN, &stcGpioCfg);    //RESET
    Gpio_SetAfMode(M_SPI0_RESET_PORT, M_SPI0_RESET_PIN, GpioAf0);

    stcGpioCfg.enDir = GpioDirIn;
    Gpio_Init(M_SPI0_MISO_PORT, M_SPI0_MISO_PIN, &stcGpioCfg);      //MISO
    Gpio_SetAfMode(M_SPI0_MISO_PORT, M_SPI0_MISO_PIN, GpioAf6);

    Spi_SetCS(Spi0, TRUE);

    Gpio_ClrIO(M_SPI0_RESET_PORT, M_SPI0_RESET_PIN);
    delay1ms(100);
    Gpio_SetIO(M_SPI0_RESET_PORT, M_SPI0_RESET_PIN);

    //SPI模块配置
    SPIConfig.bCPHA = Spicphafirst;//模式0
    SPIConfig.bCPOL = Spicpollow;
    SPIConfig.bIrqEn = FALSE;
    SPIConfig.bMasterMode = SpiMaster;
    SPIConfig.u8BaudRate = SpiClkDiv8;
    SPIConfig.pfnSpi0IrqCb = NULL;
	if(SPIConfig.bIrqEn)
	{
		EnableNvic(SPI0_IRQn,IrqLevel3,TRUE);
	}
    Spi_Init(Spi0,&SPIConfig);//模块初始化

    // Spi_SetCS(Spi0, FALSE);//使能片选信号

    // for(i=0;i<10;i++)
    // {
    //   Spi_SendData(Spi0,u8Senddata[i]);//发送数据
    // }
    // i = Spi_ReceiveData(Spi0,1);// 接收推回数据AA
    // for(i=0;i<10;i++)
    // {
    //   u8Recvdata[i]=Spi_ReceiveData(Spi0,1);//主机接收数据
    // }

    // Spi_SetCS(Spi0, TRUE);
}


void plat_spi_start(void)
{
    Spi_SetCS(Spi0, FALSE);
    // DCS
    Gpio_ClrIO(M_SPI0_DCS_PORT, M_SPI0_DCS_PIN);
}

void plat_spi_stop(void)
{
    // DCS
    Spi_SetCS(Spi0, TRUE);
    // DCS
    Gpio_SetIO(M_SPI0_DCS_PORT, M_SPI0_DCS_PIN);
}

void plat_spi_deinit(void)
{
    Spi_DeInit(Spi0);
}

void plat_spi_xmit(const uint8_t *data, uint16_t len)
{
    int i;

    for(i=0; i<len; i++) {
        Spi_SendData(Spi0, data[i]);
    }
}