#include "oled.h"
#include "oledfont.h"   /* 与 oled.c 同目录，由 PRIV_INCLUDE_DIRS 找到 */

#include <string.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* ---------- GPIO 操作宏 ---------- */
#define OLED_SCL_Set()  gpio_set_level(OLED_SCL_PIN, 1)
#define OLED_SCL_Clr()  gpio_set_level(OLED_SCL_PIN, 0)
#define OLED_SDA_Set()  gpio_set_level(OLED_SDA_PIN, 1)
#define OLED_SDA_Clr()  gpio_set_level(OLED_SDA_PIN, 0)
/* --------------------------------- */

static uint8_t OLED_GRAM[128][8];

static inline void IIC_delay(void)
{
    esp_rom_delay_us(2);
}

/* ==================== 软件 I2C ==================== */

void OLED_I2C_Start(void)
{
    OLED_SDA_Set(); OLED_SCL_Set(); IIC_delay();
    OLED_SDA_Clr(); IIC_delay();
    OLED_SCL_Clr(); IIC_delay();
}

void OLED_I2C_Stop(void)
{
    OLED_SDA_Clr(); OLED_SCL_Set(); IIC_delay();
    OLED_SDA_Set(); IIC_delay();
}

void OLED_I2C_WaitAck(void)
{
    OLED_SDA_Set(); IIC_delay();
    OLED_SCL_Set(); IIC_delay();
    OLED_SCL_Clr(); IIC_delay();
}

void OLED_Send_Byte(uint8_t dat)
{
    for (int i = 0; i < 8; i++) {
        if (dat & 0x80) OLED_SDA_Set();
        else            OLED_SDA_Clr();
        IIC_delay();
        OLED_SCL_Set(); IIC_delay();
        OLED_SCL_Clr();
        dat <<= 1;
    }
}

void OLED_WR_Byte(uint8_t dat, uint8_t mode)
{
    OLED_I2C_Start();
    OLED_Send_Byte(0x78);
    OLED_I2C_WaitAck();
    OLED_Send_Byte(mode ? 0x40 : 0x00);
    OLED_I2C_WaitAck();
    OLED_Send_Byte(dat);
    OLED_I2C_WaitAck();
    OLED_I2C_Stop();
}

/* ==================== 显示控制 ==================== */

void OLED_ColorTurn(uint8_t i)
{
    OLED_WR_Byte(i ? 0xA7 : 0xA6, OLED_CMD);
}

void OLED_DisplayTurn(uint8_t i)
{
    if (i == 0) { OLED_WR_Byte(0xC8, OLED_CMD); OLED_WR_Byte(0xA1, OLED_CMD); }
    else        { OLED_WR_Byte(0xC0, OLED_CMD); OLED_WR_Byte(0xA0, OLED_CMD); }
}

void OLED_DisPlay_On(void)
{
    OLED_WR_Byte(0x8D, OLED_CMD);
    OLED_WR_Byte(0x14, OLED_CMD);
    OLED_WR_Byte(0xAF, OLED_CMD);
}

void OLED_DisPlay_Off(void)
{
    OLED_WR_Byte(0x8D, OLED_CMD);
    OLED_WR_Byte(0x10, OLED_CMD);
    OLED_WR_Byte(0xAE, OLED_CMD);
}

void OLED_Refresh(void)
{
    for (int i = 0; i < 8; i++) {
        OLED_WR_Byte(0xB0 + i, OLED_CMD);
        OLED_WR_Byte(0x00,     OLED_CMD);
        OLED_WR_Byte(0x10,     OLED_CMD);
        OLED_I2C_Start();
        OLED_Send_Byte(0x78); OLED_I2C_WaitAck();
        OLED_Send_Byte(0x40); OLED_I2C_WaitAck();
        for (int n = 0; n < 128; n++) {
            OLED_Send_Byte(OLED_GRAM[n][i]);
            OLED_I2C_WaitAck();
        }
        OLED_I2C_Stop();
    }
}

void OLED_Clear(void)
{
    memset(OLED_GRAM, 0, sizeof(OLED_GRAM));
    OLED_Refresh();
}

/* ==================== 绘图 ==================== */

void OLED_DrawPoint(uint8_t x, uint8_t y, uint8_t t)
{
    if (x > 127 || y > 63) return;
    uint8_t page = y / 8;
    uint8_t bit  = 1 << (y % 8);
    if (t) OLED_GRAM[x][page] |=  bit;
    else   OLED_GRAM[x][page] &= ~bit;
}

void OLED_DrawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t mode)
{
    int xerr = 0, yerr = 0;
    int delta_x = x2 - x1, delta_y = y2 - y1;
    int incx = (delta_x > 0) ? 1 : (delta_x < 0) ? -1 : 0;
    int incy = (delta_y > 0) ? 1 : (delta_y < 0) ? -1 : 0;
    if (delta_x < 0) delta_x = -delta_x;
    if (delta_y < 0) delta_y = -delta_y;
    int distance = (delta_x > delta_y) ? delta_x : delta_y;
    int uRow = x1, uCol = y1;
    for (int t = 0; t <= distance; t++) {
        OLED_DrawPoint(uRow, uCol, mode);
        xerr += delta_x; yerr += delta_y;
        if (xerr > distance) { xerr -= distance; uRow += incx; }
        if (yerr > distance) { yerr -= distance; uCol += incy; }
    }
}

void OLED_DrawCircle(uint8_t x, uint8_t y, uint8_t r)
{
    int a = 0, b = r;
    while (2 * b * b >= r * r) {
        OLED_DrawPoint(x+a, y-b, 1); OLED_DrawPoint(x-a, y-b, 1);
        OLED_DrawPoint(x-a, y+b, 1); OLED_DrawPoint(x+a, y+b, 1);
        OLED_DrawPoint(x+b, y+a, 1); OLED_DrawPoint(x+b, y-a, 1);
        OLED_DrawPoint(x-b, y-a, 1); OLED_DrawPoint(x-b, y+a, 1);
        a++;
        int num = a*a + b*b - r*r;
        if (num > 0) { b--; a--; }
    }
}

/* ==================== 字符显示 ==================== */

void OLED_ShowChar(uint8_t x, uint8_t y, uint8_t chr,
                   uint8_t size1, uint8_t mode)
{
    uint8_t size2 = (size1 == 8) ? 6
                  : (size1 / 8 + ((size1 % 8) ? 1 : 0)) * (size1 / 2);
    uint8_t chr1 = chr - ' ';
    uint8_t x0 = x, y0 = y;

    for (int i = 0; i < size2; i++) {
        uint8_t temp;
        if      (size1 == 8)  temp = asc2_0806[chr1][i];
        else if (size1 == 12) temp = asc2_1206[chr1][i];
        else if (size1 == 16) temp = asc2_1608[chr1][i];
        else if (size1 == 24) temp = asc2_2412[chr1][i];
        else return;

        for (int m = 0; m < 8; m++) {
            OLED_DrawPoint(x, y, (temp & 0x01) ? mode : !mode);
            temp >>= 1;
            y++;
        }
        x++;
        if ((size1 != 8) && ((x - x0) == size1 / 2)) { x = x0; y0 += 8; }
        y = y0;
    }
    OLED_Refresh();
}

void OLED_ShowString(uint8_t x, uint8_t y, const char *chr,
                     uint8_t size1, uint8_t mode)
{
    while (*chr >= ' ' && *chr <= '~') {
        OLED_ShowChar(x, y, (uint8_t)*chr, size1, mode);
        x += (size1 == 8) ? 6 : (size1 / 2);
        chr++;
    }
    OLED_Refresh();
}

static uint32_t OLED_Pow(uint8_t m, uint8_t n)
{
    uint32_t r = 1;
    while (n--) r *= m;
    return r;
}

void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num,
                  uint8_t len, uint8_t size1, uint8_t mode)
{
    uint8_t m = (size1 == 8) ? 2 : 0;
    for (int t = 0; t < len; t++) {
        uint8_t temp = (num / OLED_Pow(10, len - t - 1)) % 10;
        OLED_ShowChar(x + (size1/2 + m)*t, y, temp+'0', size1, mode);
    }
    OLED_Refresh();
}

void OLED_ShowChinese(uint8_t x, uint8_t y, uint8_t num,
                      uint8_t size1, uint8_t mode)
{
    uint16_t size3 = (size1/8 + ((size1%8)?1:0)) * size1;
    uint8_t x0 = x, y0 = y;
    for (int i = 0; i < size3; i++) {
        uint8_t temp;
        if      (size1 == 16) temp = Hzk1[num][i];
        else if (size1 == 24) temp = Hzk2[num][i];
        else if (size1 == 32) temp = Hzk3[num][i];
        else if (size1 == 64) temp = Hzk4[num][i];
        else return;
        for (int m = 0; m < 8; m++) {
            OLED_DrawPoint(x, y, (temp & 0x01) ? mode : !mode);
            temp >>= 1; y++;
        }
        x++;
        if ((x - x0) == size1) { x = x0; y0 += 8; }
        y = y0;
    }
    OLED_Refresh();
}

void OLED_ShowPicture(uint8_t x, uint8_t y, uint8_t sizex, uint8_t sizey,
                      const uint8_t BMP[], uint8_t mode)
{
    uint16_t j = 0;
    uint8_t x0 = x, y0 = y;
    uint8_t pages = sizey/8 + ((sizey%8)?1:0);
    for (int n = 0; n < pages; n++) {
        for (int i = 0; i < sizex; i++) {
            uint8_t temp = BMP[j++];
            for (int m = 0; m < 8; m++) {
                OLED_DrawPoint(x, y, (temp & 0x01) ? mode : !mode);
                temp >>= 1; y++;
            }
            x++;
            if ((x - x0) == sizex) { x = x0; y0 += 8; }
            y = y0;
        }
    }
    OLED_Refresh();
}

/* ==================== 初始化 ==================== */

void OLED_Init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << OLED_SCL_PIN) | (1ULL << OLED_SDA_PIN),
        .mode         = GPIO_MODE_OUTPUT_OD,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    OLED_SCL_Set();
    OLED_SDA_Set();
    vTaskDelay(pdMS_TO_TICKS(200));

    OLED_WR_Byte(0xAE, OLED_CMD);
    OLED_WR_Byte(0x00, OLED_CMD);
    OLED_WR_Byte(0x10, OLED_CMD);
    OLED_WR_Byte(0x40, OLED_CMD);
    OLED_WR_Byte(0x81, OLED_CMD);
    OLED_WR_Byte(0xCF, OLED_CMD);
    OLED_WR_Byte(0xA1, OLED_CMD);
    OLED_WR_Byte(0xC8, OLED_CMD);
    OLED_WR_Byte(0xA6, OLED_CMD);
    OLED_WR_Byte(0xA8, OLED_CMD);
    OLED_WR_Byte(0x3F, OLED_CMD);
    OLED_WR_Byte(0xD3, OLED_CMD);
    OLED_WR_Byte(0x00, OLED_CMD);
    OLED_WR_Byte(0xD5, OLED_CMD);
    OLED_WR_Byte(0x80, OLED_CMD);
    OLED_WR_Byte(0xD9, OLED_CMD);
    OLED_WR_Byte(0xF1, OLED_CMD);
    OLED_WR_Byte(0xDA, OLED_CMD);
    OLED_WR_Byte(0x12, OLED_CMD);
    OLED_WR_Byte(0xDB, OLED_CMD);
    OLED_WR_Byte(0x30, OLED_CMD);
    OLED_WR_Byte(0x20, OLED_CMD);
    OLED_WR_Byte(0x02, OLED_CMD);
    OLED_WR_Byte(0x8D, OLED_CMD);
    OLED_WR_Byte(0x14, OLED_CMD);
    OLED_Clear();
    OLED_WR_Byte(0xAF, OLED_CMD);
}

void OLED_RefreshPages(uint8_t page_start, uint8_t page_end)
{
    if (page_start > 7) page_start = 7;
    if (page_end   > 7) page_end   = 7;

    for (int i = page_start; i <= page_end; i++) {
        OLED_WR_Byte(0xB0 + i, OLED_CMD);
        OLED_WR_Byte(0x00,     OLED_CMD);
        OLED_WR_Byte(0x10,     OLED_CMD);
        OLED_I2C_Start();
        OLED_Send_Byte(0x78); OLED_I2C_WaitAck();
        OLED_Send_Byte(0x40); OLED_I2C_WaitAck();
        for (int n = 0; n < 128; n++) {
            OLED_Send_Byte(OLED_GRAM[n][i]);
            OLED_I2C_WaitAck();
        }
        OLED_I2C_Stop();
    }
}

void OLED_ClearArea(uint8_t x_start, uint8_t x_end,
                    uint8_t page_start, uint8_t page_end)
{
    if (x_end     > 127) x_end     = 127;
    if (page_end  >   7) page_end  = 7;
    for (int x = x_start; x <= x_end; x++)
        for (int p = page_start; p <= page_end; p++)
            OLED_GRAM[x][p] = 0;
}
