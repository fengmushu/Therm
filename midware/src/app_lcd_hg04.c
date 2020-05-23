#include <stdlib.h>
#include <stdint.h>
#include "utils.h"

#include "app_data.h"
#include "app_lcd.h"

#ifdef HW_BOARD_HG04

static struct lcd_field field_bignum = {
        .sym_dot   = LCD_SYM_DOT_BIGNUM,
        .slot_cnt  = 4,
        .slot_bm   = { 0b00001110, 0b00001111 }, // LSB -- MSB
        .slot_addr = {
                [0] = { 1,  SAME_BANK  },
                [1] = { 3,  CROSS_BANK },
                [2] = { 9,  SAME_BANK  },
                [3] = { 11, CROSS_BANK },
        },
};

static struct lcd_field field_idxnum = {
        .sym_dot   = __LCD_SYM_INVALID,
        .slot_cnt  = 2,
        .slot_bm   = { 0b00001110, 0b00001111 },
        .slot_addr = {
                [0] = { 5, SAME_BANK  },
                [1] = { 7, CROSS_BANK },
        },
};

struct lcd_hw lcd_hg04 = {
        .com_cnt      = LCD_COM_CNT,
        .seg_cnt      = LCD_SEG_CNT,
        .seg_per_slot = LCD_SEG_PER_SLOT,
        .slot_cnt_max = LCD_SLOT_CNT_MAX,
        .chars = {
                //                        LSB    [RAM REG]    MSB
                //                              fge_,       abcd
                [LCD_7SEG_CHAR_SPACE] = { 0b00000000, 0b00000000 },
                [LCD_7SEG_CHAR_0]     = { 0b00001010, 0b00001111 },
                [LCD_7SEG_CHAR_1]     = { 0b00000000, 0b00000110 },
                [LCD_7SEG_CHAR_2]     = { 0b00000110, 0b00001101 },
                [LCD_7SEG_CHAR_3]     = { 0b00000100, 0b00001111 },
                [LCD_7SEG_CHAR_4]     = { 0b00001100, 0b00000110 },
                [LCD_7SEG_CHAR_5]     = { 0b00001100, 0b00001011 },
                [LCD_7SEG_CHAR_6]     = { 0b00001110, 0b00001011 },
                [LCD_7SEG_CHAR_7]     = { 0b00000000, 0b00001110 },
                [LCD_7SEG_CHAR_8]     = { 0b00001110, 0b00001111 },
                [LCD_7SEG_CHAR_9]     = { 0b00001100, 0b00001111 },
                [LCD_7SEG_CHAR_A]     = { 0b00001110, 0b00001110 },
                [LCD_7SEG_CHAR_b]     = { 0b00001110, 0b00000011 },
                [LCD_7SEG_CHAR_C]     = { 0b00001010, 0b00001001 },
                [LCD_7SEG_CHAR_c]     = { 0b00000110, 0b00000001 },
                [LCD_7SEG_CHAR_d]     = { 0b00000110, 0b00000111 },
                [LCD_7SEG_CHAR_E]     = { 0b00001110, 0b00001001 },
                [LCD_7SEG_CHAR_F]     = { 0b00001110, 0b00001000 },
                [LCD_7SEG_CHAR_G]     = { 0b00001010, 0b00001011 },
                [LCD_7SEG_CHAR_H]     = { 0b00001110, 0b00000110 },
                [LCD_7SEG_CHAR_h]     = { 0b00001110, 0b00000010 },
                [LCD_7SEG_CHAR_I]     = { 0b00001010, 0b00000000 },
                [LCD_7SEG_CHAR_i]     = { 0b00000000, 0b00000010 },
                [LCD_7SEG_CHAR_J]     = { 0b00000010, 0b00000111 },
                [LCD_7SEG_CHAR_L]     = { 0b00001010, 0b00000001 },
                [LCD_7SEG_CHAR_n]     = { 0b00000110, 0b00000010 },
                [LCD_7SEG_CHAR_O]     = { 0b00001010, 0b00001111 },
                [LCD_7SEG_CHAR_o]     = { 0b00000110, 0b00000011 },
                [LCD_7SEG_CHAR_P]     = { 0b00001110, 0b00001100 },
                [LCD_7SEG_CHAR_q]     = { 0b00001100, 0b00001110 },
                [LCD_7SEG_CHAR_r]     = { 0b00000110, 0b00000000 },
                [LCD_7SEG_CHAR_S]     = { 0b00001100, 0b00001011 },
                [LCD_7SEG_CHAR_t]     = { 0b00001110, 0b00000001 },
                [LCD_7SEG_CHAR_U]     = { 0b00001010, 0b00000111 },
                [LCD_7SEG_CHAR_u]     = { 0b00000010, 0b00000011 },
                [LCD_7SEG_CHAR_y]     = { 0b00001100, 0b00000111 },
                [LCD_7SEG_CHAR_DASH]  = { 0b00000100, 0b00000000 },
                [LCD_7SEG_CHAR_ULINE] = { 0b00000000, 0b00000001 },
        },
        // inited as NOT enabled, sync by lcd_clear_all()
        .syms = { // val needs shift, fit bm
                [LCD_SYM_BAT]         = &(struct lcd_sym){ 13, BIT(0), 0 },
                [LCD_SYM_BAT_LVL1]    = &(struct lcd_sym){ 0,  BIT(3), 0 }, // B4
                [LCD_SYM_BAT_LVL2]    = &(struct lcd_sym){ 0,  BIT(2), 0 }, // B3
                [LCD_SYM_BAT_LVL3]    = &(struct lcd_sym){ 9,  BIT(0), 0 }, // B2
                [LCD_SYM_EMOJI_SMILE] = &(struct lcd_sym){ 1,  BIT(0), 0 },
                [LCD_SYM_EMOJI_CRY]   = &(struct lcd_sym){ 3,  BIT(0), 0 },
                [LCD_SYM_BLUETOOTH]   = &(struct lcd_sym){ 0,  BIT(0), 0 },
                [LCD_SYM_LOCK]        = NULL,
                [LCD_SYM_BUZZER]      = &(struct lcd_sym){ 13, BIT(1), 0 },
                [LCD_SYM_TEMP_C]      = &(struct lcd_sym){ 13, BIT(3), 0 },
                [LCD_SYM_TEMP_F]      = &(struct lcd_sym){ 13, BIT(2), 0 },
                [LCD_SYM_TEXT_LOG]    = &(struct lcd_sym){ 7,  BIT(0), 0 },
                [LCD_SYM_TEXT_BODY]   = &(struct lcd_sym){ 0,  BIT(1), 0 },
                [LCD_SYM_TEXT_SURF]   = &(struct lcd_sym){ 5,  BIT(0), 0 },
                [LCD_SYM_DOT_BIGNUM]  = &(struct lcd_sym){ 11, BIT(0), 0 },
                [LCD_SYM_DOT_LOGNUM]  = NULL,
                [__LCD_SYM_INVALID]   = NULL,
        },
        .fields = {
                [LCD_BIGNUM] = &field_bignum,
                [LCD_LOGNUM] = NULL,
                [LCD_IDXNUM] = &field_idxnum,
        },
};

struct lcd_hw *g_lcd_def_hw = &lcd_hg04;

#endif /* HW_BOARD_HG04 */