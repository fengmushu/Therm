#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>

#include "lcd.h"
#include "app.h"
#include "app_rtc.h"
#include "app_lcd.h"
#include "utils.h"

#define LCD_COM_CNT_MAX                 (8)
#define LCD_SEG_CNT_MAX                 (28) // RAM8~F are not used
#define LCD_CHAR_CRX_SEGS_MAX           (4)

// undefined will lead to ' ' (idx = 0)
static uint8_t ascii_to_7seg[127] = {
        ['0'] = LCD_7SEG_CHAR_0,
        ['1'] = LCD_7SEG_CHAR_1,
        ['2'] = LCD_7SEG_CHAR_2,
        ['3'] = LCD_7SEG_CHAR_3,
        ['4'] = LCD_7SEG_CHAR_4,
        ['5'] = LCD_7SEG_CHAR_5,
        ['6'] = LCD_7SEG_CHAR_6,
        ['7'] = LCD_7SEG_CHAR_7,
        ['8'] = LCD_7SEG_CHAR_8,
        ['9'] = LCD_7SEG_CHAR_9,

        ['A'] = LCD_7SEG_CHAR_A,
        ['a'] = LCD_7SEG_CHAR_A,
        ['B'] = LCD_7SEG_CHAR_b,
        ['b'] = LCD_7SEG_CHAR_b,
        ['C'] = LCD_7SEG_CHAR_C,
        ['c'] = LCD_7SEG_CHAR_c,
        ['D'] = LCD_7SEG_CHAR_d,
        ['d'] = LCD_7SEG_CHAR_d,
        ['E'] = LCD_7SEG_CHAR_E,
        ['e'] = LCD_7SEG_CHAR_E,
        ['F'] = LCD_7SEG_CHAR_F,
        ['f'] = LCD_7SEG_CHAR_F,
        ['G'] = LCD_7SEG_CHAR_G,
        ['g'] = LCD_7SEG_CHAR_G,
        ['H'] = LCD_7SEG_CHAR_H,
        ['h'] = LCD_7SEG_CHAR_h,
        ['I'] = LCD_7SEG_CHAR_I,
        ['i'] = LCD_7SEG_CHAR_i,
        ['J'] = LCD_7SEG_CHAR_J,
        ['j'] = LCD_7SEG_CHAR_J,
        ['L'] = LCD_7SEG_CHAR_L,
        ['l'] = LCD_7SEG_CHAR_L,
        ['N'] = LCD_7SEG_CHAR_n,
        ['n'] = LCD_7SEG_CHAR_n,
        ['O'] = LCD_7SEG_CHAR_O,
        ['o'] = LCD_7SEG_CHAR_o,
        ['P'] = LCD_7SEG_CHAR_P,
        ['p'] = LCD_7SEG_CHAR_P,
        ['Q'] = LCD_7SEG_CHAR_q,
        ['q'] = LCD_7SEG_CHAR_q,
        ['R'] = LCD_7SEG_CHAR_r,
        ['r'] = LCD_7SEG_CHAR_r,
        ['S'] = LCD_7SEG_CHAR_S,
        ['s'] = LCD_7SEG_CHAR_S,
        ['T'] = LCD_7SEG_CHAR_t,
        ['t'] = LCD_7SEG_CHAR_t,
        ['U'] = LCD_7SEG_CHAR_U,
        ['u'] = LCD_7SEG_CHAR_u,
        ['V'] = LCD_7SEG_CHAR_U,
        ['v'] = LCD_7SEG_CHAR_u,
        ['Y'] = LCD_7SEG_CHAR_y,
        ['y'] = LCD_7SEG_CHAR_y,

        [' '] = LCD_7SEG_CHAR_SPACE,
        ['-'] = LCD_7SEG_CHAR_DASH,
        ['_'] = LCD_7SEG_CHAR_ULINE,
};

static __IO uint32_t        *lcd_ram_ioaddr;

struct lcd_hw               *g_lcd_hw;
struct lcd_sym             **g_lcd_syms;
struct lcd_field           **g_lcd_fields;
static struct lcd_7seg_char *g_lcd_chars;
static stc_lcd_cfg_t         g_lcd_syscfg;

static inline int lcd_seg_to_ram_bank(int seg)
{
        return (seg * 8) / 32;
}

static inline int lcd_seg_bit_offset(int seg)
{
        // (seg * 8 ) % 32
        return ((seg * 8) & ((1 << 5) - 1));
}

uint32_t lcd_ram_bank_read(int bank)
{
        return *(__IO uint32_t *)(&lcd_ram_ioaddr[bank]);
}

void lcd_ram_bank_write(int bank, uint32_t val)
{
        *(__IO uint32_t *)(&lcd_ram_ioaddr[bank]) = val;
}

/*
 * batch write: writing starts from lower SEG to higher..
 */
int lcd_seg_same_bank_write(int seg, uint8_t *mask, uint8_t *data, size_t cnt)
{
        int bank;
        int offset;
        uint32_t bm, val;

        if (!data)
                return 0;
        
        bank = lcd_seg_to_ram_bank(seg);
        offset = lcd_seg_bit_offset(seg);

        if (offset + cnt * BITS_PER_BYTE > sizeof(uint32_t) * BITS_PER_BYTE)
                return -EINVAL;

        __disable_irq();

        val = lcd_ram_bank_read(bank);

        for (size_t i = 0; i < cnt; i++) {
                uint8_t m = mask ? mask[i] : 0xff; // one SEG can have 8 COMS max
                bm        = m << (offset + i * BITS_PER_BYTE);
                val      &= ~bm;
                val      |= (data[i] << (offset + i * BITS_PER_BYTE)) & bm;
                lcd_ram_bank_write(bank, val);
        }

        __enable_irq();

        return 0;
}
 
int lcd_seg_cross_bank_write(int seg, uint8_t *mask, uint8_t *data, size_t cnt)
{
        int bank;
        int offset;
        uint32_t val;
        uint32_t bm;

        if (!data)
                return 0;

        __disable_irq();

        for (size_t i = 0; i < cnt; i++) {
                // one SEG can have 8 COMS max;
                uint8_t m = mask ? mask[i] : 0xff;
                bank      = lcd_seg_to_ram_bank(seg);
                val       = lcd_ram_bank_read(bank);
                offset    = lcd_seg_bit_offset(seg);
                bm        = m << offset;
                val      &= ~bm;
                val      |= (data[i] << offset) & bm;
                lcd_ram_bank_write(bank, val);
        }

        __enable_irq();

        return 0;
}

int lcd_seg_write(int seg, uint8_t mask, uint8_t data)
{
        return lcd_seg_cross_bank_write(seg, &mask, &data, 1);
}

int lcd_seg_read(int seg, uint8_t *data)
{
        return 0;
}

struct lcd_sym *lcd_sym_get(uint32_t idx)
{
        if (idx >= NUM_LCD_SYMS)
                return NULL;

        return g_lcd_syms[idx];
}

/*
 * lcd_sym_set() - set the value of symbol only, no writes to REG
 *                 use lcd_sym_list_apply() to do batch update
 */
void __lcd_sym_set(struct lcd_sym *sym, int enabled)
{
        uint8_t vals[2];
        uint8_t nval;

        if (!sym)
                return;

        vals[0] = 0; // disabled
        vals[1] = sym->bm; // enabled

        sym->val = vals[!!enabled];
}

void lcd_sym_list_apply(void)
{
        struct lcd_sym *sym;

        if (!g_lcd_syms)
                return;

        for (size_t i = 0; i < NUM_LCD_SYMS; i++) {
                if ((sym = lcd_sym_get(i)) == NULL)
                        continue;

                lcd_seg_write(sym->seg, sym->bm, sym->val);
        }
}

void __lcd_sym_set_apply(struct lcd_sym *sym, int enabled)
{
        if (!sym)
                return;

        __lcd_sym_set(sym, enabled);
        lcd_seg_write(sym->seg, sym->bm, sym->val);
}

struct lcd_field *lcd_field_get(uint32_t idx)
{
        if (idx >= NUM_LCD_FIELDS)
                return NULL;

        return g_lcd_fields[idx];
}

void lcd_field_dot_set(struct lcd_field *field, int dot)
{
        if (!field)
                return;

        // null checked inside
        __lcd_sym_set_apply(lcd_sym_get(field->sym_dot), dot);
}

void lcd_field_clear(struct lcd_field *field)
{
        char buf[LCD_SLOT_CNT_MAX];

        if (!field)
                return;

        memset(buf, ' ', sizeof(buf));

        __lcd_string_show(field, buf, sizeof(buf));
        lcd_field_dot_set(field, 0);
}

void lcd_field_buf_clear(struct lcd_field *field)
{
        if (!field)
                return;

        memset(field->buf, '\0', sizeof(field->buf));
}

void lcd_display_clear(void)
{
        lcd_ram_clear_all();

        for (size_t i = 0; i < NUM_LCD_SYMS; i++) {
                lcd_sym_set(i, 0);
        }

        for (size_t i = 0; i < NUM_LCD_FIELDS; i++) {
                lcd_field_buf_clear(lcd_field_get(i));
        }
}

int __lcd_string_show(struct lcd_field *field, char *buf, size_t len)
{
        int (*seg_batch_write)(int, uint8_t *, uint8_t *, size_t);
        int ret = 0;

        // if (!field || !buf || !len || !g_lcd_chars || !g_lcd_hw)
        //         return -EINVAL;

        if (len > sizeof(field->buf))
                len = sizeof(field->buf);

        // to avoid garbage char from last lcd display
        memset(field->buf, '\0', sizeof(field->buf));
        memcpy(field->buf, buf, len);

        for (size_t i = 0; i < field->slot_cnt && i < len; i++) {
                struct lcd_slot_addr *slot = &field->slot_addr[i];

                if (field->buf[i] < 0 || field->buf[i] > sizeof(ascii_to_7seg)) {
                        ret = -EINVAL;
                        goto out;
                }

                uint8_t char_idx = ascii_to_7seg[field->buf[i]];

                if (char_idx >= NUM_LCD_7SEG_CHAR) {
                        ret = -EINVAL;
                        goto out;
                }

                seg_batch_write = lcd_seg_same_bank_write;

                if (slot->cross_bank)
                        seg_batch_write = lcd_seg_cross_bank_write;

                seg_batch_write(slot->seg,
                                field->slot_bm,
                                &(g_lcd_chars[char_idx].reg_data[0]),
                                g_lcd_hw->seg_per_slot);
        }

out:
        return ret;
}

int lcd_string_left_aligned_show(struct lcd_field *field, char *buf, size_t len)
{
        size_t slen;

        if (!field || !buf || !len)
                return -EINVAL;

        slen = sizeof(field->buf) < len ? sizeof(field->buf) : len;

        // fast path: return if string is the same with last
        if (!memcmp(field->buf, buf, slen))
                return 0;

        return __lcd_string_show(field, buf, slen);
}

int lcd_string_right_aligned_show(struct lcd_field *field, char *buf, size_t len)
{
        char rbuf[LCD_SLOT_CNT_MAX + 1];

        if (!field || !buf)
                return -EINVAL;

        if (len >= field->slot_cnt || len > sizeof(rbuf))
                goto show;

        memset(rbuf, ' ', sizeof(rbuf));
        memcpy(&rbuf[field->slot_cnt - len], buf, len);
        buf = &rbuf[0];

show:
        return lcd_string_left_aligned_show(field, buf, LCD_SLOT_CNT_MAX);
}

int __lcd_number_show(struct lcd_field *field, uint8_t align, int32_t num,
                      uint8_t digit_cnt, uint8_t dot)
{
        char buf[8];

        if (!field)
                return -EINVAL;

        memset(buf, '\0', sizeof(buf));

        // snprintf() will handle digit_cnt = 0
        snprintf(buf, sizeof(buf), "%0.*d", digit_cnt, num);

        switch (align) {
        case LCD_ALIGN_LEFT:
                lcd_string_left_aligned_show(field, buf, strlen(buf));
                break;

        case LCD_ALIGN_RIGHT:
        default:
                lcd_string_right_aligned_show(field, buf, strlen(buf));
                break;

        }

        lcd_field_dot_set(field, dot);

        return 0;
}

void lcd_ram_clear_all(void)
{
        __disable_irq();

        *(volatile uint32_t *)(&M0P_LCD->RAM0) = 0x00000000;
        *(volatile uint32_t *)(&M0P_LCD->RAM1) = 0x00000000;
        *(volatile uint32_t *)(&M0P_LCD->RAM2) = 0x00000000;
        *(volatile uint32_t *)(&M0P_LCD->RAM3) = 0x00000000;
        *(volatile uint32_t *)(&M0P_LCD->RAM4) = 0x00000000;
        *(volatile uint32_t *)(&M0P_LCD->RAM5) = 0x00000000;
        *(volatile uint32_t *)(&M0P_LCD->RAM6) = 0x00000000;
        *(volatile uint32_t *)(&M0P_LCD->RAM7) = 0x00000000;
        *(volatile uint32_t *)(&M0P_LCD->RAM8) = 0x00000000;
        *(volatile uint32_t *)(&M0P_LCD->RAM9) = 0x00000000;
        *(volatile uint32_t *)(&M0P_LCD->RAMA) = 0x00000000;
        *(volatile uint32_t *)(&M0P_LCD->RAMB) = 0x00000000;
        *(volatile uint32_t *)(&M0P_LCD->RAMC) = 0x00000000;
        *(volatile uint32_t *)(&M0P_LCD->RAMD) = 0x00000000;
        *(volatile uint32_t *)(&M0P_LCD->RAME) = 0x00000000;
        *(volatile uint32_t *)(&M0P_LCD->RAMF) = 0x00000000;

        __enable_irq();
}

void lcd_ram_display_all(void)
{
        __disable_irq();

        *(volatile uint32_t *)(&M0P_LCD->RAM0) = 0xFFFFFFFF;
        *(volatile uint32_t *)(&M0P_LCD->RAM1) = 0xFFFFFFFF;
        *(volatile uint32_t *)(&M0P_LCD->RAM2) = 0xFFFFFFFF;
        *(volatile uint32_t *)(&M0P_LCD->RAM3) = 0xFFFFFFFF;
        *(volatile uint32_t *)(&M0P_LCD->RAM4) = 0xFFFFFFFF;
        *(volatile uint32_t *)(&M0P_LCD->RAM5) = 0xFFFFFFFF;
        *(volatile uint32_t *)(&M0P_LCD->RAM6) = 0xFFFFFFFF;
        *(volatile uint32_t *)(&M0P_LCD->RAM7) = 0xFFFFFFFF;
        *(volatile uint32_t *)(&M0P_LCD->RAM8) = 0x000000FF;
        *(volatile uint32_t *)(&M0P_LCD->RAM9) = 0x000000FF;
        *(volatile uint32_t *)(&M0P_LCD->RAMA) = 0x000000FF;
        *(volatile uint32_t *)(&M0P_LCD->RAMB) = 0x000000FF;
        *(volatile uint32_t *)(&M0P_LCD->RAMC) = 0x000000FF;
        *(volatile uint32_t *)(&M0P_LCD->RAMD) = 0x000000FF;
        *(volatile uint32_t *)(&M0P_LCD->RAME) = 0x000000FF;
        *(volatile uint32_t *)(&M0P_LCD->RAMF) = 0x000000FF;

        __enable_irq();
}

void lcd_sw_blink(size_t cnt, size_t interval_ms)
{
        for (size_t i = 0; i < cnt; i++) {
                lcd_ram_display_all();
                delay1ms(interval_ms);
                lcd_ram_clear_all();
                delay1ms(interval_ms);
        }

        // sync SW states
        lcd_display_clear();
}

void lcd_sys_init(void)
{
        stc_lcd_segcom_t lcd_segcom_cfg;

        Sysctrl_SetPeripheralGate(SysctrlPeripheralLcd,TRUE);

        Sysctrl_SetRCLTrim(SysctrlRclFreq32768);
        Sysctrl_ClkSourceEnable(SysctrlClkRCL,TRUE);

        lcd_segcom_cfg.u32Seg0_31 = 0xFFE00000; // 配置LCD_POEN0寄存器 开启SEG0~SEG9, SEG10-19
        lcd_segcom_cfg.stc_seg32_51_com0_8_t.seg32_51_com0_8     = 0xffffffff; // 初始化LCD_POEN1寄存器 全部关闭输出端口
        lcd_segcom_cfg.stc_seg32_51_com0_8_t.segcom_bit.Com0_3   = 0; // 使能COM0~COM3
        lcd_segcom_cfg.stc_seg32_51_com0_8_t.segcom_bit.Mux      = 0; // Mux=0,Seg32_35=0,BSEL=1表示:选择外部电容工作模式，内部电阻断路
        lcd_segcom_cfg.stc_seg32_51_com0_8_t.segcom_bit.Seg32_35 = 0;
        Lcd_SetSegCom(&lcd_segcom_cfg); // LCD COMSEG端口配置

        g_lcd_syscfg.LcdBiasSrc = LcdExtCap;   // 电容分压模式，需要外部电路配合
        g_lcd_syscfg.LcdDuty    = LcdDuty4;    // 1/4 duty
        g_lcd_syscfg.LcdBias    = LcdBias3;    // 1/3 BIAS
        g_lcd_syscfg.LcdCpClk   = LcdClk2k;    // 电压泵时钟频率选择2kHz
        g_lcd_syscfg.LcdScanClk = LcdClk128hz; // LCD扫描频率选择128Hz
        g_lcd_syscfg.LcdMode    = LcdMode0;    // 选择模式0
        g_lcd_syscfg.LcdClkSrc  = LcdRCL;      // LCD时钟选择RCL
        g_lcd_syscfg.LcdEn      = LcdEnable;   // 使能LCD模块
        Lcd_Init(&g_lcd_syscfg);
}

void lcd_sys_enable(int enable)
{
        g_lcd_syscfg.LcdEn = enable ? LcdEnable : LcdDisable;
        Lcd_Init(&g_lcd_syscfg);
}

void lcd_init(struct lcd_hw *hw)
{
        if (!hw)
                return;

        lcd_ram_ioaddr = &M0P_LCD->RAM0;

        lcd_sys_init();
        lcd_ram_clear_all();
        lcd_sys_enable(1);

        g_lcd_hw     = hw;
        g_lcd_chars  = hw->chars;
        g_lcd_syms   = hw->syms;
        g_lcd_fields = hw->fields;

        lcd_sym_list_apply();
}

#if 0
void lcd_test(void)
{
        static int inited = 0;
        char buf[8] = { 0 };
        static int c = 0;
        int i = c % NUM_LCD_SYMS;

        if (!inited) {
                lcd_init(g_lcd_def_hw);
                inited = 1;
        }

        // snprintf(buf, sizeof(buf), "%x", c);

        lcd_string_show(LCD_BIGNUM, LCD_ALIGN_RIGHT, "Scan", 4);
        lcd_string_show(LCD_LOGNUM, LCD_ALIGN_RIGHT, "ConF", 4);
        lcd_string_show(LCD_IDXNUM, LCD_ALIGN_RIGHT, "YY", 2);

        // lcd_number_show(LCD_BIGNUM, LCD_ALIGN_RIGHT, c,      2, 1);
        // lcd_number_show(LCD_LOGNUM, LCD_ALIGN_RIGHT, c * -1, 3, 1);
        // lcd_number_show(LCD_IDXNUM, LCD_ALIGN_RIGHT, c,      0, 1);

        lcd_sym_set_invert(i);

        c++;
        delay1ms(200);
}
#endif
