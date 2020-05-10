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
// const uint8_t Soba [] PROGMEM = {
// 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
// 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
// 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x80,
// 0x09, 0x00, 0x00, 0x00, 0xc0, 0x0c, 0x00, 0x00,
// 0x00, 0x60, 0x06, 0x00, 0x00, 0x00, 0x30, 0x03,
// 0x00, 0x00, 0x00, 0x98, 0x01, 0x00, 0xf8, 0x00,
// 0xcc, 0x00, 0x00, 0xde, 0x03, 0x66, 0x00, 0x80,
// 0x07, 0x0f, 0x33, 0x00, 0xc0, 0x01, 0x9c, 0x19,
// 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0xff,
// 0xff, 0xff, 0x07, 0xe0, 0xff, 0xff, 0xff, 0x07,
// 0xe0, 0xff, 0xff, 0xff, 0x07, 0xe0, 0xff, 0xff,
// 0xff, 0x07, 0xe0, 0xff, 0xff, 0xff, 0x07, 0xc0,
// 0xff, 0xff, 0xff, 0x03, 0xc0, 0xff, 0xff, 0xff,
// 0x03, 0x80, 0xff, 0xff, 0xff, 0x01, 0x80, 0xff,
// 0xff, 0xff, 0x01, 0x00, 0xff, 0xff, 0xff, 0x00,
// 0x00, 0xfe, 0xff, 0x7f, 0x00, 0x00, 0xfc, 0xff,
// 0x3f, 0x00, 0x00, 0xf8, 0xff, 0x1f, 0x00, 0x00,
// 0xf0, 0xff, 0x0f, 0x00, 0x00, 0xc0, 0xff, 0x03,
// 0x00, 0x00, 0x80, 0xff, 0x01, 0x00, 0x00, 0x00,
// 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
// };

/* Lets show 4 hearts on the display */
const int spritesCount = 4;

/* Declare variable that represents our 4 objects */
struct
{
    NanoPoint pos;
    NanoPoint speed;
} objects[ spritesCount ];

/*
 * Each pixel in SSD1306 display takes 1 bit of the memory. So, full resolution
 * of 128x64 LCD display will require 128*64/8 = 1024 bytes of SRAM for the buffer.
 * To let this example to run on Attiny devices (they have 256/512 byte SRAM), we
 * will use small canvas buffer: 32x32 (requires 128 bytes of SRAM), so the example
 * would run even on Attiny45.
 */
const int canvasWidth = 128; // Width must be power of 2, i.e. 16, 32, 64, 128...
const int canvasHeight = 64; // Height must be divided on 8, i.e. 8, 16, 24, 32...
uint8_t canvasData[canvasWidth*(canvasHeight/8)];
/* Create canvas object */
NanoCanvas1 canvas(canvasWidth, canvasHeight, canvasData);

/*
 * Define sprite width. The width can be of any size.
 * But sprite height is always assumed to be 8 pixels
 * (number of bits in single byte).
 */
// const int spriteWidth = sizeof(heartImage);

// /* Declare variable that represents our sprite */
// SPRITE sprite;
// int speedX = 1;
// int speedY = 1;


///< LCD 初始化
void sys_display_init(void)
{
    ssd1306_128x64_spi_init(-1,-1,-1);
    ssd1306_fillScreen(0x00);

    /* Create 4 "hearts", and place them at different positions and give different movement direction */
    for(uint8_t i = 0; i < spritesCount; i++)
    {
        objects[i].speed = { .x = (i & 1) ? -1:  1, .y = (i & 2) ? -1:  1 };
        objects[i].pos = { .x = i*4, .y = i*4 + 2 };
    }
    canvas.setMode( CANVAS_MODE_TRANSPARENT );

    /* Create sprite at 0,0 position. The function initializes sprite structure. */
    // sprite = ssd1306_createSprite( 0, 0, spriteWidth, heartImage );
    // /* Draw sprite on the display */
    // sprite.draw();
}

void AppLcdDisplayAll(void)
{
    ssd1306_fillScreen(0xFF);
}

void AppLcdClearAll(void)
{
    ssd1306_fillScreen(0x00);
}

void AppLcdDisplayUpdate(uint32_t delay_ms)
{
    /* Move sprite every 40 milliseconds */
    // delay(delay_ms);

    /* Recalculate position and movement direction of all 4 "hearts" */
    for (uint8_t i = 0; i < spritesCount; i++)
    {
        objects[i].pos += objects[i].speed;
        /* If left or right boundary is reached, reverse X direction */
        if ((objects[i].pos.x == (canvasWidth - 8)) || (objects[i].pos.x == 0))
            objects[i].speed.x = -objects[i].speed.x;
        /* Sprite height is always 8 pixels. Reverse Y direction if bottom or top boundary is reached. */
        if ((objects[i].pos.y == (canvasHeight - 8)) || (objects[i].pos.y == 0))
            objects[i].speed.y = -objects[i].speed.y;
    }

    /* Clear canvas surface */
    canvas.clear();
    /* Draw line */
    canvas.drawLine( 0, 0, canvasWidth - 1, canvasHeight-1);
    /* Draw rectangle around our canvas. It will show the range of the canvas on the display */
    canvas.drawRect(0, 0, canvasWidth-1, canvasHeight-1);
    /* Draw all 4 sprites on the canvas */
    for (uint8_t i = 0; i < spritesCount; i++)
    {
        canvas.drawBitmap1( objects[i].pos.x, objects[i].pos.y, 8, 8, heartImage );
    }
    /* Now, draw canvas on the display */
    canvas.blt(0, 0);

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


    // ssd1306_clearScreen( );
    // ssd1306_drawXBitmap(0, 0, 40, 32, Soba);
    // delay(1000);
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

