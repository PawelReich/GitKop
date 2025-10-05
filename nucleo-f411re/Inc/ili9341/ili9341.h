#ifndef __ILI9341_H__
#define __ILI9341_H__

#include "ili9341_fonts.h"
#include "math.h"
#include "stdbool.h"
#include "stdint.h"
#include "stdlib.h"
#include "stm32f4xx_hal.h"

#define ILI9341_MADCTL_MY 0x80
#define ILI9341_MADCTL_MX 0x40
#define ILI9341_MADCTL_MV 0x20
#define ILI9341_MADCTL_ML 0x10
#define ILI9341_MADCTL_RGB 0x00
#define ILI9341_MADCTL_BGR 0x08
#define ILI9341_MADCTL_MH 0x04

// default orientation
#define ILI9341_ROTATION_VERTICAL_1 0

// rotate right
#define ILI9341_ROTATION_HORIZONTAL_1 1

// rotate left
#define ILI9341_ROTATION_HORIZONTAL_2 2

// upside down
#define ILI9341_ROTATION_VERTICAL_2 3

// Color definitions
#define ILI9341_COLOR_BLACK 0x0000
#define ILI9341_COLOR_BLUE 0x001F
#define ILI9341_COLOR_RED 0xF800
#define ILI9341_COLOR_GREEN 0x07E0
#define ILI9341_COLOR_CYAN 0x07FF
#define ILI9341_COLOR_MAGENTA 0xF81F
#define ILI9341_COLOR_YELLOW 0xFFE0
#define ILI9341_COLOR_WHITE 0xFFFF

#define ILI9341_COLOR565(r, g, b) \
    ((((uint16_t)r & 0xF8) << 8) | (((uint16_t)g & 0xFC) << 3) | (((uint16_t)b & 0xF8) >> 3))

// Other constants
#define ILI9341_FILL_RECT_BUFFER_SIZE 512   // pixels x 2 bytes per pixel = 1024 bytes
#define ILI9341_DRAW_GLYPH_BUFFER_SIZE 512  // pixels x 2 bytes per pixel = 1024 bytes
#define FALLBACK_CODEPOINT 0x7F

/**
 * @brief ILI9341 handle structure
 */
typedef struct {
    SPI_HandleTypeDef* spi_handle;
    GPIO_TypeDef* cs_port;
    uint16_t cs_pin;
    GPIO_TypeDef* dc_port;
    uint16_t dc_pin;
    GPIO_TypeDef* rst_port;
    uint16_t rst_pin;
    int_fast8_t rotation;
    int_fast16_t width;
    int_fast16_t height;
} ILI9341_HandleTypeDef;

/**
 * @brief Deselect the ILI9341 display, call before using other SPI peripherals on the same bus
 * @param ili9341 Pointer to ILI9341 handle structure
 */
void ILI9341_Deselect(const ILI9341_HandleTypeDef* ili9341);

/**
 * @brief Initialize the ILI9341 display
 * @param ili9341 Pointer to ILI9341 handle structure
 * @param spi_handle Pointer to the SPI handle
 * @param cs_port GPIO port for Chip Select pin
 * @param cs_pin GPIO pin for Chip Select
 * @param dc_port GPIO port for Data/Command pin
 * @param dc_pin GPIO pin for Data/Command
 * @param rst_port GPIO port for Reset pin
 * @param rst_pin GPIO pin for Reset
 * @param rotation Initial display rotation, one of ILI9341_ROTATION_* values
 * @param width Display width in pixels
 * @param height Display height in pixels
 * @return Initialized ILI9341_HandleTypeDef structure
 */
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
);

/**
 * @brief Set display orientation
 * @param ili9341 Pointer to ILI9341 handle structure
 * @param rotation New display rotation, one of ILI9341_ROTATION_* values
 */
void ILI9341_SetOrientation(ILI9341_HandleTypeDef* ili9341, int_fast8_t rotation);

/**
 * @brief Set display brightness (only for displays with backlight control via ILI9341)
 * @param ili9341 Pointer to ILI9341 handle structure
 * @param brightness Brightness level from 0 (min) to 255 (max)
 */
void ILI9341_SetBrightness(const ILI9341_HandleTypeDef* ili9341, uint_fast8_t brightness);

/**
 * @brief Invert the display colors
 * @param ili9341 Pointer to ILI9341 handle structure
 * @param invert true to invert colors, false for normal colors
 */
void ILI9341_InvertColors(const ILI9341_HandleTypeDef* ili9341, bool invert);

/**
 * @brief Draw a single pixel at specified coordinates
 * @param ili9341 Pointer to ILI9341 handle structure
 * @param x X coordinate of the pixel
 * @param y Y coordinate of the pixel
 * @param color 16-bit color of the pixel in RGB565 format
 */
void ILI9341_DrawPixel(const ILI9341_HandleTypeDef* ili9341, int_fast16_t x, int_fast16_t y, uint16_t color);

/**
 * @brief Fill a rectangle with specified color
 * @param ili9341 Pointer to ILI9341 handle structure
 * @param x X coordinate of the top-left corner of the rectangle
 * @param y Y coordinate of the top-left corner of the rectangle
 * @param w Width of the rectangle in pixels
 * @param h Height of the rectangle in pixels
 * @param color 16-bit fill color in RGB565 format
 */
void ILI9341_FillRectangle(
    const ILI9341_HandleTypeDef* ili9341,
    int_fast16_t x,
    int_fast16_t y,
    int_fast16_t w,
    int_fast16_t h,
    uint16_t color
);

/**
 * @brief Fill the entire screen with specified color
 * @param ili9341 Pointer to ILI9341 handle structure
 * @param color 16-bit fill color in RGB565 format
 */
void ILI9341_FillScreen(const ILI9341_HandleTypeDef* ili9341, uint16_t color);

/**
 * @brief Write a string to the display with specified font and colors
 * @param ili9341 Pointer to ILI9341 handle structure
 * @param x X coordinate of the left of the string
 * @param y Y coordinate of the baseline of the string
 * @param str Null-terminated string to write
 * @param font Font definition to use for rendering the string
 * @param color 16-bit text color in RGB565 format
 * @param bgcolor 16-bit background color in RGB565 format
 * @param wrap Whether to wrap text to the next line if it exceeds display width
 * @param scale Integer scaling factor for the font, must be >= 1
 * @param tracking Additional space in pixels between characters, can be negative (negative tracking should only be used
 *                 with ILI9341_WriteStringTransparent to avoid background overlap)
 * @param leading Additional space in pixels between lines when wrapping, can be negative (negative leading should only
 *                be used with ILI9341_WriteStringTransparent to avoid background overlap)
 */
void ILI9341_WriteString(
    const ILI9341_HandleTypeDef* ili9341,
    int_fast16_t x,
    int_fast16_t y,
    const char* str,
    ILI9341_FontDef font,
    uint16_t color,
    uint16_t bgcolor,
    bool wrap,
    int_fast16_t scale,
    int_fast16_t tracking,
    int_fast16_t leading
);

/**
 * @brief Write a scaled string to the display with specified font and color, background is not drawn (transparent)
 * @param ili9341 Pointer to ILI9341 handle structure
 * @param x X coordinate of the left corner of the string
 * @param y Y coordinate of the baseline corner of the string
 * @param str Null-terminated string to write
 * @param font Font definition to use for rendering the string
 * @param color 16-bit text color in RGB565 format
 * @param wrap Whether to wrap text to the next line if it exceeds display width
 * @param scale Integer scaling factor for the font, must be >= 1
 * @param tracking Additional space in pixels between characters, can be negative
 * @param leading Additional space in pixels between lines when wrapping, can be negative
 */
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
);

/**
 * @brief Draw an image (bitmap) at specified coordinates
 * @param ili9341 Pointer to ILI9341 handle structure
 * @param x X coordinate of the top-left corner of the image
 * @param y Y coordinate of the top-left corner of the image
 * @param w Width of the image in pixels
 * @param h Height of the image in pixels
 * @param data Pointer to the image pixel data in RGB565 format with the 2 bytes swapped, must contain at least w*h
 * elements
 */
void ILI9341_DrawImage(
    const ILI9341_HandleTypeDef* ili9341,
    int_fast16_t x,
    int_fast16_t y,
    int_fast16_t w,
    int_fast16_t h,
    const uint16_t* data
);

/**
 * @brief Draw a thin line between two points
 * @param ili9341 Pointer to ILI9341 handle structure
 * @param x1 X coordinate of the start point
 * @param y1 Y coordinate of the start point
 * @param x2 X coordinate of the end point
 * @param y2 Y coordinate of the end point
 * @param color 16-bit line color in RGB565 format
 */
void ILI9341_DrawLine(
    const ILI9341_HandleTypeDef* ili9341,
    int_fast16_t x1,
    int_fast16_t y1,
    int_fast16_t x2,
    int_fast16_t y2,
    uint16_t color
);

/**
 * @brief Draw a thick line between two points
 * @param ili9341 Pointer to ILI9341 handle structure
 * @param x1 X coordinate of the start point
 * @param y1 Y coordinate of the start point
 * @param x2 X coordinate of the end point
 * @param y2 Y coordinate of the end point
 * @param color 16-bit line color in RGB565 format
 * @param thickness Line thickness in pixels, must be >= 1
 * @param cap true to draw rounded line caps, false for no caps
 */
void ILI9341_DrawLineThick(
    const ILI9341_HandleTypeDef* ili9341,
    int_fast16_t x1,
    int_fast16_t y1,
    int_fast16_t x2,
    int_fast16_t y2,
    uint16_t color,
    int_fast16_t thickness,
    bool cap
);

/**
 * @brief Draw a thin rectangle outline
 * @param ili9341 Pointer to ILI9341 handle structure
 * @param x X coordinate of the top-left corner of the rectangle
 * @param y Y coordinate of the top-left corner of the rectangle
 * @param w Width of the rectangle in pixels
 * @param h Height of the rectangle in pixels
 * @param color 16-bit rectangle color in RGB565 format
 */
void ILI9341_DrawRectangle(
    const ILI9341_HandleTypeDef* ili9341,
    int_fast16_t x,
    int_fast16_t y,
    int_fast16_t w,
    int_fast16_t h,
    uint16_t color
);

/**
 * @brief Draw a thick rectangle outline
 * @param ili9341 Pointer to ILI9341 handle structure
 * @param x X coordinate of the top-left corner of the rectangle
 * @param y Y coordinate of the top-left corner of the rectangle
 * @param w Width of the rectangle in pixels
 * @param h Height of the rectangle in pixels
 * @param color 16-bit rectangle color in RGB565 format
 * @param thickness Line thickness in pixels, must be >= 1
 */
void ILI9341_DrawRectangleThick(
    const ILI9341_HandleTypeDef* ili9341,
    int_fast16_t x,
    int_fast16_t y,
    int_fast16_t w,
    int_fast16_t h,
    uint16_t color,
    int_fast16_t thickness
);

/**
 * @brief Draw a thin circle outline
 * @param ili9341 Pointer to ILI9341 handle structure
 * @param x X coordinate of the center of the circle
 * @param y Y coordinate of the center of the circle
 * @param r Radius of the circle
 * @param color 16-bit circle color in RGB565 format
 */
void ILI9341_DrawCircle(
    const ILI9341_HandleTypeDef* ili9341,
    int_fast16_t x,
    int_fast16_t y,
    int_fast16_t r,
    uint16_t color
);

/**
 * @brief Draw a thick circle outline
 * @param ili9341 Pointer to ILI9341 handle structure
 * @param x0 X coordinate of the center of the circle
 * @param y0 Y coordinate of the center of the circle
 * @param r Radius of the circle
 * @param color 16-bit circle color in RGB565 format
 * @param thickness Circle line thickness in pixels, must be >= 1 and <= r
 */
void ILI9341_DrawCircleThick(
    const ILI9341_HandleTypeDef* ili9341,
    int_fast16_t x0,
    int_fast16_t y0,
    int_fast16_t r,
    uint16_t color,
    int_fast16_t thickness
);

/**
 * @brief Fill a circle
 * @param ili9341 Pointer to ILI9341 handle structure
 * @param x X coordinate of the center of the circle
 * @param y Y coordinate of the center of the circle
 * @param r Radius of the circle
 * @param color 16-bit circle color in RGB565 format
 */
void ILI9341_FillCircle(
    const ILI9341_HandleTypeDef* ili9341,
    int_fast16_t x,
    int_fast16_t y,
    int_fast16_t r,
    uint16_t color
);

/**
 * @brief Draw a thin ellipse outline
 * @param ili9341 Pointer to ILI9341 handle structure
 * @param xc X coordinate of the center of the ellipse
 * @param yc Y coordinate of the center of the ellipse
 * @param rx Horizontal radius of the ellipse
 * @param ry Vertical radius of the ellipse
 * @param color 16-bit ellipse color in RGB565 format
 */
void ILI9341_DrawEllipse(
    const ILI9341_HandleTypeDef* ili9341,
    int_fast16_t xc,
    int_fast16_t yc,
    int_fast16_t rx,
    int_fast16_t ry,
    uint16_t color
);

/**
 * @brief Draw a thick ellipse outline
 * @param ili9341 Pointer to ILI9341 handle structure
 * @param xc X coordinate of the center of the ellipse
 * @param yc Y coordinate of the center of the ellipse
 * @param rx Horizontal radius of the ellipse
 * @param ry Vertical radius of the ellipse
 * @param color 16-bit ellipse color in RGB565 format
 * @param thickness Ellipse line thickness in pixels, must be >= 1
 */
void ILI9341_DrawEllipseThick(
    const ILI9341_HandleTypeDef* ili9341,
    int_fast16_t xc,
    int_fast16_t yc,
    int_fast16_t rx,
    int_fast16_t ry,
    uint16_t color,
    int_fast16_t thickness
);

/**
 * @brief Fill an ellipse
 * @param ili9341 Pointer to ILI9341 handle structure
 * @param xc X coordinate of the center of the ellipse
 * @param yc Y coordinate of the center of the ellipse
 * @param rx Horizontal radius of the ellipse
 * @param ry Vertical radius of the ellipse
 * @param color 16-bit ellipse color in RGB565 format
 */
void ILI9341_FillEllipse(
    const ILI9341_HandleTypeDef* ili9341,
    int_fast16_t xc,
    int_fast16_t yc,
    int_fast16_t rx,
    int_fast16_t ry,
    uint16_t color
);

/**
 * @brief Draw a polygon outline
 * @param ili9341 Pointer to ILI9341 handle structure
 * @param x Array of X coordinates of the polygon vertices
 * @param y Array of Y coordinates of the polygon vertices
 * @param n Number of vertices in the polygon
 * @param color 16-bit polygon color in RGB565 format
 * @note The polygon is automatically closed by connecting the last vertex to the first.
 */
void ILI9341_DrawPolygon(const ILI9341_HandleTypeDef* ili9341, int16_t* x, int16_t* y, size_t n, uint16_t color);

/**
 * @brief Draw a thick polygon outline
 * @param ili9341 Pointer to ILI9341 handle structure
 * @param x Array of X coordinates of the polygon vertices
 * @param y Array of Y coordinates of the polygon vertices
 * @param n Number of vertices in the polygon
 * @param color 16-bit polygon color in RGB565 format
 * @param thickness Line thickness in pixels, must be >= 1
 * @param cap true to draw rounded line caps, false for no caps
 * @note The polygon is automatically closed by connecting the last vertex to the first.
 */
void ILI9341_DrawPolygonThick(
    const ILI9341_HandleTypeDef* ili9341,
    int16_t* x,
    int16_t* y,
    size_t n,
    uint16_t color,
    int_fast16_t thickness,
    bool cap
);

/**
 * @brief Fill a polygon
 * @param ili9341 Pointer to ILI9341 handle structure
 * @param x Array of X coordinates of the polygon vertices
 * @param y Array of Y coordinates of the polygon vertices
 * @param n Number of vertices in the polygon
 * @param color 16-bit polygon color in RGB565 format
 * @note The algorithm used is scanline algorithm, with support for concave and self-intersecting polygons. Max
 * intersections for scanline is 32.
 */
void ILI9341_FillPolygon(const ILI9341_HandleTypeDef* ili9341, int16_t* x, int16_t* y, size_t n, uint16_t color);

#endif  // __ILI9341_H__
