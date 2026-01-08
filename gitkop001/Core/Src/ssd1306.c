#include "ssd1306.h"
#include <stdio.h>
#include "ssd1306_fonts.h" // We will define this next

/* Screen Buffer */
static uint8_t SSD1306_Buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];

/* Cursor Position */
typedef struct {
    uint16_t CurrentX;
    uint16_t CurrentY;
} SSD1306_t;

static SSD1306_t SSD1306;

/* =========================================================================
 * LOW LEVEL SOFT I2C IMPLEMENTATION
 * ========================================================================= */

#define SSD1306_SCL_SET()   HAL_GPIO_WritePin(SSD1306_SCL_PORT, SSD1306_SCL_PIN, GPIO_PIN_SET)
#define SSD1306_SCL_CLR()   HAL_GPIO_WritePin(SSD1306_SCL_PORT, SSD1306_SCL_PIN, GPIO_PIN_RESET)
#define SSD1306_SDA_SET()   HAL_GPIO_WritePin(SSD1306_SDA_PORT, SSD1306_SDA_PIN, GPIO_PIN_SET)
#define SSD1306_SDA_CLR()   HAL_GPIO_WritePin(SSD1306_SDA_PORT, SSD1306_SDA_PIN, GPIO_PIN_RESET)

// Simple delay for timing (adjust if CPU is very fast, e.g., >100MHz)
static void SW_I2C_Delay(void) {
    // Empty loop or __NOP() usually suffices for ~100kHz-400kHz on standard MCUs
    // If your display glitches, increase this delay.
    for(volatile int i=0; i<10; i++) { __NOP(); }
}

static void SW_I2C_Start(void) {
    SSD1306_SDA_SET();
    SSD1306_SCL_SET();
    SW_I2C_Delay();
    SSD1306_SDA_CLR();
    SW_I2C_Delay();
    SSD1306_SCL_CLR();
}

static void SW_I2C_Stop(void) {
    SSD1306_SDA_CLR();
    SSD1306_SCL_SET();
    SW_I2C_Delay();
    SSD1306_SDA_SET();
}

static void SW_I2C_WriteByte(uint8_t data) {
    for(uint8_t i = 0; i < 8; i++) {
        if((data & 0x80) != 0) SSD1306_SDA_SET();
        else SSD1306_SDA_CLR();

        SW_I2C_Delay();
        SSD1306_SCL_SET();
        SW_I2C_Delay();
        SSD1306_SCL_CLR();
        data <<= 1;
    }

    // Read ACK (We ignore the actual ACK value to keep it simple/fast for output-only)
    SSD1306_SDA_SET(); // Release SDA
    SW_I2C_Delay();
    SSD1306_SCL_SET();
    SW_I2C_Delay();
    SSD1306_SCL_CLR();
}

static void SSD1306_WriteCommand(uint8_t command) {
    SW_I2C_Start();
    SW_I2C_WriteByte(SSD1306_I2C_ADDR); // Address + Write
    SW_I2C_WriteByte(0x00);             // Control byte: Command
    SW_I2C_WriteByte(command);
    SW_I2C_Stop();
}

static void SSD1306_WriteData(uint8_t* data, uint16_t size) {
    SW_I2C_Start();
    SW_I2C_WriteByte(SSD1306_I2C_ADDR);
    SW_I2C_WriteByte(0x40); // Control byte: Data

    for(uint16_t i = 0; i < size; i++) {
        SW_I2C_WriteByte(data[i]);
    }
    SW_I2C_Stop();
}

/* =========================================================================
 * SSD1306 DRIVER FUNCTIONS
 * ========================================================================= */

void SSD1306_Init(void) {
    // Ensure pins are initialized in main.c (GPIO Output Open-Drain recommended)

    HAL_Delay(100); // Boot delay

    // Init sequence
    SSD1306_WriteCommand(0xAE); // Display Off
    SSD1306_WriteCommand(0x20); // Set Memory Addressing Mode
    SSD1306_WriteCommand(0x10); // 00,Horizontal Addressing Mode; 01,Vertical; 10,Page
    SSD1306_WriteCommand(0xB0); // Set Page Start Address
    SSD1306_WriteCommand(0xC8); // Set COM Output Scan Direction
    SSD1306_WriteCommand(0x00); // Set Low Column Address
    SSD1306_WriteCommand(0x10); // Set High Column Address
    SSD1306_WriteCommand(0x40); // Set Start Line Address
    SSD1306_WriteCommand(0x81); // Set Contrast Control
    SSD1306_WriteCommand(0xFF);
    SSD1306_WriteCommand(0xA1); // Set Segment Re-map
    SSD1306_WriteCommand(0xA6); // Set Normal/Inverse Display
    SSD1306_WriteCommand(0xA8); // Set Multiplex Ratio
    SSD1306_WriteCommand(0x3F);
    SSD1306_WriteCommand(0xA4); // Entire Display GDDRAM/On
    SSD1306_WriteCommand(0xD3); // Set Display Offset
    SSD1306_WriteCommand(0x00);
    SSD1306_WriteCommand(0xD5); // Set Display Clock Divide Ratio
    SSD1306_WriteCommand(0xF0);
    SSD1306_WriteCommand(0xD9); // Set Pre-charge Period
    SSD1306_WriteCommand(0x22);
    SSD1306_WriteCommand(0xDA); // Set COM Pins Hardware Configuration
    SSD1306_WriteCommand(0x12);
    SSD1306_WriteCommand(0xDB); // Set VCOMH Deselect Level
    SSD1306_WriteCommand(0x20);
    SSD1306_WriteCommand(0x8D); // Charge Pump Setting
    SSD1306_WriteCommand(0x14);
    SSD1306_WriteCommand(0xAF); // Display On

    SSD1306_Fill(Black);
    SSD1306_UpdateScreen();
    SSD1306_Fill(Black);
    SSD1306_UpdateScreen();

}
void SSD1306_Fill(SSD1306_COLOR color) {
    memset(SSD1306_Buffer, (color == Black) ? 0x00 : 0xFF, sizeof(SSD1306_Buffer));
}

void SSD1306_UpdateScreen(void) {
    for (uint8_t i = 0; i < 8; i++) {
        SSD1306_WriteCommand(0xB0 + i); // Set Page Start Address
        SSD1306_WriteCommand(0x00);     // Set Low Column Start Address
        SSD1306_WriteCommand(0x10);     // Set High Column Start Address

        SSD1306_WriteData(&SSD1306_Buffer[SSD1306_WIDTH * i], SSD1306_WIDTH);
    }
}

void SSD1306_DrawPixel(uint8_t x, uint8_t y, SSD1306_COLOR color) {
    if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) return;

    x = SSD1306_WIDTH - 1 - x;
    y = SSD1306_HEIGHT - 1 - y;

    if (color == White) {
        SSD1306_Buffer[x + (y / 8) * SSD1306_WIDTH] |= (1 << (y % 8));
    } else {
        SSD1306_Buffer[x + (y / 8) * SSD1306_WIDTH] &= ~(1 << (y % 8));
    }
}

void SSD1306_DrawBitmap(uint8_t x, uint8_t y, const unsigned char* bitmap, uint8_t w, uint8_t h, SSD1306_COLOR color) {
    int16_t byteWidth = (w + 7) / 8; // Bitmap scanline width in bytes
    uint8_t byte = 0;

    for(int16_t j=0; j<h; j++, y++) {
        for(int16_t i=0; i<w; i++) {
            if(i & 7) byte <<= 1;
            else      byte = bitmap[j * byteWidth + i / 8];

            if(byte & 0x80) SSD1306_DrawPixel(x+i, y, color);
        }
    }
}

void SSD1306_SetCursor(uint8_t x, uint8_t y) {
    SSD1306.CurrentX = x;
    SSD1306.CurrentY = y;
}

void SSD1306_WriteChar(char ch, SSD1306_Font_t font, SSD1306_COLOR color) {
    uint32_t b;

    if (ch == '\n')
    {
        SSD1306.CurrentY += font.height;
        return;
    }

    // Check remaining space on current line
    if (SSD1306_WIDTH <= (SSD1306.CurrentX + font.width) ||
        SSD1306_HEIGHT <= (SSD1306.CurrentY + font.height))
    {
        return;
    }


    // Use font data to draw pixels
    for (int i = 0; i < font.height; i++) {
        b = font.data[(ch - 32) * font.height + i];
        for (int j = 0; j < font.width; j++) {
            if ((b << j) & 0x8000) {
                SSD1306_DrawPixel(SSD1306.CurrentX + j, (SSD1306.CurrentY + i), (SSD1306_COLOR) color);
            } else {
                SSD1306_DrawPixel(SSD1306.CurrentX + j, (SSD1306.CurrentY + i), (SSD1306_COLOR)!color);
            }
        }
    }

    SSD1306.CurrentX += font.width;
}

void SSD1306_WriteString(char* str, SSD1306_Font_t font, SSD1306_COLOR color) {
    while (*str) {
        SSD1306_WriteChar(*str, font, color);
        str++;
    }
}
