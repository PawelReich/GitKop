#include "ili9341/ili9341.h"

#include "stm32f4xx_hal.h"

/**
 * @brief Select the ILI9341 display
 * @param ili9341 Pointer to ILI9341 handle structure
 */
static void ILI9341_Select(const ILI9341_HandleTypeDef* ili9341) {
    HAL_GPIO_WritePin(ili9341->cs_port, ili9341->cs_pin, GPIO_PIN_RESET);
}

void ILI9341_Deselect(const ILI9341_HandleTypeDef* ili9341) {
    HAL_GPIO_WritePin(ili9341->cs_port, ili9341->cs_pin, GPIO_PIN_SET);
}

/**
 * @brief Reset the ILI9341 display
 * @param ili9341 Pointer to ILI9341 handle structure
 */
static void ILI9341_Reset(const ILI9341_HandleTypeDef* ili9341) {
    HAL_GPIO_WritePin(ili9341->rst_port, ili9341->rst_pin, GPIO_PIN_RESET);
    HAL_Delay(5);
    HAL_GPIO_WritePin(ili9341->rst_port, ili9341->rst_pin, GPIO_PIN_SET);
}

/**
 * @brief Write a command to the ILI9341 display
 * @param ili9341 Pointer to ILI9341 handle structure
 * @param cmd Command byte to write
 */
static void ILI9341_WriteCommand(const ILI9341_HandleTypeDef* ili9341, uint8_t cmd) {
    HAL_GPIO_WritePin(ili9341->dc_port, ili9341->dc_pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(ili9341->spi_handle, &cmd, sizeof(cmd), HAL_MAX_DELAY);
}

/**
 * @brief Write data to the ILI9341 display
 * @param ili9341 Pointer to ILI9341 handle structure
 * @param buff Pointer to the data buffer
 * @param bufferSize Size of the data buffer
 */
static void ILI9341_WriteData(const ILI9341_HandleTypeDef* ili9341, uint8_t* buff, size_t bufferSize) {
    HAL_GPIO_WritePin(ili9341->dc_port, ili9341->dc_pin, GPIO_PIN_SET);

    // split data in small chunks because HAL can't send more then 64K at once
    while (bufferSize > 0) {
        uint16_t chunkSize = bufferSize > 32768 ? 32768 : bufferSize;
        HAL_SPI_Transmit(ili9341->spi_handle, buff, chunkSize, HAL_MAX_DELAY);
        buff += chunkSize;
        bufferSize -= chunkSize;
    }
}

ILI9341_HandleTypeDef ILI9341_Init(
    SPI_HandleTypeDef* spi_handle,
    GPIO_TypeDef* cs_port,
    uint16_t cs_pin,
    GPIO_TypeDef* dc_port,
    uint16_t dc_pin,
    GPIO_TypeDef* rst_port,
    uint16_t rst_pin,
    int_fast8_t rotation,
    int_fast16_t width,
    int_fast16_t height
) {
    width = abs(width);
    height = abs(height);

    const ILI9341_HandleTypeDef ili9341_instance = {
        .spi_handle = spi_handle,
        .cs_port = cs_port,
        .cs_pin = cs_pin,
        .dc_port = dc_port,
        .dc_pin = dc_pin,
        .rst_port = rst_port,
        .rst_pin = rst_pin,
        .rotation = rotation,
        .width = width,
        .height = height
    };

    const ILI9341_HandleTypeDef* ili9341 = &ili9341_instance;

    ILI9341_Select(ili9341);
    ILI9341_Reset(ili9341);

    // Initialization command list is based on https://github.com/martnak/STM32-ILI9341 which have the following license

    /*
        MIT License

        Copyright (c) 2017 martnak

        Permission is hereby granted, free of charge, to any person obtaining a copy
        of this software and associated documentation files (the "Software"), to deal
        in the Software without restriction, including without limitation the rights
        to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
        copies of the Software, and to permit persons to whom the Software is
        furnished to do so, subject to the following conditions:

        The above copyright notice and this permission notice shall be included in all
        copies or substantial portions of the Software.

        THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
        IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
        FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
        AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
        LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
        OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
        SOFTWARE.
    */

    // SOFTWARE RESET
    ILI9341_WriteCommand(ili9341, 0x01);
    HAL_Delay(1000);

    // POWER CONTROL A
    ILI9341_WriteCommand(ili9341, 0xCB);
    {
        uint8_t data[] = {0x39, 0x2C, 0x00, 0x34, 0x02};
        ILI9341_WriteData(ili9341, data, sizeof(data));
    }

    // POWER CONTROL B
    ILI9341_WriteCommand(ili9341, 0xCF);
    {
        uint8_t data[] = {0x00, 0xC1, 0x30};
        ILI9341_WriteData(ili9341, data, sizeof(data));
    }

    // DRIVER TIMING CONTROL A
    ILI9341_WriteCommand(ili9341, 0xE8);
    {
        uint8_t data[] = {0x85, 0x00, 0x78};
        ILI9341_WriteData(ili9341, data, sizeof(data));
    }

    // DRIVER TIMING CONTROL B
    ILI9341_WriteCommand(ili9341, 0xEA);
    {
        uint8_t data[] = {0x00, 0x00};
        ILI9341_WriteData(ili9341, data, sizeof(data));
    }

    // POWER ON SEQUENCE CONTROL
    ILI9341_WriteCommand(ili9341, 0xED);
    {
        uint8_t data[] = {0x64, 0x03, 0x12, 0x81};
        ILI9341_WriteData(ili9341, data, sizeof(data));
    }

    // PUMP RATIO CONTROL
    ILI9341_WriteCommand(ili9341, 0xF7);
    {
        uint8_t data[] = {0x20};
        ILI9341_WriteData(ili9341, data, sizeof(data));
    }

    // POWER CONTROL,VRH[5:0]
    ILI9341_WriteCommand(ili9341, 0xC0);
    {
        uint8_t data[] = {0x23};
        ILI9341_WriteData(ili9341, data, sizeof(data));
    }

    // POWER CONTROL,SAP[2:0];BT[3:0]
    ILI9341_WriteCommand(ili9341, 0xC1);
    {
        uint8_t data[] = {0x10};
        ILI9341_WriteData(ili9341, data, sizeof(data));
    }

    // VCM CONTROL
    ILI9341_WriteCommand(ili9341, 0xC5);
    {
        uint8_t data[] = {0x3E, 0x28};
        ILI9341_WriteData(ili9341, data, sizeof(data));
    }

    // VCM CONTROL 2
    ILI9341_WriteCommand(ili9341, 0xC7);
    {
        uint8_t data[] = {0x86};
        ILI9341_WriteData(ili9341, data, sizeof(data));
    }

    // MEMORY ACCESS CONTROL
    ILI9341_WriteCommand(ili9341, 0x36);
    {
        uint8_t data[] = {0x48};
        ILI9341_WriteData(ili9341, data, sizeof(data));
    }

    // PIXEL FORMAT
    ILI9341_WriteCommand(ili9341, 0x3A);
    {
        uint8_t data[] = {0x55};
        ILI9341_WriteData(ili9341, data, sizeof(data));
    }

    // FRAME RATIO CONTROL, STANDARD RGB COLOR
    ILI9341_WriteCommand(ili9341, 0xB1);
    {
        uint8_t data[] = {0x00, 0x18};
        ILI9341_WriteData(ili9341, data, sizeof(data));
    }

    // DISPLAY FUNCTION CONTROL
    ILI9341_WriteCommand(ili9341, 0xB6);
    {
        uint8_t data[] = {0x08, 0x82, 0x27};
        ILI9341_WriteData(ili9341, data, sizeof(data));
    }

    // 3GAMMA FUNCTION DISABLE
    ILI9341_WriteCommand(ili9341, 0xF2);
    {
        uint8_t data[] = {0x00};
        ILI9341_WriteData(ili9341, data, sizeof(data));
    }

    // GAMMA CURVE SELECTED
    ILI9341_WriteCommand(ili9341, 0x26);
    {
        uint8_t data[] = {0x01};
        ILI9341_WriteData(ili9341, data, sizeof(data));
    }

    // POSITIVE GAMMA CORRECTION
    ILI9341_WriteCommand(ili9341, 0xE0);
    {
        uint8_t data[] = {0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00};
        ILI9341_WriteData(ili9341, data, sizeof(data));
    }

    // NEGATIVE GAMMA CORRECTION
    ILI9341_WriteCommand(ili9341, 0xE1);
    {
        uint8_t data[] = {0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F};
        ILI9341_WriteData(ili9341, data, sizeof(data));
    }

    // EXIT SLEEP
    ILI9341_WriteCommand(ili9341, 0x11);
    HAL_Delay(120);

    // TURN ON DISPLAY
    ILI9341_WriteCommand(ili9341, 0x29);

    // MADCTL
    ILI9341_WriteCommand(ili9341, 0x36);
    switch (ili9341->rotation) {
        case ILI9341_ROTATION_VERTICAL_1: {
            uint8_t rotation = ILI9341_MADCTL_MX | ILI9341_MADCTL_BGR;
            uint8_t data[] = {rotation};
            ILI9341_WriteData(ili9341, data, sizeof(data));
            break;
        }
        case ILI9341_ROTATION_HORIZONTAL_1: {
            uint8_t rotation = ILI9341_MADCTL_MX | ILI9341_MADCTL_MY | ILI9341_MADCTL_MV | ILI9341_MADCTL_BGR;
            uint8_t data[] = {rotation};
            ILI9341_WriteData(ili9341, data, sizeof(data));
            break;
        }
        case ILI9341_ROTATION_HORIZONTAL_2: {
            uint8_t rotation = ILI9341_MADCTL_MV | ILI9341_MADCTL_BGR;
            uint8_t data[] = {rotation};
            ILI9341_WriteData(ili9341, data, sizeof(data));
            break;
        }
        case ILI9341_ROTATION_VERTICAL_2: {
            uint8_t rotation = ILI9341_MADCTL_MY | ILI9341_MADCTL_BGR;
            uint8_t data[] = {rotation};
            ILI9341_WriteData(ili9341, data, sizeof(data));
            break;
        }

        default:
            break;
    }

    ILI9341_Deselect(ili9341);

    return ili9341_instance;
}

void ILI9341_SetOrientation(ILI9341_HandleTypeDef* ili9341, int_fast8_t rotation) {
    ILI9341_Select(ili9341);

    // MADCTL
    ILI9341_WriteCommand(ili9341, 0x36);
    switch (rotation) {
        case ILI9341_ROTATION_VERTICAL_1: {
            uint8_t rotation = ILI9341_MADCTL_MX | ILI9341_MADCTL_BGR;
            uint8_t data[] = {rotation};
            ILI9341_WriteData(ili9341, data, sizeof(data));
            break;
        }
        case ILI9341_ROTATION_HORIZONTAL_1: {
            uint8_t rotation = ILI9341_MADCTL_MX | ILI9341_MADCTL_MY | ILI9341_MADCTL_MV | ILI9341_MADCTL_BGR;
            uint8_t data[] = {rotation};
            ILI9341_WriteData(ili9341, data, sizeof(data));
            break;
        }
        case ILI9341_ROTATION_HORIZONTAL_2: {
            uint8_t rotation = ILI9341_MADCTL_MV | ILI9341_MADCTL_BGR;
            uint8_t data[] = {rotation};
            ILI9341_WriteData(ili9341, data, sizeof(data));
            break;
        }
        case ILI9341_ROTATION_VERTICAL_2: {
            uint8_t rotation = ILI9341_MADCTL_MY | ILI9341_MADCTL_BGR;
            uint8_t data[] = {rotation};
            ILI9341_WriteData(ili9341, data, sizeof(data));
            break;
        }

        default:
            break;
    }

    if ((ili9341->rotation == ILI9341_ROTATION_HORIZONTAL_1 || ili9341->rotation == ILI9341_ROTATION_HORIZONTAL_2) &&
        (rotation == ILI9341_ROTATION_VERTICAL_1 || rotation == ILI9341_ROTATION_VERTICAL_2)) {
        int_fast16_t temp = ili9341->width;
        ili9341->width = ili9341->height;
        ili9341->height = temp;
    } else if ((ili9341->rotation == ILI9341_ROTATION_VERTICAL_1 || ili9341->rotation == ILI9341_ROTATION_VERTICAL_2) &&
               (rotation == ILI9341_ROTATION_HORIZONTAL_1 || rotation == ILI9341_ROTATION_HORIZONTAL_2)) {
        int_fast16_t temp = ili9341->width;
        ili9341->width = ili9341->height;
        ili9341->height = temp;
    }

    ili9341->rotation = rotation;

    ILI9341_Deselect(ili9341);
}

void ILI9341_SetBrightness(const ILI9341_HandleTypeDef* ili9341, uint_fast8_t brightness) {
    if (brightness > 0xFF) brightness = 0xFF;

    ILI9341_Select(ili9341);

    ILI9341_WriteCommand(ili9341, 0x51);
    {
        uint8_t data[] = {brightness & 0xFF};
        ILI9341_WriteData(ili9341, data, sizeof(data));
    }

    ILI9341_Deselect(ili9341);
}

void ILI9341_InvertColors(const ILI9341_HandleTypeDef* ili9341, bool invert) {
    ILI9341_Select(ili9341);
    ILI9341_WriteCommand(ili9341, invert ? 0x21 /* INVON */ : 0x20 /* INVOFF */);
    ILI9341_Deselect(ili9341);
}

/**
 * @brief Set the address window for subsequent pixel data
 * @param ili9341 Pointer to ILI9341 handle structure
 * @param x0 X coordinate of the top-left corner of the window
 * @param y0 Y coordinate of the top-left corner of the window
 * @param x1 X coordinate of the bottom-right corner of the window
 * @param y1 Y coordinate of the bottom-right corner of the window
 */
static void ILI9341_SetAddressWindow(
    const ILI9341_HandleTypeDef* ili9341,
    uint16_t x0,
    uint16_t y0,
    uint16_t x1,
    uint16_t y1
) {
    // column address set
    ILI9341_WriteCommand(ili9341, 0x2A);  // CASET
    {
        uint8_t data[] = {(x0 >> 8) & 0xFF, x0 & 0xFF, (x1 >> 8) & 0xFF, x1 & 0xFF};
        ILI9341_WriteData(ili9341, data, sizeof(data));
    }

    // row address set
    ILI9341_WriteCommand(ili9341, 0x2B);  // RASET
    {
        uint8_t data[] = {(y0 >> 8) & 0xFF, y0 & 0xFF, (y1 >> 8) & 0xFF, y1 & 0xFF};
        ILI9341_WriteData(ili9341, data, sizeof(data));
    }

    // write to RAM
    ILI9341_WriteCommand(ili9341, 0x2C);  // RAMWR
}

/**
 * @brief Draw a pixel at specified coordinates without selecting/deselecting the display
 * @param ili9341 Pointer to ILI9341 handle structure
 * @param x X coordinate of the pixel
 * @param y Y coordinate of the pixel
 * @param color 16-bit pixel color in RGB565 format
 */
static void ILI9341_DrawPixelFast(
    const ILI9341_HandleTypeDef* ili9341,
    int_fast16_t x,
    int_fast16_t y,
    uint16_t color
) {
    if (x < 0 || y < 0 || x >= ili9341->width || y >= ili9341->height) return;

    ILI9341_SetAddressWindow(ili9341, x, y, x + 1, y + 1);
    uint8_t data[] = {color >> 8, color & 0xFF};
    ILI9341_WriteData(ili9341, data, sizeof(data));
}

void ILI9341_DrawPixel(const ILI9341_HandleTypeDef* ili9341, int_fast16_t x, int_fast16_t y, uint16_t color) {
    ILI9341_Select(ili9341);
    ILI9341_DrawPixelFast(ili9341, x, y, color);
    ILI9341_Deselect(ili9341);
}

/**
 * @brief Fill a rectangle without selecting/deselecting the display
 * @param ili9341 Pointer to ILI9341 handle structure
 * @param x X coordinate of the top-left corner of the rectangle
 * @param y Y coordinate of the top-left corner of the rectangle
 * @param w Width of the rectangle in pixels, can be negative
 * @param h Height of the rectangle in pixels, can be negative
 * @param color 16-bit fill color in RGB565 format
 */
static void ILI9341_FillRectangleFast(
    const ILI9341_HandleTypeDef* ili9341,
    int_fast16_t x,
    int_fast16_t y,
    int_fast16_t w,
    int_fast16_t h,
    uint16_t color
) {
    if (w < 0) {
        w = -w;
        x -= w - 1;
    }

    if (h < 0) {
        h = -h;
        y -= h - 1;
    }

    if (x >= ili9341->width || y >= ili9341->height || w == 0 || h == 0) return;

    if (x < 0) {
        w += x;
        x = 0;
        if (w <= 0) return;
    }

    if (y < 0) {
        h += y;
        y = 0;
        if (h <= 0) return;
    }

    if ((x + w - 1) >= ili9341->width) w = ili9341->width - x;
    if ((y + h - 1) >= ili9341->height) h = ili9341->height - y;

    uint8_t buffer[ILI9341_FILL_RECT_BUFFER_SIZE * 2];
    size_t totalSize = w * h;
    size_t chunkSize = totalSize > ILI9341_FILL_RECT_BUFFER_SIZE ? ILI9341_FILL_RECT_BUFFER_SIZE : totalSize;

    color = (color >> 8) | (color << 8);
    for (size_t i = 0; i < chunkSize; i++) { *(uint16_t*)&buffer[i * 2] = color; }

    ILI9341_SetAddressWindow(ili9341, x, y, x + w - 1, y + h - 1);

    while (totalSize > 0) {
        ILI9341_WriteData(ili9341, buffer, chunkSize * 2);
        totalSize -= chunkSize;
        chunkSize = (totalSize > ILI9341_FILL_RECT_BUFFER_SIZE) ? ILI9341_FILL_RECT_BUFFER_SIZE : totalSize;
    }
}

void ILI9341_FillRectangle(
    const ILI9341_HandleTypeDef* ili9341,
    int_fast16_t x,
    int_fast16_t y,
    int_fast16_t w,
    int_fast16_t h,
    uint16_t color
) {
    ILI9341_Select(ili9341);
    ILI9341_FillRectangleFast(ili9341, x, y, w, h, color);
    ILI9341_Deselect(ili9341);
}

void ILI9341_FillScreen(const ILI9341_HandleTypeDef* ili9341, uint16_t color) {
    ILI9341_Select(ili9341);
    ILI9341_FillRectangleFast(ili9341, 0, 0, ili9341->width, ili9341->height, color);
    ILI9341_Deselect(ili9341);
}

/**
 * @brief Draw a glyph at specified coordinates without selecting/deselecting the display
 * @param ili9341 Pointer to ILI9341 handle structure
 * @param x X coordinate of the left of the character
 * @param y Y coordinate of the baseline of the character
 * @param glyph Glyph to render
 * @param color 16-bit character color in RGB565 format
 * @param bgColor 16-bit background color in RGB565 format
 * @param scale Scaling factor (integer) to enlarge the character
 */
static void ILI9341_DrawGlyphFast(
    const ILI9341_HandleTypeDef* ili9341,
    int_fast16_t x,
    int_fast16_t y,
    ILI9341_GlyphDef glyph,
    uint16_t color,
    uint16_t bgColor,
    int_fast16_t scale
) {
    int_fast16_t startX = x + glyph.bbX * scale;
    int_fast16_t startY = y - glyph.bbY * scale - glyph.bbH * scale + 1;
    int_fast16_t endX = startX + glyph.bbW * scale - 1;
    int_fast16_t endY = startY + glyph.bbH * scale - 1;

    if (endX < 0 || endY < 0 || startX >= ili9341->width || startY >= ili9341->height ||
        (glyph.bbW == 0 || glyph.bbH == 0))
        return;

    int_fast16_t clipStartX = startX < 0 ? -startX : 0;
    int_fast16_t clipStartY = startY < 0 ? -startY : 0;
    int_fast16_t clipEndX = endX >= ili9341->width ? ili9341->width - startX - 1 : glyph.bbW * scale - 1;
    int_fast16_t clipEndY = endY >= ili9341->height ? ili9341->height - startY - 1 : glyph.bbH * scale - 1;

    color = (color >> 8) | (color << 8);
    bgColor = (bgColor >> 8) | (bgColor << 8);

    uint16_t buffer[ILI9341_DRAW_GLYPH_BUFFER_SIZE];
    size_t bufferIndex = 0;

    int_fast32_t bitIndex = 0;

    ILI9341_SetAddressWindow(ili9341, startX + clipStartX, startY + clipStartY, startX + clipEndX, startY + clipEndY);

    for (int_fast16_t row = 0; row < glyph.bbH; row++) {
        for (int_fast16_t vScale = 0; vScale < scale; vScale++) {
            int_fast16_t pixelRow = row * scale + vScale;
            for (int_fast16_t col = 0; col < glyph.bbW; col++) {
                uint8_t mask = 0x80 >> (bitIndex % 8);
                int_fast16_t index = bitIndex / 8;
                for (int_fast16_t hScale = 0; hScale < scale; hScale++) {
                    int_fast16_t pixelCol = col * scale + hScale;
                    if ((pixelRow >= clipStartY && pixelRow <= clipEndY) &&
                        (pixelCol >= clipStartX && pixelCol <= clipEndX)) {
                        if (glyph.data[index] & mask) {
                            buffer[bufferIndex++] = color;
                        } else {
                            buffer[bufferIndex++] = bgColor;
                        }

                        if (bufferIndex >= ILI9341_DRAW_GLYPH_BUFFER_SIZE) {
                            ILI9341_WriteData(ili9341, (uint8_t*)buffer, bufferIndex * 2);
                            bufferIndex = 0;
                        }
                    }
                }
                bitIndex++;
            }
            bitIndex -= glyph.bbW;
        }
        bitIndex += glyph.bbW;
    }

    if (bufferIndex > 0) ILI9341_WriteData(ili9341, (uint8_t*)buffer, bufferIndex * 2);
}

void ILI9341_WriteString(
    const ILI9341_HandleTypeDef* ili9341,
    int_fast16_t x,
    int_fast16_t y,
    const char* str,
    ILI9341_FontDef font,
    uint16_t color,
    uint16_t bgColor,
    bool wrap,
    int_fast16_t scale,
    int_fast16_t tracking,
    int_fast16_t leading
) {
    if (scale < 1 || y + font.descent * scale < 0 || y - font.ascent * scale >= ili9341->height) return;

    int_fast16_t originalX = x;

    ILI9341_Select(ili9341);

    for (unsigned char c; (c = *(str++));) {
        if (c == '\r') {
            x = originalX;
            continue;
        }

        if (c == '\n') {
            y += (font.ascent + font.descent) * scale + leading;
            x = originalX;
            if (y - font.ascent * scale >= ili9341->height) break;
            continue;
        }

        if (c < font.startCodepoint || c > font.endCodepoint) { c = FALLBACK_CODEPOINT; }
        ILI9341_GlyphDef glyph = font.glyphs[c - font.startCodepoint];

        // Only wrap if current char is not zero-width, help prevent newline on diacritics
        if (wrap && glyph.advance > 0 && x + (glyph.bbX + glyph.bbW) * scale + 1 >= ili9341->width) {
            y += (font.ascent + font.descent) * scale + leading;
            x = originalX;
            if (y - font.ascent * scale >= ili9341->height) break;
            if (c == 0x20 || c == 0xA0) {  // Ignore space and nbsp after newline
                continue;
            }
        }

        ILI9341_DrawGlyphFast(ili9341, x, y, glyph, color, bgColor, scale);
        x += glyph.advance * scale;

        c = *str;
        if (c < font.startCodepoint || c > font.endCodepoint) { c = FALLBACK_CODEPOINT; }

        // Only apply tracking if next char is not zero-width, help diacritics stay aligned
        if (tracking) {
            ILI9341_GlyphDef glyph = font.glyphs[c - font.startCodepoint];
            x += glyph.advance > 0 ? tracking : 0;
        }
    }

    ILI9341_Deselect(ili9341);
}

/**
 * @brief Write a scaled character with transparent background at specified coordinates without selecting/deselecting
 * the display
 * @param ili9341 Pointer to ILI9341 handle structure
 * @param x X coordinate of the left of the character
 * @param y Y coordinate of the baseline of the character
 * @param ch ASCII character to write
 * @param font Font definition to use for rendering the character
 * @param color 16-bit character color in RGB565 format
 * @param scale Scaling factor (integer) to enlarge the character
 */
static void ILI9341_DrawGlyphTransparentFast(
    const ILI9341_HandleTypeDef* ili9341,
    int_fast16_t x,
    int_fast16_t y,
    ILI9341_GlyphDef glyph,
    uint16_t color,
    int_fast16_t scale
) {
    int_fast16_t startX = x + glyph.bbX * scale;
    int_fast16_t startY = y - glyph.bbY * scale - glyph.bbH * scale + 1;
    int_fast16_t endX = startX + glyph.bbW * scale - 1;
    int_fast16_t endY = startY + glyph.bbH * scale - 1;

    if (endX < 0 || endY < 0 || startX >= ili9341->width || startY >= ili9341->height ||
        (glyph.bbW == 0 || glyph.bbH == 0))
        return;

    int_fast16_t index = 0;
    uint8_t mask = 0x80;

    for (int_fast16_t row = 0; row < glyph.bbH; row++) {
        for (int_fast16_t col = 0; col < glyph.bbW; col++) {
            if (glyph.data[index] & mask) {
                ILI9341_FillRectangleFast(ili9341, startX + col * scale, startY + row * scale, scale, scale, color);
            }
            mask >>= 1;
            if (mask == 0) {
                index++;
                mask = 0x80;
            }
        }
    }
}

void ILI9341_WriteStringTransparent(
    const ILI9341_HandleTypeDef* ili9341,
    int_fast16_t x,
    int_fast16_t y,
    const char* str,
    ILI9341_FontDef font,
    uint16_t color,
    bool wrap,
    int_fast16_t scale,
    int_fast16_t tracking,
    int_fast16_t leading
) {
    if (scale < 1 || y + font.descent * scale < 0 || y - font.ascent * scale >= ili9341->height) return;

    int_fast16_t originalX = x;

    ILI9341_Select(ili9341);

    for (unsigned char c; (c = *(str++));) {
        if (c == '\r') {
            x = originalX;
            continue;
        }

        if (c == '\n') {
            y += (font.ascent + font.descent) * scale + leading;
            x = originalX;
            if (y - font.ascent * scale >= ili9341->height) break;
            continue;
        }

        if (c < font.startCodepoint || c > font.endCodepoint) { c = FALLBACK_CODEPOINT; }
        ILI9341_GlyphDef glyph = font.glyphs[c - font.startCodepoint];

        // Only wrap if current char is not zero-width, help prevent newline on diacritics
        if (wrap && glyph.advance > 0 && x + (glyph.bbX + glyph.bbW) * scale + 1 >= ili9341->width) {
            y += (font.ascent + font.descent) * scale + leading;
            x = originalX;
            if (y - font.ascent * scale >= ili9341->height) break;
            if (c == 0x20 || c == 0xA0) {  // Ignore space and nbsp after newline
                continue;
            }
        }

        ILI9341_DrawGlyphTransparentFast(ili9341, x, y, glyph, color, scale);
        x += glyph.advance * scale;

        c = *str;
        if (c < font.startCodepoint || c > font.endCodepoint) { c = FALLBACK_CODEPOINT; }

        // Only apply tracking if next char is not zero-width, help diacritics stay aligned
        if (tracking) {
            ILI9341_GlyphDef glyph = font.glyphs[c - font.startCodepoint];
            x += glyph.advance > 0 ? tracking : 0;
        }
    }

    ILI9341_Deselect(ili9341);
}

void ILI9341_DrawImage(
    const ILI9341_HandleTypeDef* ili9341,
    int_fast16_t x,
    int_fast16_t y,
    int_fast16_t w,
    int_fast16_t h,
    const uint16_t* data
) {
    if (w == 0 || h == 0) return;
    if (w < 0) {
        w = -w;
        x -= w - 1;
    }
    if (h < 0) {
        h = -h;
        y -= h - 1;
    }
    if (x < 0 || y < 0 || x >= ili9341->width || y >= ili9341->height || (x + w - 1) >= ili9341->width ||
        (y + h - 1) >= ili9341->height)
        return;

    ILI9341_Select(ili9341);
    ILI9341_SetAddressWindow(ili9341, x, y, x + w - 1, y + h - 1);
    ILI9341_WriteData(ili9341, (uint8_t*)data, sizeof(uint16_t) * w * h);
    ILI9341_Deselect(ili9341);
}

/**
 * @brief Draw a line using Bresenham's algorithm without selecting/deselecting the display
 * @param ili9341 Pointer to ILI9341 handle structure
 * @param x1 X coordinate of the start point
 * @param y1 Y coordinate of the start point
 * @param x2 X coordinate of the end point
 * @param y2 Y coordinate of the end point
 * @param color 16-bit line color in RGB565 format
 */
static void ILI9341_DrawLineFast(
    const ILI9341_HandleTypeDef* ili9341,
    int_fast16_t x1,
    int_fast16_t y1,
    int_fast16_t x2,
    int_fast16_t y2,
    uint16_t color
) {
    if (x1 == x2) {
        ILI9341_FillRectangleFast(ili9341, x1, y1, 1, y2 - y1 + 1, color);
        return;
    } else if (y1 == y2) {
        ILI9341_FillRectangleFast(ili9341, x1, y1, x2 - x1 + 1, 1, color);
        return;
    }

    int_fast16_t dx = abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
    int_fast16_t dy = -abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
    int_fast16_t err = dx + dy, e2; /* error value e_xy */

    while (true) {
        ILI9341_DrawPixelFast(ili9341, x1, y1, color);
        if (x1 == x2 && y1 == y2) break;
        e2 = 2 * err;
        if (e2 >= dy) { /* e_xy+e_x > 0 */
            err += dy;
            x1 += sx;
        }
        if (e2 <= dx) { /* e_xy+e_y < 0 */
            err += dx;
            y1 += sy;
        }
    }
}

void ILI9341_DrawLine(
    const ILI9341_HandleTypeDef* ili9341,
    int_fast16_t x1,
    int_fast16_t y1,
    int_fast16_t x2,
    int_fast16_t y2,
    uint16_t color
) {
    ILI9341_Select(ili9341);
    ILI9341_DrawLineFast(ili9341, x1, y1, x2, y2, color);
    ILI9341_Deselect(ili9341);
}

void ILI9341_DrawLineThick(
    const ILI9341_HandleTypeDef* ili9341,
    int_fast16_t x1,
    int_fast16_t y1,
    int_fast16_t x2,
    int_fast16_t y2,
    uint16_t color,
    int_fast16_t thickness,
    bool cap
) {
    if (thickness <= 0) return;

    // calculate line vector
    int_fast16_t dx = x2 - x1;
    int_fast16_t dy = y2 - y1;
    float length = sqrtf(dx * dx + dy * dy);
    if (length == 0) return;  // zero-length line

    // unit vector
    float ux = dx / length;
    float uy = dy / length;

    // perpendicular vector
    float px = -uy;
    float py = ux;

    // half thickness
    float halfThickness = thickness / 2.0f;

    // calculate corner points
    int16_t xCorners[4] = {
        (int16_t)(x1 + px * halfThickness),
        (int16_t)(x1 - px * halfThickness),
        (int16_t)(x2 - px * halfThickness),
        (int16_t)(x2 + px * halfThickness)
    };
    int16_t yCorners[4] = {
        (int16_t)(y1 + py * halfThickness),
        (int16_t)(y1 - py * halfThickness),
        (int16_t)(y2 - py * halfThickness),
        (int16_t)(y2 + py * halfThickness)
    };

    ILI9341_FillPolygon(ili9341, xCorners, yCorners, 4, color);

    if (cap) {
        ILI9341_FillCircle(ili9341, x1, y1, halfThickness, color);
        ILI9341_FillCircle(ili9341, x2, y2, halfThickness, color);
    }
}

void ILI9341_DrawRectangle(
    const ILI9341_HandleTypeDef* ili9341,
    int_fast16_t x,
    int_fast16_t y,
    int_fast16_t w,
    int_fast16_t h,
    uint16_t color
) {
    ILI9341_DrawRectangleThick(ili9341, x, y, w, h, color, 1);
}

void ILI9341_DrawRectangleThick(
    const ILI9341_HandleTypeDef* ili9341,
    int_fast16_t x,
    int_fast16_t y,
    int_fast16_t w,
    int_fast16_t h,
    uint16_t color,
    int_fast16_t thickness
) {
    if (thickness <= 0) return;

    ILI9341_Select(ili9341);

    ILI9341_FillRectangleFast(ili9341, x, y, w, thickness, color);                              // top
    ILI9341_FillRectangleFast(ili9341, x, y + h - thickness, w, thickness, color);              // bottom
    ILI9341_FillRectangleFast(ili9341, x, y + thickness, thickness, h - 2 * thickness, color);  // left
    ILI9341_FillRectangleFast(ili9341, x + w - thickness, y + thickness, thickness, h - 2 * thickness, color);  // right

    ILI9341_Deselect(ili9341);
}

void ILI9341_DrawCircle(
    const ILI9341_HandleTypeDef* ili9341,
    int_fast16_t xc,
    int_fast16_t yc,
    int_fast16_t r,
    uint16_t color
) {
    r = abs(r);
    if (r == 0 || xc + r < 0 || xc - r >= ili9341->width || yc + r < 0 || yc - r >= ili9341->height) return;

    int_fast16_t f = 1 - r;
    int_fast16_t dfx = -2 * r;
    int_fast16_t dfy = 1;
    int_fast16_t x = r;
    int_fast16_t y = 0;

    ILI9341_Select(ili9341);

    ILI9341_DrawPixelFast(ili9341, xc, yc + r, color);
    ILI9341_DrawPixelFast(ili9341, xc, yc - r, color);
    ILI9341_DrawPixelFast(ili9341, xc + r, yc, color);
    ILI9341_DrawPixelFast(ili9341, xc - r, yc, color);

    while (x >= y) {
        if (f >= 0) {
            x--;
            dfx += 2;
            f += dfx;
        }
        y++;
        dfy += 2;
        f += dfy;

        ILI9341_DrawPixelFast(ili9341, xc + x, yc + y, color);
        ILI9341_DrawPixelFast(ili9341, xc - x, yc + y, color);
        ILI9341_DrawPixelFast(ili9341, xc + x, yc - y, color);
        ILI9341_DrawPixelFast(ili9341, xc - x, yc - y, color);
        ILI9341_DrawPixelFast(ili9341, xc + y, yc + x, color);
        ILI9341_DrawPixelFast(ili9341, xc - y, yc + x, color);
        ILI9341_DrawPixelFast(ili9341, xc + y, yc - x, color);
        ILI9341_DrawPixelFast(ili9341, xc - y, yc - x, color);
    }

    ILI9341_Deselect(ili9341);
}

void ILI9341_DrawCircleThick(
    const ILI9341_HandleTypeDef* ili9341,
    int_fast16_t xc,
    int_fast16_t yc,
    int_fast16_t r,
    uint16_t color,
    int_fast16_t thickness
) {
    r = abs(r);
    if (r == 0 || thickness <= 0 || xc + r < 0 || xc - r >= ili9341->width || yc + r < 0 || yc - r >= ili9341->height)
        return;
    if (thickness > r) thickness = r;

    int_fast16_t ri = r - thickness;
    int_fast16_t xo = r;
    int_fast16_t xi = ri;

    ILI9341_Select(ili9341);

    for (int_fast16_t y = 0; y <= r; y++) {
        while (xo * xo + y * y > r * r) { xo--; }
        while (xi * xi + y * y > ri * ri && xi > 0) { xi--; }

        ILI9341_DrawLineFast(ili9341, xc - xo, yc + y, xc - xi, yc + y, color);
        ILI9341_DrawLineFast(ili9341, xc + xi, yc + y, xc + xo, yc + y, color);
        ILI9341_DrawLineFast(ili9341, xc - xo, yc - y, xc - xi, yc - y, color);
        ILI9341_DrawLineFast(ili9341, xc + xi, yc - y, xc + xo, yc - y, color);
    }

    ILI9341_Deselect(ili9341);
}

void ILI9341_FillCircle(
    const ILI9341_HandleTypeDef* ili9341,
    int_fast16_t xc,
    int_fast16_t yc,
    int_fast16_t r,
    uint16_t color
) {
    r = abs(r);
    if (r == 0 || xc + r < 0 || xc - r >= ili9341->width || yc + r < 0 || yc - r >= ili9341->height) return;

    int_fast16_t f = 1 - r;
    int_fast16_t dfx = -2 * r;
    int_fast16_t dfy = 1;
    int_fast16_t x = r;
    int_fast16_t y = 0;

    ILI9341_Select(ili9341);

    ILI9341_DrawLineFast(ili9341, xc - r, yc, xc + r, yc, color);
    ILI9341_DrawPixelFast(ili9341, xc, yc + r, color);
    ILI9341_DrawPixelFast(ili9341, xc, yc - r, color);

    while (x >= y) {
        if (f >= 0) {
            x--;
            dfx += 2;
            f += dfx;
        }
        y++;
        dfy += 2;
        f += dfy;

        ILI9341_DrawLineFast(ili9341, xc - x, yc + y, xc + x, yc + y, color);
        ILI9341_DrawLineFast(ili9341, xc - x, yc - y, xc + x, yc - y, color);
        ILI9341_DrawLineFast(ili9341, xc - y, yc + x, xc + y, yc + x, color);
        ILI9341_DrawLineFast(ili9341, xc - y, yc - x, xc + y, yc - x, color);
    }

    ILI9341_Deselect(ili9341);
}

void ILI9341_DrawEllipse(
    const ILI9341_HandleTypeDef* ili9341,
    int_fast16_t xc,
    int_fast16_t yc,
    int_fast16_t rx,
    int_fast16_t ry,
    uint16_t color
) {
    rx = abs(rx);
    ry = abs(ry);
    if (rx == 0 || ry == 0 || xc + rx < 0 || xc - rx >= ili9341->width || yc + ry < 0 || yc - ry >= ili9341->height)
        return;

    int_fast32_t rx2 = rx * rx;
    int_fast32_t ry2 = ry * ry;
    int_fast32_t twoRx2 = 2 * rx2;
    int_fast32_t twoRy2 = 2 * ry2;
    int_fast32_t p;
    int_fast32_t x = 0;
    int_fast32_t y = ry;
    int_fast32_t px = 0;
    int_fast32_t py = twoRx2 * y;

    ILI9341_Select(ili9341);

    ILI9341_DrawPixelFast(ili9341, xc, yc + ry, color);
    ILI9341_DrawPixelFast(ili9341, xc, yc - ry, color);

    p = ry2 - (rx2 * ry) + (rx2 / 4);
    while (px < py) {
        x++;
        px += twoRy2;
        if (p < 0) {
            p += ry2 + px;
        } else {
            y--;
            py -= twoRx2;
            p += ry2 + px - py;
        }

        ILI9341_DrawPixelFast(ili9341, xc + x, yc + y, color);
        ILI9341_DrawPixelFast(ili9341, xc - x, yc + y, color);
        ILI9341_DrawPixelFast(ili9341, xc + x, yc - y, color);
        ILI9341_DrawPixelFast(ili9341, xc - x, yc - y, color);
    }

    p = ry2 * (x + 1) * (x + 1) + rx2 * (y - 1) * (y - 1) - rx2 * ry2;
    while (y > 0) {
        y--;
        py -= twoRx2;
        if (p > 0) {
            p += rx2 - py;
        } else {
            x++;
            px += twoRy2;
            p += rx2 - py + px;
        }

        ILI9341_DrawPixelFast(ili9341, xc + x, yc + y, color);
        ILI9341_DrawPixelFast(ili9341, xc - x, yc + y, color);
        ILI9341_DrawPixelFast(ili9341, xc + x, yc - y, color);
        ILI9341_DrawPixelFast(ili9341, xc - x, yc - y, color);
    }

    ILI9341_Deselect(ili9341);
}

void ILI9341_DrawEllipseThick(
    const ILI9341_HandleTypeDef* ili9341,
    int_fast16_t xc,
    int_fast16_t yc,
    int_fast16_t rx,
    int_fast16_t ry,
    uint16_t color,
    int_fast16_t thickness
) {
    rx = abs(rx);
    ry = abs(ry);
    if (rx == 0 || ry == 0 || thickness <= 0 || xc + rx < 0 || xc - rx >= ili9341->width || yc + ry < 0 ||
        yc - ry >= ili9341->height)
        return;
    if (thickness > rx || thickness > ry) {
        ILI9341_FillEllipse(ili9341, xc, yc, rx, ry, color);
        return;
    }

    int_fast16_t x = rx;
    int_fast16_t rxi = rx - thickness;
    int_fast16_t ryi = ry - thickness;
    int_fast16_t xi = rxi;

    ILI9341_Select(ili9341);

    for (int_fast16_t y = 0; y <= ry; y++) {
        while (x * x * ry * ry + y * y * rx * rx > rx * rx * ry * ry) { x--; }
        while (xi * xi * ryi * ryi + y * y * rxi * rxi > rxi * rxi * ryi * ryi && xi > 0) { xi--; }
        ILI9341_DrawLineFast(ili9341, xc - x, yc + y, xc - xi, yc + y, color);
        ILI9341_DrawLineFast(ili9341, xc + xi, yc + y, xc + x, yc + y, color);
        ILI9341_DrawLineFast(ili9341, xc - x, yc - y, xc - xi, yc - y, color);
        ILI9341_DrawLineFast(ili9341, xc + xi, yc - y, xc + x, yc - y, color);
    }

    ILI9341_Deselect(ili9341);
}

void ILI9341_FillEllipse(
    const ILI9341_HandleTypeDef* ili9341,
    int_fast16_t xc,
    int_fast16_t yc,
    int_fast16_t rx,
    int_fast16_t ry,
    uint16_t color
) {
    rx = abs(rx);
    ry = abs(ry);
    if (rx == 0 || ry == 0 || xc + rx < 0 || xc - rx >= ili9341->width || yc + ry < 0 || yc - ry >= ili9341->height)
        return;

    int_fast32_t rx2 = rx * rx;
    int_fast32_t ry2 = ry * ry;
    int_fast32_t twoRx2 = 2 * rx2;
    int_fast32_t twoRy2 = 2 * ry2;
    int_fast32_t p;
    int_fast32_t x = 0;
    int_fast32_t y = ry;
    int_fast32_t px = 0;
    int_fast32_t py = twoRx2 * y;

    ILI9341_Select(ili9341);

    ILI9341_DrawLineFast(ili9341, xc - rx, yc, xc + rx, yc, color);
    ILI9341_DrawPixelFast(ili9341, xc, yc + ry, color);
    ILI9341_DrawPixelFast(ili9341, xc, yc - ry, color);

    p = ry2 - (rx2 * ry) + (rx2 / 4);
    while (px < py) {
        x++;
        px += twoRy2;
        if (p < 0) {
            p += ry2 + px;
        } else {
            y--;
            py -= twoRx2;
            p += ry2 + px - py;
        }

        ILI9341_DrawLineFast(ili9341, xc - x, yc + y, xc + x, yc + y, color);
        ILI9341_DrawLineFast(ili9341, xc - x, yc - y, xc + x, yc - y, color);
    }

    p = ry2 * (x + 1) * (x + 1) + ry2 / 4 + rx2 * (y - 1) * (y - 1) - rx2 * ry2;
    while (y > 0) {
        y--;
        py -= twoRx2;
        if (p > 0) {
            p += rx2 - py;
        } else {
            x++;
            px += twoRy2;
            p += rx2 - py + px;
        }

        ILI9341_DrawLineFast(ili9341, xc - x, yc + y, xc + x, yc + y, color);
        ILI9341_DrawLineFast(ili9341, xc - x, yc - y, xc + x, yc - y, color);
    }

    ILI9341_Deselect(ili9341);
}

void ILI9341_DrawPolygon(const ILI9341_HandleTypeDef* ili9341, int16_t* x, int16_t* y, size_t n, uint16_t color) {
    if (n < 2) return;

    ILI9341_Select(ili9341);

    for (size_t i = 0; i < n - 1; i++) { ILI9341_DrawLineFast(ili9341, x[i], y[i], x[i + 1], y[i + 1], color); }
    ILI9341_DrawLineFast(ili9341, x[n - 1], y[n - 1], x[0], y[0], color);

    ILI9341_Deselect(ili9341);
}

void ILI9341_DrawPolygonThick(
    const ILI9341_HandleTypeDef* ili9341,
    int16_t* x,
    int16_t* y,
    size_t n,
    uint16_t color,
    int_fast16_t thickness,
    bool cap
) {
    if (n < 2 || thickness <= 0) return;

    for (size_t i = 0; i < n - 1; i++) {
        ILI9341_DrawLineThick(ili9341, x[i], y[i], x[i + 1], y[i + 1], color, thickness, cap);
    }
    ILI9341_DrawLineThick(ili9341, x[n - 1], y[n - 1], x[0], y[0], color, thickness, cap);
}

void ILI9341_FillPolygon(const ILI9341_HandleTypeDef* ili9341, int16_t* x, int16_t* y, size_t n, uint16_t color) {
    if (n < 3) return;

    // find max and min Y
    int_fast16_t minY = y[0], maxY = y[0];
    for (size_t i = 1; i < n; i++) {
        if (y[i] < minY) minY = y[i];
        if (y[i] > maxY) maxY = y[i];
    }

    if (minY >= ili9341->height) return;
    if (maxY < 0) return;
    if (minY < 0) minY = 0;
    if (maxY >= ili9341->height) maxY = ili9341->height - 1;

    int_fast16_t nodeX[32];  // max 32 intersections

    ILI9341_Select(ili9341);

    // scanline algorithm
    for (int_fast16_t j = minY; j <= maxY; j++) {
        int_fast16_t nodes = 0;
        int_fast16_t k = n - 1;

        // find intersections
        for (int_fast16_t i = 0; i < n; i++) {
            if ((y[i] < j && y[k] >= j) || (y[k] < j && y[i] >= j)) {
                int_fast16_t dy = y[k] - y[i];
                if (dy != 0 && nodes < 32) {
                    nodeX[nodes++] = x[i] + ((int_fast32_t)(j - y[i]) * (int_fast32_t)(x[k] - x[i])) / dy;
                }
            }
            k = i;
        }

        // insertion sort nodeX array
        for (int_fast16_t i = 1; i < nodes; i++) {
            int_fast16_t key = nodeX[i];
            int_fast16_t jSort = i - 1;
            while (jSort >= 0 && nodeX[jSort] > key) {
                nodeX[jSort + 1] = nodeX[jSort];
                jSort--;
            }
            nodeX[jSort + 1] = key;
        }

        // fill the pixels between node pairs
        for (int_fast16_t i = 0; i < nodes - 1; i += 2) {
            int_fast16_t x1 = nodeX[i];
            int_fast16_t x2 = nodeX[i + 1];

            if (x1 >= ili9341->width) break;
            if (x2 >= ili9341->width) x2 = ili9341->width - 1;
            if (x1 < ili9341->width && x2 > 0 && x2 >= x1) {
                ILI9341_FillRectangleFast(ili9341, x1, j, x2 - x1 + 1, 1, color);
            }
        }
    }

    ILI9341_Deselect(ili9341);
}
