#ifndef __APP_LCD_H__
#define __APP_LCD_H__

#include <stdint.h>

#include "app.h"
#include "app_data.h"

#ifdef HW_BOARD_HG03
#define LCD_SLOT_CNT_MAX                (4)
#define LCD_COM_CNT                     (4)
#define LCD_SEG_CNT                     (20)
#define LCD_SEG_PER_SLOT                (2)
#endif

#ifdef HW_BOARD_HG04
#define LCD_SLOT_CNT_MAX                (4)
#define LCD_COM_CNT                     (4)
#define LCD_SEG_CNT                     (14)
#define LCD_SEG_PER_SLOT                (2)
#endif

enum lcd_fields {
        LCD_BIGNUM = 0,
        LCD_LOGNUM,
        LCD_IDXNUM,
        NUM_LCD_FIELDS,
};

enum lcd_field_aligns {
        LCD_ALIGN_LEFT = 0,
        LCD_ALIGN_RIGHT,
        NUM_LCD_ALIGNS,
};

enum {
        LCD_NO_DOT = 0,
        LCD_SHOW_DOT,
};

enum {
        SAME_BANK = 0,
        CROSS_BANK,
};

// symbol definitions share across devices
enum lcd_syms {
        LCD_SYM_BAT = 0,
        LCD_SYM_BAT_LVL1,
        LCD_SYM_BAT_LVL2,
        LCD_SYM_BAT_LVL3,
        LCD_SYM_EMOJI_SMILE,
        LCD_SYM_EMOJI_CRY,
        LCD_SYM_BLUETOOTH,
        LCD_SYM_LOCK,
        LCD_SYM_BUZZER,
        LCD_SYM_TEMP_C,
        LCD_SYM_TEMP_F,
        LCD_SYM_TEXT_LOG,
        LCD_SYM_TEXT_BODY,
        LCD_SYM_TEXT_SURF,
        LCD_SYM_DOT_BIGNUM,
        LCD_SYM_DOT_LOGNUM,

        __LCD_SYM_INVALID,

        NUM_LCD_SYMS,
};

enum lcd_seg_show_types {
        LCD_SHOW_NONE = 0,
        LCD_SHOW_NUM,
        LCD_SHOW_STR,
        NUM_LCD_SHOW_TYPE,
};

enum lcd_7seg_chars {
        LCD_7SEG_CHAR_SPACE = 0,

        LCD_7SEG_CHAR_0,
        LCD_7SEG_CHAR_1,
        LCD_7SEG_CHAR_2,
        LCD_7SEG_CHAR_3,
        LCD_7SEG_CHAR_4,
        LCD_7SEG_CHAR_5,
        LCD_7SEG_CHAR_6,
        LCD_7SEG_CHAR_7,
        LCD_7SEG_CHAR_8,
        LCD_7SEG_CHAR_9,

        LCD_7SEG_CHAR_A,
        LCD_7SEG_CHAR_b,
        LCD_7SEG_CHAR_C,
        LCD_7SEG_CHAR_c,
        LCD_7SEG_CHAR_d,
        LCD_7SEG_CHAR_E,
        LCD_7SEG_CHAR_F,
        LCD_7SEG_CHAR_G,
        LCD_7SEG_CHAR_H,
        LCD_7SEG_CHAR_h,
        LCD_7SEG_CHAR_I,
        LCD_7SEG_CHAR_i,
        LCD_7SEG_CHAR_J,
        LCD_7SEG_CHAR_L,
        LCD_7SEG_CHAR_n,
        LCD_7SEG_CHAR_O,
        LCD_7SEG_CHAR_o,
        LCD_7SEG_CHAR_P,
        LCD_7SEG_CHAR_q,
        LCD_7SEG_CHAR_r,
        LCD_7SEG_CHAR_S,
        LCD_7SEG_CHAR_t,
        LCD_7SEG_CHAR_U,
        LCD_7SEG_CHAR_u,
        LCD_7SEG_CHAR_y,

        LCD_7SEG_CHAR_DASH,
        LCD_7SEG_CHAR_ULINE,

        NUM_LCD_7SEG_CHAR,
};

struct lcd_7seg_char {
        /*
         * seg order:
         *
         *       aaa
         *      f   b
         *      f   b
         *       ggg
         *      e   c
         *      e   c
         *       ddd
         */
        uint8_t reg_data[LCD_SEG_PER_SLOT]; // [0] = LO [0++] = HI
};

struct lcd_sym {
        const uint8_t seg;
        const uint8_t bm;
        uint8_t       val;

        // to save space, removed now, reserved
        // void          (*set)(struct lcd_sym *, int enabled);
};

//
// slot order:
//
//        0      1      2     ...  
//
//       000    000    000    000  
//      0   0  0   0  0   0  0   0 
//      0   0  0   0  0   0  0   0 
//       000    000    000    000  
//      0   0  0   0  0   0  0   0 
//      0   0  0   0  0   0  0   0 
//       000    000    000    000  
//
//     |-slot-|
//     |--------- field ----------|
//

struct lcd_slot_addr {
        uint8_t seg;
        uint8_t cross_bank; // define it manually, for now..
};

struct lcd_field {
        uint8_t              sym_dot;

        uint8_t              buf[LCD_SLOT_CNT_MAX + 1];

        uint8_t              slot_cnt;
        uint8_t              slot_bm[LCD_SEG_PER_SLOT]; // all slot same bm

        struct lcd_slot_addr slot_addr[];
};

struct lcd_hw {
        uint8_t              com_cnt;
        uint8_t              seg_cnt;
        uint8_t              seg_per_slot;
        uint8_t              slot_cnt_max;
        struct lcd_7seg_char chars[NUM_LCD_7SEG_CHAR];
        struct lcd_sym      *syms[NUM_LCD_SYMS];
        struct lcd_field    *fields[NUM_LCD_FIELDS];
};

extern struct lcd_hw     *g_lcd_hw;
extern struct lcd_sym   **g_lcd_syms;
extern struct lcd_field **g_lcd_fields;
extern struct lcd_hw     *g_lcd_def_hw;

void lcd_init(struct lcd_hw *hw);

void lcd_ram_clear_all(void);
void lcd_ram_display_all(void);

void lcd_sw_blink(size_t cnt, size_t interval_ms);

void lcd_sys_init(void);
void lcd_sys_enable(int enable);

uint32_t lcd_ram_bank_read(int bank);
void lcd_ram_bank_write(int bank, uint32_t val);

int lcd_seg_same_bank_write(int seg, uint8_t *mask, uint8_t *data, size_t cnt);
int lcd_seg_cross_bank_write(int seg, uint8_t *mask, uint8_t *data, size_t cnt);

int lcd_seg_write(int seg, uint8_t mask, uint8_t data);
int lcd_seg_read(int seg, uint8_t *data);

struct lcd_field *lcd_field_get(uint32_t idx);
void lcd_field_dot_set(struct lcd_field *field, int dot);
void lcd_field_clear(struct lcd_field *field);

struct lcd_sym *lcd_sym_get(uint32_t idx);
void __lcd_sym_set(struct lcd_sym *sym, int enabled);
void __lcd_sym_set_apply(struct lcd_sym *sym, int enabled);
void lcd_sym_list_apply(void);

static __always_inline void lcd_sym_set(uint8_t sym_idx, int enabled)
{
        __lcd_sym_set(lcd_sym_get(sym_idx), enabled);
}

static __always_inline void lcd_sym_set_apply(uint8_t sym_idx, int enabled)
{
        __lcd_sym_set_apply(lcd_sym_get(sym_idx), enabled);
}

static __always_inline void lcd_sym_set_invert(uint8_t sym_idx)
{
        struct lcd_sym *sym = lcd_sym_get(sym_idx);

        if (!sym)
                return;

        __lcd_sym_set_apply(sym, !!!sym->val);
}

void lcd_display_clear(void);

int __lcd_string_show(struct lcd_field *field, char *buf, size_t len); // force show
int lcd_string_left_aligned_show(struct lcd_field *field, char *buf, size_t len); // for directed @field
int lcd_string_right_aligned_show(struct lcd_field *field, char *buf, size_t len);

int __lcd_number_show(struct lcd_field *field, uint8_t align, int32_t num,
                      uint8_t digit_cnt, uint8_t dot);

static __always_inline int lcd_string_show(uint8_t field_idx, uint8_t align,
                                           char *buf, size_t len)
{
        struct lcd_field *f = lcd_field_get(field_idx);
        int ret = 0;

        switch (align) {
        case LCD_ALIGN_RIGHT:
                ret = lcd_string_right_aligned_show(f, buf, len);
                break;

        case LCD_ALIGN_LEFT:
        default:
                ret = lcd_string_left_aligned_show(f, buf, len);
                break;
        }

        // showing string will hide dot by default
        lcd_field_dot_set(f, 0);

        return ret;
}

static __always_inline int lcd_number_show(uint8_t field_idx, uint8_t align,
                                           int32_t num, uint8_t digit_cnt,
                                           uint8_t dot)
{
        return __lcd_number_show(lcd_field_get(field_idx), align, num, digit_cnt, dot);
}

// NOTE: always show dot, last digit of @num is considered as float .1
static __always_inline int lcd_float1_show(uint8_t field_idx, int32_t num)
{
        return lcd_number_show(field_idx, LCD_ALIGN_RIGHT, num, 2, LCD_SHOW_DOT);
}

static __always_inline int lcd_temperature_show(uint8_t field_idx, int32_t num)
{
        return lcd_float1_show(field_idx, num);
}

// WARN: calling this too fast will cause screen flickers
static __always_inline void lcd_string_clean_show(uint8_t field_idx,
                                                  uint8_t align, uint16_t stay_ms,
                                                  char *buf, size_t len)
{
        lcd_display_clear();
        lcd_string_show(field_idx, align, buf, len);

        if (stay_ms)
                delay1ms(stay_ms);
}

static __always_inline void lcd_sym_temp_unit_set(uint8_t tunit)
{
        lcd_sym_set(LCD_SYM_TEMP_C, tunit == TUNIT_C);
        lcd_sym_set(LCD_SYM_TEMP_F, tunit == TUNIT_F);
}

static __always_inline void lcd_sym_scan_mode_set(uint8_t mode)
{
        lcd_sym_set(LCD_SYM_TEXT_BODY, mode == SCAN_BODY);
        lcd_sym_set(LCD_SYM_TEXT_SURF, mode == SCAN_SURFACE);
}

static __always_inline void lcd_sym_temp_unit_apply(uint8_t tunit)
{
        lcd_sym_set_apply(LCD_SYM_TEMP_C, tunit == TUNIT_C);
        lcd_sym_set_apply(LCD_SYM_TEMP_F, tunit == TUNIT_F);
}

static __always_inline void lcd_sym_scan_mode_apply(uint8_t mode)
{
        lcd_sym_set_apply(LCD_SYM_TEXT_BODY, mode == SCAN_BODY);
        lcd_sym_set_apply(LCD_SYM_TEXT_SURF, mode == SCAN_SURFACE);
}

static __always_inline void lcd_sym_bat_bar_hide(void)
{
        const uint8_t map[NUM_BAT_LVLS] = {
                [BAT_LVL_CRIT] = LCD_SYM_BAT,
                [BAT_LVL_LOW]  = LCD_SYM_BAT_LVL1,
                [BAT_LVL_NRM]  = LCD_SYM_BAT_LVL2,
                [BAT_LVL_HI]   = LCD_SYM_BAT_LVL3,
        };

        for (uint8_t i = 0; i < NUM_BAT_LVLS; i++)
                lcd_sym_set(map[i], 0);
}

static __always_inline void lcd_sym_bat_lvl_set(uint8_t lvl)
{
        const uint8_t map[NUM_BAT_LVLS] = {
                [BAT_LVL_CRIT] = LCD_SYM_BAT,
                [BAT_LVL_LOW]  = LCD_SYM_BAT_LVL1,
                [BAT_LVL_NRM]  = LCD_SYM_BAT_LVL2,
                [BAT_LVL_HI]   = LCD_SYM_BAT_LVL3,
        };

        for (uint8_t i = 0; i < NUM_BAT_LVLS; i++)
                lcd_sym_set(map[i], 0);

        for (uint8_t i = 0; i <= lvl; i++)
                lcd_sym_set(map[i], 1);
}

#endif /* __APP_LCD_H__ */