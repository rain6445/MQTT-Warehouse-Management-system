#ifndef __OLED_H
#define __OLED_H

#include <stdint.h>

/* ---- 根据实际接线修改这两个引脚 ---- */
#define OLED_SCL_PIN   22
#define OLED_SDA_PIN   21
/* ----------------------------------- */

#define OLED_CMD  0
#define OLED_DATA 1

void OLED_ColorTurn(uint8_t i);
void OLED_DisplayTurn(uint8_t i);
void OLED_I2C_Start(void);
void OLED_I2C_Stop(void);
void OLED_I2C_WaitAck(void);
void OLED_Send_Byte(uint8_t dat);
void OLED_WR_Byte(uint8_t dat, uint8_t mode);
void OLED_DisPlay_On(void);
void OLED_DisPlay_Off(void);
void OLED_Refresh(void);
void OLED_Clear(void);
void OLED_DrawPoint(uint8_t x, uint8_t y, uint8_t t);
void OLED_DrawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t mode);
void OLED_DrawCircle(uint8_t x, uint8_t y, uint8_t r);
void OLED_ShowChar(uint8_t x, uint8_t y, uint8_t chr, uint8_t size1, uint8_t mode);
void OLED_ShowString(uint8_t x, uint8_t y, const char *chr, uint8_t size1, uint8_t mode);
void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size1, uint8_t mode);
void OLED_ShowChinese(uint8_t x, uint8_t y, uint8_t num, uint8_t size1, uint8_t mode);
void OLED_ShowPicture(uint8_t x, uint8_t y, uint8_t sizex, uint8_t sizey,
                      const uint8_t BMP[], uint8_t mode);
                      /* 只刷新指定页范围（page = y/8，例如 y=16 对应 page=2） */
void OLED_RefreshPages(uint8_t start_page, uint8_t end_page);

/* 只刷新指定页（page 0~7，每页 8px 高） */
void OLED_RefreshPages(uint8_t page_start, uint8_t page_end);
/* 只清除 GRAM 中指定矩形区域（不触发刷新） */
void OLED_ClearArea(uint8_t x_start, uint8_t x_end,
                    uint8_t page_start, uint8_t page_end);



void OLED_Init(void);

#endif /* __OLED_H */
