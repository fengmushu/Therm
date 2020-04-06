/* i2c eeprom api source file */

#include "i2c.h"
#include "app.h"
#include "app_i2c.h"

/* see 24AA02 datasheet, A0-A2 no input */
#define I2C_SLAVE_ADDR 0xa0
/* 256 bytes */
#define I2C_MAX_SIZE 0x100 
/* page size */
#define I2C_PAGE_SIZE 16
/* 状态等待时间 100 * 10 us */
#define MAX_WAIT_CNT 100 

#define I2C_WRITE    0x0
#define I2C_READ     0x1

/* 发送或接收失败重试次数 */
#define MAX_TRY_CNT 1024

/* I2C state code */
typedef enum en_i2c_stat
{
    STARTED          = 0x8,
    RESTARTED        = 0x10,
    SND_GET_ACK      = 0x18,
    SND_D_GET_ACK    = 0x28,
    RCV_GET_ACK      = 0x40,
    RCV_D_GET_ACK    = 0x50,
    RCV_D_GET_NACK = 0x58
}en_i2c_stat_t;

static M0P_I2C_TypeDef * s_i2c = M0P_I2C0;

static boolean_t __app_i2c_start()
{
    uint8_t state = 0;
    uint8_t wait_cnt = MAX_WAIT_CNT;

    I2C_SetFunc(s_i2c, I2cStart_En);

    while ((!I2C_GetIrq(s_i2c)) && ((wait_cnt--) > 0))delay10us(1);

    state = I2C_GetState(s_i2c);
    if (state != STARTED && state != RESTARTED)
    {
        return FALSE;
    }
    
    I2C_ClearFunc(s_i2c, I2cStart_En);
    return TRUE;
}

static boolean_t __app_i2c_stop()
{
    uint8_t state = 0;

    I2C_SetFunc(s_i2c, I2cStop_En);
    I2C_ClearIrq(s_i2c);

    I2C_ClearFunc(s_i2c, I2cStop_En);
    return TRUE;
}


static boolean_t __app_i2c_send_write_cmd()
{
    uint8_t state = 0;
    uint8_t wait_cnt = MAX_WAIT_CNT;
    
    I2C_WriteByte(s_i2c, I2C_SLAVE_ADDR | I2C_WRITE);
    
    I2C_ClearIrq(s_i2c);   
    while ((!I2C_GetIrq(s_i2c)) && ((wait_cnt--) > 0))delay10us(1);
    
    state = I2C_GetState(s_i2c);
    if (state != SND_GET_ACK)
    {
        return FALSE;
    }
    
    return TRUE;
}

static boolean_t __app_i2c_send_data_byte(uint8_t data)
{
    uint8_t wait_cnt = MAX_WAIT_CNT;
    
    I2C_WriteByte(s_i2c, data);

    I2C_ClearIrq(s_i2c);   
    while ((!I2C_GetIrq(s_i2c)) && ((wait_cnt--) > 0))delay10us(1);
    
    if (I2C_GetState(s_i2c) != SND_D_GET_ACK)
    {
        return FALSE;
    }

    return TRUE;
}

static boolean_t __app_i2c_send_read_cmd()
{
    uint8_t state = 0;
    uint8_t wait_cnt = MAX_WAIT_CNT;
    
    I2C_WriteByte(s_i2c, I2C_SLAVE_ADDR | I2C_READ);
    
    I2C_ClearIrq(s_i2c);  
    while ((!I2C_GetIrq(s_i2c)) && ((wait_cnt--) > 0))delay10us(1);
    
    state = I2C_GetState(s_i2c);
    if (state != RCV_GET_ACK)
    {
        return FALSE;
    }
    
    return TRUE;
}

static boolean_t __app_i2c_recv_data_byte(uint8_t *pdata)
{
    uint8_t state = 0;
    uint8_t wait_cnt = MAX_WAIT_CNT;
    
    I2C_SetFunc(s_i2c, I2cAck_En);
    I2C_ClearIrq(s_i2c);  
    while ((!I2C_GetIrq(s_i2c)) && ((wait_cnt--) > 0))delay10us(1);

    *pdata = I2C_ReadByte(s_i2c);

    state = I2C_GetState(s_i2c);
    if (state != RCV_D_GET_ACK && state != RCV_D_GET_NACK)
    {
        return FALSE;
    }

    return TRUE;
}

void app_i2c_init()
{
    stc_i2c_cfg_t i2c_cfg;
    
    i2c_cfg.bGc = 0;
    i2c_cfg.enMode = I2cMasterMode;
    i2c_cfg.u32Pclk = 4000000; /* 4000KHz PCLK */
    i2c_cfg.u32Baud = 100000; /* 100K baudRate */
    i2c_cfg.u8SlaveAddr = I2C_SLAVE_ADDR;

    /* 打开i2c外设时钟门控 */
    Sysctrl_SetPeripheralGate(SysctrlPeripheralI2c0, TRUE);

    /* use PB08 as SCL, PB09 as SDA, gpio inited int _AppEeI2cPortInit */
    //_AppEeI2cPortInit();
        
    /* init i2c config */
    I2C_Init(s_i2c, &i2c_cfg); 

    /* enable i2c moudle*/
    I2C_SetFunc(s_i2c, I2cModule_En);   

    return;
}


boolean_t app_i2c_write_data(uint8_t addr, uint8_t* pdata, uint16_t len)
{
    uint8_t state = 0;
    uint16_t try_cnt = MAX_TRY_CNT;
    uint16_t write_cnt = 0;

#define WRITE_HANDLE_ERROR \
    do { \
        try_cnt--; \
        __app_i2c_stop(); \
        goto __start; \
    } while(0)

    if ((uint16_t)addr + len > I2C_MAX_SIZE)
    {
        DBG_PRINT("%s:%u Invalid addr=%u len=%u!\r\n", 
            __func__, __LINE__, addr, len);
        return FALSE;
    }

__start:    
    if (!try_cnt) 
    {
        DBG_PRINT("%s write failed!\r\n", __func__);
        return FALSE;
    }

    if (!__app_i2c_start())
    {
        WRITE_HANDLE_ERROR;
    }

    if (!__app_i2c_send_write_cmd())
    {
        WRITE_HANDLE_ERROR;
    }

    /* 发送地址 */
    if (!__app_i2c_send_data_byte(addr))
    {
        WRITE_HANDLE_ERROR;
    }

    /* 发送数据 */
    while (write_cnt < len)
    {
        if (!__app_i2c_send_data_byte(*(pdata + write_cnt)))
        {
            if (!try_cnt)
            {
                __app_i2c_stop();
                return FALSE;
            }
            try_cnt--;
            DBG_PRINT("%s:%u failed!\r\n", __func__, __LINE__);
            continue;
        }
        write_cnt++;
        
        /* eeprom write a page over should restart */
        if (write_cnt % I2C_PAGE_SIZE == 0 && write_cnt < len)
        {
            __app_i2c_stop();
            addr += I2C_PAGE_SIZE;
            goto __start;
        }
    }

    //DBG_PRINT("wirte data %u bytes, retry_cnt %u\r\n", len, MAX_TRY_CNT - try_cnt);

    __app_i2c_stop();

    return TRUE;  
}

boolean_t app_i2c_read_data(uint8_t addr, uint8_t* pdata, uint16_t len)
{
    uint8_t state = 0;
    uint16_t try_cnt = MAX_TRY_CNT;
    uint16_t read_cnt = 0;

#define READ_HANDLE_ERROR \
    do { \
        try_cnt--; \
        __app_i2c_stop(); \
        goto __start; \
    } while(0)

    if ((uint16_t)addr + len > I2C_MAX_SIZE)
    {
        DBG_PRINT("%s:%u Invalid addr=%u len=%u!\r\n", 
            __func__, __LINE__, addr, len);
        return FALSE;
    }

__start:    
    if (!try_cnt) 
    {
        DBG_PRINT("%s read failed!\r\n", __func__);
        return FALSE;
    }

    if (!__app_i2c_start())
    {
        READ_HANDLE_ERROR;
    }

    if (!__app_i2c_send_write_cmd())
    {
        READ_HANDLE_ERROR;
    }

    /* 发送地址 */
    if (!__app_i2c_send_data_byte(addr))
    {
        READ_HANDLE_ERROR;
    }

    /* 切换到读取命令,   需要再发一次STOP&START */
    if (!__app_i2c_stop())
    {
        READ_HANDLE_ERROR;
    }
    
    if (!__app_i2c_start())
    {
        READ_HANDLE_ERROR;
    }
		
    if (!__app_i2c_send_read_cmd())
    {
        READ_HANDLE_ERROR;
    }

    /* 读取数据 */
    while (read_cnt < len)
    {
        if (!__app_i2c_recv_data_byte(pdata + read_cnt))
        {
            if (!try_cnt)
            {
                __app_i2c_stop();
                return FALSE;
            }
            try_cnt--;
            DBG_PRINT("%s:%u failed!\r\n", __func__, __LINE__);
            continue;
        }
        read_cnt++;
    }

    __app_i2c_stop();

    return TRUE;  
}

void app_i2c_dump()
{
    uint16_t i = 0, j = 0;
    uint8_t buf[16];
    printf("----i2c dump, size %u bytes----\r\n", I2C_MAX_SIZE);
    while (i < I2C_MAX_SIZE)
    {
        app_i2c_read_data((uint8_t)i, buf, sizeof(buf));
        
        printf("%02x: ", i);
        j = 0;
        while (j < sizeof(buf))
        {
            printf("%02x ", buf[j++]);
        }
        printf("\r\n");
        i += sizeof(buf);
    }
    printf("----i2c dump end----\r\n");
    return;
}


#define I2C_DEBUG 0
#if I2C_DEBUG
void i2c_debug()
{
    uint16_t i = 0;
    uint8_t tmp = 0;
    uint8_t tmp_d[I2C_MAX_SIZE] = { 0 };

    printf("%s:%u begin i2c test!\r\n", __func__, __LINE__);
    
    app_i2c_init();

    app_i2c_dump();

    i = 0;
    while (i < I2C_MAX_SIZE)
    {
        uint8_t t = (uint8_t)i;
        if (!app_i2c_write_data(i, &t, 1))
        {
            printf("%s:%u write error: i=0x%x\r\n", __func__, __LINE__, i);
            return;
        }
        
        i++;
    }  
    
    printf("%s:%u i2c write test OK!\r\n", __func__, __LINE__);
    
    i = 0;
    while (i < I2C_MAX_SIZE)
    {
        app_i2c_read_data(i, &tmp, 1);
        if (i != tmp)
        {
            printf("%s:%u read error: i=0x%x tmp=0x%x\r\n", 
                __func__, __LINE__, i, tmp);
            return;
        }
        i++;
    }

    printf("%s:%u i2c read one byte test OK!\r\n", __func__, __LINE__);

    app_i2c_read_data(0, tmp_d, I2C_MAX_SIZE);
    i = 0;
    while (i < I2C_MAX_SIZE)
    {
        if (i != tmp_d[i])
        {
            printf("%s:%u read error: i=0x%x tmp=0x%x\r\n",
                __func__, __LINE__, i, tmp);
            return;
        }
        i++;
    }
    
    printf("%s:%u i2c read %d bytes test OK!\r\n", __func__, __LINE__, I2C_MAX_SIZE);

    if (!app_i2c_write_data(0, tmp_d, sizeof(tmp_d)))
    {
        printf("%s:%u write %d bytes error\r\n",
                __func__, __LINE__, sizeof(tmp_d));
        return;
    }
    
    memset(tmp_d, 0, sizeof(tmp_d));
    app_i2c_read_data(0, tmp_d, I2C_MAX_SIZE);
    i = 0;
    while (i < I2C_MAX_SIZE)
    {
        if (i != tmp_d[i])
        {
            printf("%s:%u read error: i=0x%x tmp=0x%x\r\n",
                __func__, __LINE__, i, tmp);
            return;
        }
        i++;
    }
    printf("%s:%u i2c write %d bytes test OK!\r\n", __func__, __LINE__, I2C_MAX_SIZE);

    memset(tmp_d, 'S', sizeof(tmp_d));
    if (!app_i2c_write_data(0, tmp_d, sizeof(tmp_d)))
    {
        printf("%s:%u write %d bytes error\r\n",
                __func__, __LINE__, sizeof(tmp_d));
        return;
    }
    
    memset(tmp_d, 0, sizeof(tmp_d));
    app_i2c_read_data(0, tmp_d, I2C_MAX_SIZE);
    i = 0;
    while (i < I2C_MAX_SIZE)
    {
        if ('S' != tmp_d[i])
        {
            printf("%s:%u read error: i=0x%x tmp=0x%x\r\n",
                __func__, __LINE__, i, tmp);
            return;
        }
        i++;
    }
    printf("%s:%u i2c write %d bytes test OK!\r\n", __func__, __LINE__, I2C_MAX_SIZE);
    return;
}
#endif

