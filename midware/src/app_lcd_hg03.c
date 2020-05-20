#include <stdlib.h>
#include <stdint.h>
#include "utils.h"

#include "app_data.h"
#include "app_lcd.h"
#include "app_lcd_hg03.h"

#ifdef HW_BOARD_HG03

static struct lcd_field field_bignum = {
        .sym_dot   = LCD_SYM_DOT_BIGNUM,
        .slot_cnt  = 4,
        .slot_bm   = { 0b00000111, 0b00001111 }, // LSB -- MSB
        .slot_addr = {
                [0] = { 18, 0 }, // 18-19, lcd_seg_write() lower seg first
                [1] = { 16, 0 },
                [2] = { 14, 0 },
                [3] = { 12, 0 },
        },
};

static struct lcd_field field_lognum = {
        .sym_dot   = LCD_SYM_DOT_LOGNUM,
        .slot_cnt  = 4,
        .slot_bm   = { 0b00000111, 0b00001111 },
        .slot_addr = {
                [0] = { 6, 0 },
                [1] = { 4, 0 },
                [2] = { 2, 0 },
                [3] = { 0, 0 },
        },
};

static struct lcd_field field_idxnum = {
        .sym_dot   = __LCD_SYM_INVALID,
        .slot_cnt  = 2,
        .slot_bm   = { 0b00000111, 0b00001111 },
        .slot_addr = {
                [0] = { 10, 0 },
                [1] = {  8, 0 },
        },
};

struct lcd_hw lcd_hg03 = {
        .com_cnt      = 4,
        .seg_cnt      = 20,
        .seg_per_slot = 2,
        .slot_cnt_max = 4,
        .chars = {
                //                        LSB    [RAM REG]    MSB
                //                              _cgb        defa
                [LCD_7SEG_CHAR_SPACE] = { 0b00000000, 0b00000000 },
                [LCD_7SEG_CHAR_0]     = { 0b00000101, 0b00001111 },
                [LCD_7SEG_CHAR_1]     = { 0b00000101, 0b00000000 },
                [LCD_7SEG_CHAR_2]     = { 0b00000011, 0b00001101 },
                [LCD_7SEG_CHAR_3]     = { 0b00000111, 0b00001001 },
                [LCD_7SEG_CHAR_4]     = { 0b00000111, 0b00000010 },
                [LCD_7SEG_CHAR_5]     = { 0b00000110, 0b00001011 },
                [LCD_7SEG_CHAR_6]     = { 0b00000110, 0b00001111 },
                [LCD_7SEG_CHAR_7]     = { 0b00000101, 0b00000001 },
                [LCD_7SEG_CHAR_8]     = { 0b00000111, 0b00001111 },
                [LCD_7SEG_CHAR_9]     = { 0b00000111, 0b00001011 },
                [LCD_7SEG_CHAR_A]     = { 0b00000111, 0b00000111 },
                [LCD_7SEG_CHAR_b]     = { 0b00000110, 0b00001110 },
                [LCD_7SEG_CHAR_C]     = { 0b00000000, 0b00001111 },
                [LCD_7SEG_CHAR_c]     = { 0b00000010, 0b00001100 },
                [LCD_7SEG_CHAR_d]     = { 0b00000111, 0b00001100 },
                [LCD_7SEG_CHAR_E]     = { 0b00000010, 0b00001111 },
                [LCD_7SEG_CHAR_F]     = { 0b00000010, 0b00000111 },
                [LCD_7SEG_CHAR_G]     = { 0b00000100, 0b00001111 },
                [LCD_7SEG_CHAR_H]     = { 0b00000111, 0b00000110 },
                [LCD_7SEG_CHAR_h]     = { 0b00000110, 0b00000110 },
                [LCD_7SEG_CHAR_I]     = { 0b00000000, 0b00000110 },
                [LCD_7SEG_CHAR_i]     = { 0b00000100, 0b00000000 },
                [LCD_7SEG_CHAR_J]     = { 0b00000101, 0b00001100 },
                [LCD_7SEG_CHAR_L]     = { 0b00000000, 0b00001110 },
                [LCD_7SEG_CHAR_n]     = { 0b00000110, 0b00000100 },
                [LCD_7SEG_CHAR_O]     = { 0b00000101, 0b00001111 },
                [LCD_7SEG_CHAR_o]     = { 0b00000110, 0b00001100 },
                [LCD_7SEG_CHAR_P]     = { 0b00000011, 0b00000111 },
                [LCD_7SEG_CHAR_q]     = { 0b00000111, 0b00000011 },
                [LCD_7SEG_CHAR_r]     = { 0b00000010, 0b00000100 },
                [LCD_7SEG_CHAR_S]     = { 0b00000110, 0b00001011 },
                [LCD_7SEG_CHAR_t]     = { 0b00000010, 0b00001110 },
                [LCD_7SEG_CHAR_U]     = { 0b00000101, 0b00001110 },
                [LCD_7SEG_CHAR_u]     = { 0b00000100, 0b00001100 },
                [LCD_7SEG_CHAR_y]     = { 0b00000111, 0b00001010 },
                [LCD_7SEG_CHAR_DASH]  = { 0b00000010, 0b00000000 },
                [LCD_7SEG_CHAR_ULINE] = { 0b00000000, 0b00001000 },
        },
        // inited as NOT enabled, sync by lcd_clear_all()
        .syms = { // val needs shift, fit bm
                [LCD_SYM_BAT]        = &(struct lcd_sym){ 0,  BIT(3), 0 },
                [LCD_SYM_LOCK]       = &(struct lcd_sym){ 8,  BIT(3), 0 },
                [LCD_SYM_BUZZER]     = &(struct lcd_sym){ 12, BIT(3), 0 },
                [LCD_SYM_TEMP_C]     = &(struct lcd_sym){ 6,  BIT(3), 0 },
                [LCD_SYM_TEMP_F]     = &(struct lcd_sym){ 4,  BIT(3), 0 },
                [LCD_SYM_TEXT_LOG]   = &(struct lcd_sym){ 10, BIT(3), 0 },
                [LCD_SYM_TEXT_BODY]  = &(struct lcd_sym){ 16, BIT(3), 0 },
                [LCD_SYM_TEXT_SURF]  = &(struct lcd_sym){ 18, BIT(3), 0 },
                [LCD_SYM_DOT_BIGNUM] = &(struct lcd_sym){ 14, BIT(3), 0 },
                [LCD_SYM_DOT_LOGNUM] = &(struct lcd_sym){ 2,  BIT(3), 0 },
                [__LCD_SYM_INVALID]  = NULL,
        },
        .fields = {
                [LCD_BIGNUM] = &field_bignum,
                [LCD_LOGNUM] = &field_lognum,
                [LCD_IDXNUM] = &field_idxnum,
        },
};

struct lcd_hw *g_lcd_def_hw = &lcd_hg03;

#endif