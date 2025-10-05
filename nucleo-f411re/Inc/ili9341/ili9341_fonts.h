#ifndef __ILI9341_FONTS_H__
#define __ILI9341_FONTS_H__

#include "stdint.h"
#include "stdlib.h"

/**
 * @brief Glyph definition structure
 * @note The glyph location is based on the bottom-left corner of the glyph.
 */
typedef struct {
    /** Bounding box X offset (positive is right) */
    const int8_t bbX;
    /** Bounding box Y offset (positive is up) */
    const int8_t bbY;
    /** Bounding box width */
    const int8_t bbW;
    /** Bounding box height */
    const int8_t bbH;
    /** Horizontal advance to the next character */
    const int8_t advance;
    /** Pointer to the actual glyph data, the data format is binary bitmap stored as uint8_t array.
     * Order is left-to-right, bottom-to-top. 1 is foreground, 0 is background (or none for transparent mode) */
    const uint8_t* data;
} ILI9341_GlyphDef;

/**
 * @brief Font definition structure
 * @note The 0x7F (del) glyph will be used for out-of-range character
 */
typedef struct {
    /** Start codepoint (inclusive) */
    const uint_fast8_t startCodepoint;
    /** End codepoint (inclusive) */
    const uint_fast8_t endCodepoint;
    /** Average char width of the font, in 10 pixels (eg. average width 80 -> 80 / 10 = 8 pixels) */
    const int_fast8_t averageWidth;
    /** Font ascent (how far up does the font extend from baseline) */
    const int_fast8_t ascent;
    /** Font descent (how far down does the font extend from baseline) */
    const int_fast8_t descent;
    /** Glyphs data */
    const ILI9341_GlyphDef* glyphs;
} ILI9341_FontDef;

extern const ILI9341_FontDef ILI9341_Font_Terminus6x12b;
extern const ILI9341_FontDef ILI9341_Font_Terminus6x12;
extern const ILI9341_FontDef ILI9341_Font_Terminus8x14b;
extern const ILI9341_FontDef ILI9341_Font_Terminus8x14;
extern const ILI9341_FontDef ILI9341_Font_Terminus8x14v;
extern const ILI9341_FontDef ILI9341_Font_Terminus8x16b;
extern const ILI9341_FontDef ILI9341_Font_Terminus8x16;
extern const ILI9341_FontDef ILI9341_Font_Terminus8x16v;
extern const ILI9341_FontDef ILI9341_Font_Terminus10x18b;
extern const ILI9341_FontDef ILI9341_Font_Terminus10x18;
extern const ILI9341_FontDef ILI9341_Font_Terminus10x20b;
extern const ILI9341_FontDef ILI9341_Font_Terminus10x20;
extern const ILI9341_FontDef ILI9341_Font_Terminus11x22b;
extern const ILI9341_FontDef ILI9341_Font_Terminus11x22;
extern const ILI9341_FontDef ILI9341_Font_Terminus12x24b;
extern const ILI9341_FontDef ILI9341_Font_Terminus12x24;
extern const ILI9341_FontDef ILI9341_Font_Terminus14x28b;
extern const ILI9341_FontDef ILI9341_Font_Terminus14x28;
extern const ILI9341_FontDef ILI9341_Font_Terminus16x32b;
extern const ILI9341_FontDef ILI9341_Font_Terminus16x32;

extern const ILI9341_FontDef ILI9341_Font_Spleen5x8;
extern const ILI9341_FontDef ILI9341_Font_Spleen6x12;
extern const ILI9341_FontDef ILI9341_Font_Spleen8x16;
extern const ILI9341_FontDef ILI9341_Font_Spleen12x24;
extern const ILI9341_FontDef ILI9341_Font_Spleen16x32;
extern const ILI9341_FontDef ILI9341_Font_Spleen32x64;

extern const ILI9341_FontDef ILI9341_Font_Manop6x14;
extern const ILI9341_FontDef ILI9341_Font_Manop7x18;
extern const ILI9341_FontDef ILI9341_Font_Manop8x20;

#endif  // __ILI9341_FONTS_H__
