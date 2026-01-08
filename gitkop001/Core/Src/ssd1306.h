#ifndef SSD1306_H
#define SSD1306_H

#include <stdint.h>
#include <string.h>

#include "gpio.h"
#include "tim.h"
#include "usart.h"
#include "adc.h"

// --- Config: Dimensions ---
#define SSD1306_WIDTH  128
#define SSD1306_HEIGHT 64

// --- Config: GPIO Pins for Soft I2C ---
// Change these to match your schematic
#define SSD1306_SCL_PORT  GPIOB
#define SSD1306_SCL_PIN   GPIO_PIN_6

#define SSD1306_SDA_PORT  GPIOB
#define SSD1306_SDA_PIN   GPIO_PIN_10

// --- Config: I2C Address ---
// 0x78 is the default for most modules (0x3C << 1)
#define SSD1306_I2C_ADDR  0x78

// Color enumeration
typedef enum {
    Black = 0x00, // Black color, no pixel
    White = 0x01  // Pixel is set. Color depends on OLED
} SSD1306_COLOR;

// Font structure
typedef struct {
    const uint8_t width;
    const uint8_t height;
    const uint16_t *data;
} SSD1306_Font_t;

// Function Prototypes
void SSD1306_Init(void);
void SSD1306_UpdateScreen(void);
void SSD1306_Fill(SSD1306_COLOR color);
void SSD1306_DrawPixel(uint8_t x, uint8_t y, SSD1306_COLOR color);
void SSD1306_WriteChar(char ch, SSD1306_Font_t font, SSD1306_COLOR color);
void SSD1306_WriteString(char* str, SSD1306_Font_t font, SSD1306_COLOR color);
void SSD1306_DrawBitmap(uint8_t x, uint8_t y, const unsigned char* bitmap, uint8_t w, uint8_t h, SSD1306_COLOR color);

// Simple cursor position control for text
void SSD1306_SetCursor(uint8_t x, uint8_t y);

#endif
