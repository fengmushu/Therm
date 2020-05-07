/******************************************************************************
* Copyright (C) 2019, Huada Semiconductor Co.,Ltd All rights reserved.
*
* This software is owned and published by:
* Huada Semiconductor Co.,Ltd ("HDSC").
*
* BY DOWNLOADING, INSTALLING OR USING THIS SOFTWARE, YOU AGREE TO BE BOUND
* BY ALL THE TERMS AND CONDITIONS OF THIS AGREEMENT.
*
* This software contains source code for use with HDSC
* components. This software is licensed by HDSC to be adapted only
* for use in systems utilizing HDSC components. HDSC shall not be
* responsible for misuse or illegal use of this software for devices not
* supported herein. HDSC is providing this software "AS IS" and will
* not be responsible for issues arising from incorrect user implementation
* of the software.
*
* Disclaimer:
* HDSC MAKES NO WARRANTY, EXPRESS OR IMPLIED, ARISING BY LAW OR OTHERWISE,
* REGARDING THE SOFTWARE (INCLUDING ANY ACOOMPANYING WRITTEN MATERIALS),
* ITS PERFORMANCE OR SUITABILITY FOR YOUR INTENDED USE, INCLUDING,
* WITHOUT LIMITATION, THE IMPLIED WARRANTY OF MERCHANTABILITY, THE IMPLIED
* WARRANTY OF FITNESS FOR A PARTICULAR PURPOSE OR USE, AND THE IMPLIED
* WARRANTY OF NONINFRINGEMENT.
* HDSC SHALL HAVE NO LIABILITY (WHETHER IN CONTRACT, WARRANTY, TORT,
* NEGLIGENCE OR OTHERWISE) FOR ANY DAMAGES WHATSOEVER (INCLUDING, WITHOUT
* LIMITATION, DAMAGES FOR LOSS OF BUSINESS PROFITS, BUSINESS INTERRUPTION,
* LOSS OF BUSINESS INFORMATION, OR OTHER PECUNIARY LOSS) ARISING FROM USE OR
* INABILITY TO USE THE SOFTWARE, INCLUDING, WITHOUT LIMITATION, ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL OR CONSEQUENTIAL DAMAGES OR LOSS OF DATA,
* SAVINGS OR PROFITS,
* EVEN IF Disclaimer HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
* YOU ASSUME ALL RESPONSIBILITIES FOR SELECTION OF THE SOFTWARE TO ACHIEVE YOUR
* INTENDED RESULTS, AND FOR THE INSTALLATION OF, USE OF, AND RESULTS OBTAINED
* FROM, THE SOFTWARE.
*
* This software may be replicated in part or whole for the licensed use,
* with the restriction that this Disclaimer and Copyright notice must be
* included with each copy of this software, whether used in part or whole,
* at all times.
*/
/******************************************************************************/
/** \file lcd.c
 **
 ** lcd driver API.
 **
 **   - 2020-04-03    First Version
 **
 ******************************************************************************/

/******************************************************************************
 * Include files
 ******************************************************************************/
#include "lcd.h"
#include "app.h"
#include "app_lcd.h"

#include "ssd1306.h"

#include "nano_engine.h"

NanoEngine<TILE_128x64_MONO> engine;

/* 
 * Heart image below is defined directly in flash memory.
 * This reduces SRAM consumption.
 * The image is defined from bottom to top (bits), from left to
 * right (bytes).
 */
const PROGMEM uint8_t heartImage[8] =
{
    0B00001110,
    0B00011111,
    0B00111111,
    0B01111110,
    0B01111110,
    0B00111101,
    0B00011001,
    0B00001110
};

/**
 * Soba bitmap source is generated via script from https://github.com/robertgallup/bmp2hex
 * MIT license
 */
const uint8_t Soba [] PROGMEM = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x80,
0x09, 0x00, 0x00, 0x00, 0xc0, 0x0c, 0x00, 0x00,
0x00, 0x60, 0x06, 0x00, 0x00, 0x00, 0x30, 0x03,
0x00, 0x00, 0x00, 0x98, 0x01, 0x00, 0xf8, 0x00,
0xcc, 0x00, 0x00, 0xde, 0x03, 0x66, 0x00, 0x80,
0x07, 0x0f, 0x33, 0x00, 0xc0, 0x01, 0x9c, 0x19,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0xff,
0xff, 0xff, 0x07, 0xe0, 0xff, 0xff, 0xff, 0x07,
0xe0, 0xff, 0xff, 0xff, 0x07, 0xe0, 0xff, 0xff,
0xff, 0x07, 0xe0, 0xff, 0xff, 0xff, 0x07, 0xc0,
0xff, 0xff, 0xff, 0x03, 0xc0, 0xff, 0xff, 0xff,
0x03, 0x80, 0xff, 0xff, 0xff, 0x01, 0x80, 0xff,
0xff, 0xff, 0x01, 0x00, 0xff, 0xff, 0xff, 0x00,
0x00, 0xfe, 0xff, 0x7f, 0x00, 0x00, 0xfc, 0xff,
0x3f, 0x00, 0x00, 0xf8, 0xff, 0x1f, 0x00, 0x00,
0xf0, 0xff, 0x0f, 0x00, 0x00, 0xc0, 0xff, 0x03,
0x00, 0x00, 0x80, 0xff, 0x01, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

/*
 * Define sprite width. The width can be of any size.
 * But sprite height is always assumed to be 8 pixels
 * (number of bits in single byte).
 */
const int spriteWidth = sizeof(heartImage);

/* Declare variable that represents our sprite */
SPRITE sprite;
int speedX = 1;
int speedY = 1;

///< LCD 初始化
void sys_display_init(void)
{
    ssd1306_128x64_spi_init(-1,-1,-1);
    ssd1306_fillScreen(0xFF);

    /* Create sprite at 0,0 position. The function initializes sprite structure. */
    sprite = ssd1306_createSprite( 0, 0, spriteWidth, heartImage );
    /* Draw sprite on the display */
    sprite.draw();
}

void AppLcdDisplayAll(void)
{
    ssd1306_fillScreen(0x00);
}

void AppLcdClearAll(void)
{
    ssd1306_fillScreen(0x01);
}

void AppLcdDisplayUpdate(uint32_t delay_ms)
{
    // /* Move sprite every 40 milliseconds */
    // delay(delay_ms);
    // sprite.x += speedX;
    // sprite.y += speedY;
    // /* If right boundary is reached, reverse X direction */
    // if (sprite.x == (128 - spriteWidth)) speedX = -speedX;
    // /* If left boundary is reached, reverse X direction */ 
    // if (sprite.x == 0) speedX = -speedX;
    // /* Sprite height is always 8 pixels. Reverse Y direction if bottom boundary is reached. */
    // if (sprite.y == (64 - 8)) speedY = -speedY;
    // /* If top boundary is reached, reverse Y direction */
    // if (sprite.y == 0) speedY = -speedY;
    // /* Erase sprite on old place. The library knows old position of the sprite. */
    // sprite.eraseTrace();
    // /* Draw sprite on new place */
    // sprite.draw();


    ssd1306_clearScreen( );
    ssd1306_drawXBitmap(0, 0, 40, 32, Soba);
    delay(3000);
}

void AppLcdDisable(void)
{
    ssd1306_displayOff();
}

void AppLcdEnable(void)
{
    ssd1306_displayOn();
}

/******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/

