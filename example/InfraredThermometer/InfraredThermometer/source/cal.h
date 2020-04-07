/* cal header file */

#ifndef __CAL_H__
#define __CAL_H__

typedef struct {
    ///< 数据校验
    uint32_t    uMagic;
    uint16_t    u16Len;
    uint16_t    u16Crc;

    ///< 校准数据
    CalData_t   CalData;
} FactoryData_t;

extern CalData_t *AppCalGet(void);

extern CalData_t *AppCalLoad(void);

extern void AppCalUpdateAndSaveFactory(CalData_t *pCal);

extern void AppCalClean(void);

#endif /* end __CAL_H__ */