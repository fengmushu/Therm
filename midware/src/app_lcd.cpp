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
#include "rng.h"
#include "app_lcd.h"

#include "ssd1306.h"

#include "nano_engine.h"

/*
 * Define snowflake images directly in flash memory.
 * This reduces SRAM consumption.
 * The image is defined from bottom to top (bits), from left to right (bytes).
 */
const PROGMEM uint8_t snowFlakeImage[8][8] =
{
    {
        0B00111000,
        0B01010100,
        0B10010010,
        0B11111110,
        0B10010010,
        0B01010100,
        0B00111000,
        0B00000000
    },
    {
        0B00010000,
        0B01010100,
        0B00111000,
        0B11101110,
        0B00111000,
        0B01010100,
        0B00010000,
        0B00000000
    },
    {
        0B00111000,
        0B00010000,
        0B10111010,
        0B11101110,
        0B10111010,
        0B00010000,
        0B00111000,
        0B00000000
    },
    {
        0B00011000,
        0B01011010,
        0B00100100,
        0B11011011,
        0B11011011,
        0B00100100,
        0B01011010,
        0B00011000
    },
    {
        0B00010000,
        0B00111000,
        0B01010100,
        0B11101110,
        0B01010100,
        0B00111000,
        0B00010000,
        0B00000000
    },
    {
        0B10000010,
        0B00101000,
        0B01101100,
        0B00010000,
        0B01101100,
        0B00101000,
        0B10000010,
        0B00000000
    },
    {
        0B01000100,
        0B10101010,
        0B01101100,
        0B00010000,
        0B01101100,
        0B10101010,
        0B01000100,
        0B00000000
    },
    {
        0B00101000,
        0B01010100,
        0B10111010,
        0B01101100,
        0B10111010,
        0B01010100,
        0B00101000,
        0B00000000
    },
};

NanoEngine1 engine;

class SnowFlake: public NanoFixedSprite<NanoEngine1, engine>
{
public:
    SnowFlake(): NanoFixedSprite<NanoEngine1, engine>({0, 0}, {8, 8}, nullptr) { }

    bool isAlive() { return falling; }

    void bringToLife()
    {
        setBitmap( &snowFlakeImage[random(0, 8)][0] );
        /* Set initial position in scaled coordinates */
        scaled_position = { random(0, ssd1306_displayWidth() * 8), -8 * 8 };
        /* Use some random speed */
        speed = { random(-16, 16), random(4, 12) };
        /* After countdown timer ticks to 0, change X direction */
        timer = random(24, 48);
        moveTo( scaled_position/8 );
        falling = true;
    }

    void move()
    {
        scaled_position += speed;
        timer--;
        if (0 == timer)
        {
            /* Change movement direction */
            speed.x = random(-16, 16);
            timer = random(24, 48);
        }
        moveTo( scaled_position/8 );
        if (y() >= static_cast<lcdint_t>(ssd1306_displayHeight()) )
        {
            falling = false;
        }
    }

private:
    NanoPoint scaled_position;
    NanoPoint speed;
    uint8_t timer;
    bool falling = false;
};

static const uint8_t maxCount = 20;

/* These are our snow flakes */
SnowFlake snowFlakes[maxCount];

bool onDraw()
{
    engine.canvas.clear();
    for (uint8_t i=0; i<maxCount; i++)
    {
        if (snowFlakes[i].isAlive())
        {
            snowFlakes[i].draw();
        }
    }
    return true;
}

void addSnowFlake()
{
    for (uint8_t i=0; i<maxCount; i++)
    {
        if (!snowFlakes[i].isAlive())
        {
            snowFlakes[i].bringToLife();
            break;
        }
    }
}

static uint8_t globalTimer=3;

void moveSnowFlakes()
{
    for (uint8_t i=0; i<maxCount; i++)
    {
        if (snowFlakes[i].isAlive())
        {
            snowFlakes[i].move();
        }
    }
}

///< LCD 初始化
void sys_display_init(void)
{
    ssd1306_128x64_spi_init(-1,-1,-1);

    engine.setFrameRate( 10 );
    engine.begin();
    engine.drawCallback( onDraw );

    engine.canvas.setMode(CANVAS_MODE_TRANSPARENT);
    engine.refresh();
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
    if (!engine.nextFrame()) return;
    if (0 == (--globalTimer))
    {
        /* Try to add new snowflake every ~ 90ms */
        globalTimer = 3;
        addSnowFlake();
        ///< update location
        Rng_Generate();
    }
    moveSnowFlakes();
    engine.display();
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

