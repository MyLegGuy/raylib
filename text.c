/**********************************************************************************************
*
*   raylib.text - Basic functions to load Fonts and draw Text
*
*   CONFIGURATION:
*
*   DEPENDENCIES:
*       stb_truetype  - Load TTF file and rasterize characters data
*       stb_rect_pack - Rectangles packing algorythms, required for font atlas generation
*
*
*   LICENSE: zlib/libpng
*
*   Copyright (c) 2013-2019 Ramon Santamaria (@raysan5)
*
*   This software is provided "as-is", without any express or implied warranty. In no event
*   will the authors be held liable for any damages arising from the use of this software.
*
*   Permission is granted to anyone to use this software for any purpose, including commercial
*   applications, and to alter it and redistribute it freely, subject to the following restrictions:
*
*     1. The origin of this software must not be misrepresented; you must not claim that you
*     wrote the original software. If you use this software in a product, an acknowledgment
*     in the product documentation would be appreciated but is not required.
*
*     2. Altered source versions must be plainly marked as such, and must not be misrepresented
*     as being the original software.
*
*     3. This notice may not be removed or altered from any source distribution.
*
**********************************************************************************************/

#include "rayn.h"         // Declares module functions

// Check if config flags have been externally provided on compilation line
#if !defined(EXTERNAL_CONFIG_FLAGS)
    #include "config.h"     // Defines module configuration flags
#endif

#include <stdlib.h>         // Required for: malloc(), free()
#include <string.h>         // Required for: strlen()
#include <stdarg.h>         // Required for: va_list, va_start(), vfprintf(), va_end()
#include <stdio.h>          // Required for: FILE, fopen(), fclose(), fscanf(), feof(), rewind(), fgets()
#include <ctype.h>          // Required for: toupper(), tolower()

#include "utils.h"          // Required for: fopen() Android mapping

#define STB_RECT_PACK_IMPLEMENTATION
#include "external/stb_rect_pack.h"     // Required for: ttf font rectangles packaging

#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#include "external/stb_truetype.h"      // Required for: ttf font data reading

//----------------------------------------------------------------------------------
// Module Functions Definition
//----------------------------------------------------------------------------------

// Load Font from file into GPU memory (VRAM)
Font LoadFont(const char *fileName)
{
    // Default hardcoded values for ttf file loading
    #define DEFAULT_TTF_FONTSIZE    32      // Font first character (32 - space)
    #define DEFAULT_TTF_NUMCHARS    95      // ASCII 32..126 is 95 glyphs
    #define DEFAULT_FIRST_CHAR      32      // Expected first char for image sprite font

    Font font = LoadFontEx(fileName, DEFAULT_TTF_FONTSIZE, NULL, DEFAULT_TTF_NUMCHARS);

    if (font.texture.id == 0)
    {
        TraceLog(LOG_WARNING, "[%s] Font could not be loaded.", fileName);
    }
    else SetTextureFilter(font.texture, FILTER_POINT);    // By default we set point filter (best performance)

    return font;
}

// Load Font from TTF font file with generation parameters
// NOTE: You can pass an array with desired characters, those characters should be available in the font
// if array is NULL, default char set is selected 32..126
Font LoadFontEx(const char *fileName, int fontSize, int *fontChars, int charsCount)
{
    Font font = { 0 };

    font.baseSize = fontSize;
    font.charsCount = (charsCount > 0)? charsCount : 95;
    font.chars = LoadFontData(fileName, font.baseSize, fontChars, font.charsCount, FONT_DEFAULT);

    if (font.chars != NULL)
    {
        Image atlas = GenImageFontAtlas(font.chars, &font.recs, font.charsCount, font.baseSize, 2, 0);
        font.texture = LoadTextureFromImage(atlas);

        // Update chars[i].image to use alpha, required to be used on ImageDrawText()
        for (int i = 0; i < font.charsCount; i++)
        {
            UnloadImage(font.chars[i].image);
            font.chars[i].image = ImageFromImage(atlas, font.recs[i]);
        }

        UnloadImage(atlas);
    }

    return font;
}

// Load font data for further use
// NOTE: Requires TTF font and can generate SDF data
CharInfo *LoadFontData(const char *fileName, int fontSize, int *fontChars, int charsCount, int type)
{
    // NOTE: Using some SDF generation default values,
    // trades off precision with ability to handle *smaller* sizes
    #define SDF_CHAR_PADDING            4
    #define SDF_ON_EDGE_VALUE         128
    #define SDF_PIXEL_DIST_SCALE     64.0f

    #define BITMAP_ALPHA_THRESHOLD     80

    CharInfo *chars = NULL;

    // Load font data (including pixel data) from TTF file
    // NOTE: Loaded information should be enough to generate font image atlas,
    // using any packaging method
    FILE *fontFile = fopen(fileName, "rb");     // Load font file

    if (fontFile != NULL)
    {
        fseek(fontFile, 0, SEEK_END);
        long size = ftell(fontFile);    // Get file size
        fseek(fontFile, 0, SEEK_SET);   // Reset file pointer

        unsigned char *fontBuffer = (unsigned char *)RL_MALLOC(size);

        fread(fontBuffer, size, 1, fontFile);
        fclose(fontFile);

        // Init font for data reading
        stbtt_fontinfo fontInfo;
        if (!stbtt_InitFont(&fontInfo, fontBuffer, 0)) TraceLog(LOG_WARNING, "Failed to init font!");

        // Calculate font scale factor
        float scaleFactor = stbtt_ScaleForPixelHeight(&fontInfo, (float)fontSize);

        // Calculate font basic metrics
        // NOTE: ascent is equivalent to font baseline
        int ascent, descent, lineGap;
        stbtt_GetFontVMetrics(&fontInfo, &ascent, &descent, &lineGap);

        // In case no chars count provided, default to 95
        charsCount = (charsCount > 0)? charsCount : 95;

        // Fill fontChars in case not provided externally
        // NOTE: By default we fill charsCount consecutevely, starting at 32 (Space)
        int genFontChars = false;
        if (fontChars == NULL)
        {
            fontChars = (int *)RL_MALLOC(charsCount*sizeof(int));
            for (int i = 0; i < charsCount; i++) fontChars[i] = i + 32;
            genFontChars = true;
        }

        chars = (CharInfo *)RL_MALLOC(charsCount*sizeof(CharInfo));

        // NOTE: Using simple packaging, one char after another
        for (int i = 0; i < charsCount; i++)
        {
            int chw = 0, chh = 0;   // Character width and height (on generation)
            int ch = fontChars[i];  // Character value to get info for
            chars[i].value = ch;

            //  Render a unicode codepoint to a bitmap
            //      stbtt_GetCodepointBitmap()           -- allocates and returns a bitmap
            //      stbtt_GetCodepointBitmapBox()        -- how big the bitmap must be
            //      stbtt_MakeCodepointBitmap()          -- renders into bitmap you provide

            if (type != FONT_SDF) chars[i].image.data = stbtt_GetCodepointBitmap(&fontInfo, scaleFactor, scaleFactor, ch, &chw, &chh, &chars[i].offsetX, &chars[i].offsetY);
            else if (ch != 32) chars[i].image.data = stbtt_GetCodepointSDF(&fontInfo, scaleFactor, ch, SDF_CHAR_PADDING, SDF_ON_EDGE_VALUE, SDF_PIXEL_DIST_SCALE, &chw, &chh, &chars[i].offsetX, &chars[i].offsetY);
            else chars[i].image.data = NULL;

            if (type == FONT_BITMAP)
            {
                // Aliased bitmap (black & white) font generation, avoiding anti-aliasing
                // NOTE: For optimum results, bitmap font should be generated at base pixel size
                for (int p = 0; p < chw*chh; p++)
                {
                    if (((unsigned char *)chars[i].image.data)[p] < BITMAP_ALPHA_THRESHOLD) ((unsigned char *)chars[i].image.data)[p] = 0;
                    else ((unsigned char *)chars[i].image.data)[p] = 255;
                }
            }

            // Load characters images
            chars[i].image.width = chw;
            chars[i].image.height = chh;
            chars[i].image.mipmaps = 1;
            chars[i].image.format = UNCOMPRESSED_GRAYSCALE;

            chars[i].offsetY += (int)((float)ascent*scaleFactor);

            // Get bounding box for character (may be offset to account for chars that dip above or below the line)
            int chX1, chY1, chX2, chY2;
            stbtt_GetCodepointBitmapBox(&fontInfo, ch, scaleFactor, scaleFactor, &chX1, &chY1, &chX2, &chY2);

            TraceLog(LOG_DEBUG, "Character box measures: %i, %i, %i, %i", chX1, chY1, chX2 - chX1, chY2 - chY1);
            TraceLog(LOG_DEBUG, "Character offsetY: %i", (int)((float)ascent*scaleFactor) + chY1);

            stbtt_GetCodepointHMetrics(&fontInfo, ch, &chars[i].advanceX, NULL);
            chars[i].advanceX *= scaleFactor;
        }

        RL_FREE(fontBuffer);
        if (genFontChars) RL_FREE(fontChars);
    }
    else TraceLog(LOG_WARNING, "[%s] TTF file could not be opened", fileName);

    return chars;
}

// Generate image font atlas using chars info
// NOTE: Packing method: 0-Default, 1-Skyline
Image GenImageFontAtlas(const CharInfo *chars, Rectangle **charRecs, int charsCount, int fontSize, int padding, int packMethod)
{
    Image atlas = { 0 };

    *charRecs = NULL;

    // In case no chars count provided we suppose default of 95
    charsCount = (charsCount > 0)? charsCount : 95;

    // NOTE: Rectangles memory is loaded here!
    Rectangle *recs = (Rectangle *)RL_MALLOC(charsCount*sizeof(Rectangle));

    // Calculate image size based on required pixel area
    // NOTE 1: Image is forced to be squared and POT... very conservative!
    // NOTE 2: SDF font characters already contain an internal padding,
    // so image size would result bigger than default font type
    float requiredArea = 0;
    for (int i = 0; i < charsCount; i++) requiredArea += ((chars[i].image.width + 2*padding)*(chars[i].image.height + 2*padding));
    float guessSize = sqrtf(requiredArea)*1.3f;
    int imageSize = (int)powf(2, ceilf(logf((float)guessSize)/logf(2)));  // Calculate next POT

    atlas.width = imageSize;   // Atlas bitmap width
    atlas.height = imageSize;  // Atlas bitmap height
    atlas.data = (unsigned char *)RL_CALLOC(1, atlas.width*atlas.height);      // Create a bitmap to store characters (8 bpp)
    atlas.format = UNCOMPRESSED_GRAYSCALE;
    atlas.mipmaps = 1;

    // DEBUG: We can see padding in the generated image setting a gray background...
    //for (int i = 0; i < atlas.width*atlas.height; i++) ((unsigned char *)atlas.data)[i] = 100;

    if (packMethod == 0)   // Use basic packing algorythm
    {
        int offsetX = padding;
        int offsetY = padding;

        // NOTE: Using simple packaging, one char after another
        for (int i = 0; i < charsCount; i++)
        {
            // Copy pixel data from fc.data to atlas
            for (int y = 0; y < chars[i].image.height; y++)
            {
                for (int x = 0; x < chars[i].image.width; x++)
                {
                    ((unsigned char *)atlas.data)[(offsetY + y)*atlas.width + (offsetX + x)] = ((unsigned char *)chars[i].image.data)[y*chars[i].image.width + x];
                }
            }

            // Fill chars rectangles in atlas info
            recs[i].x = (float)offsetX;
            recs[i].y = (float)offsetY;
            recs[i].width = (float)chars[i].image.width;
            recs[i].height = (float)chars[i].image.height;

            // Move atlas position X for next character drawing
            offsetX += (chars[i].image.width + 2*padding);

            if (offsetX >= (atlas.width - chars[i].image.width - padding))
            {
                offsetX = padding;

                // NOTE: Be careful on offsetY for SDF fonts, by default SDF
                // use an internal padding of 4 pixels, it means char rectangle
                // height is bigger than fontSize, it could be up to (fontSize + 8)
                offsetY += (fontSize + 2*padding);

                if (offsetY > (atlas.height - fontSize - padding)) break;
            }
        }
    }
    else if (packMethod == 1)  // Use Skyline rect packing algorythm (stb_pack_rect)
    {
        TraceLog(LOG_DEBUG, "Using Skyline packing algorythm!");

        stbrp_context *context = (stbrp_context *)RL_MALLOC(sizeof(*context));
        stbrp_node *nodes = (stbrp_node *)RL_MALLOC(charsCount*sizeof(*nodes));

        stbrp_init_target(context, atlas.width, atlas.height, nodes, charsCount);
        stbrp_rect *rects = (stbrp_rect *)RL_MALLOC(charsCount*sizeof(stbrp_rect));

        // Fill rectangles for packaging
        for (int i = 0; i < charsCount; i++)
        {
            rects[i].id = i;
            rects[i].w = chars[i].image.width + 2*padding;
            rects[i].h = chars[i].image.height + 2*padding;
        }

        // Package rectangles into atlas
        stbrp_pack_rects(context, rects, charsCount);

        for (int i = 0; i < charsCount; i++)
        {
            // It return char rectangles in atlas
            recs[i].x = rects[i].x + (float)padding;
            recs[i].y = rects[i].y + (float)padding;
            recs[i].width = (float)chars[i].image.width;
            recs[i].height = (float)chars[i].image.height;

            if (rects[i].was_packed)
            {
                // Copy pixel data from fc.data to atlas
                for (int y = 0; y < chars[i].image.height; y++)
                {
                    for (int x = 0; x < chars[i].image.width; x++)
                    {
                        ((unsigned char *)atlas.data)[(rects[i].y + padding + y)*atlas.width + (rects[i].x + padding + x)] = ((unsigned char *)chars[i].image.data)[y*chars[i].image.width + x];
                    }
                }
            }
            else TraceLog(LOG_WARNING, "Character could not be packed: %i", i);
        }

        RL_FREE(rects);
        RL_FREE(nodes);
        RL_FREE(context);
    }

    // TODO: Crop image if required for smaller size

    // Convert image data from GRAYSCALE to GRAY_ALPHA
    // WARNING: ImageAlphaMask(&atlas, atlas) does not work in this case, requires manual operation
    unsigned char *dataGrayAlpha = (unsigned char *)RL_MALLOC(atlas.width*atlas.height*sizeof(unsigned char)*2); // Two channels

    for (int i = 0, k = 0; i < atlas.width*atlas.height; i++, k += 2)
    {
        dataGrayAlpha[k] = 255;
        dataGrayAlpha[k + 1] = ((unsigned char *)atlas.data)[i];
    }

    RL_FREE(atlas.data);
    atlas.data = dataGrayAlpha;
    atlas.format = UNCOMPRESSED_GRAY_ALPHA;

    *charRecs = recs;

    return atlas;
}

// Unload Font from GPU memory (VRAM)
void UnloadFont(Font font)
{
    // NOTE: Make sure spriteFont is not default font (fallback)
    if (font.texture.id != GetFontDefault().texture.id)
    {
        for (int i = 0; i < font.charsCount; i++) UnloadImage(font.chars[i].image);

        UnloadTexture(font.texture);
        RL_FREE(font.chars);
        RL_FREE(font.recs);

        TraceLog(LOG_DEBUG, "Unloaded sprite font data");
    }
}

// Draw text using Font
// NOTE: chars spacing is NOT proportional to fontSize
void DrawTextEx(Font font, const char *text, Vector2 position, float fontSize, float spacing, Color tint)
{
    int length = strlen(text);      // Total length in bytes of the text, scanned by codepoints in loop

    int textOffsetY = 0;            // Offset between lines (on line break '\n')
    float textOffsetX = 0.0f;       // Offset X to next character to draw
    
    float scaleFactor = fontSize/font.baseSize;     // Character quad scaling factor

    for (int i = 0; i < length; i++)
    {
        // Get next codepoint from byte string and glyph index in font
        int codepointByteCount = 0;
        int codepoint = GetNextCodepoint(&text[i], &codepointByteCount);
        int index = GetGlyphIndex(font, codepoint);

        // NOTE: Normally we exit the decoding sequence as soon as a bad byte is found (and return 0x3f)
        // but we need to draw all of the bad bytes using the '?' symbol moving one byte
        if (codepoint == 0x3f) codepointByteCount = 1;

        if (codepoint == '\n')
        {
            // NOTE: Fixed line spacing of 1.5 line-height
            // TODO: Support custom line spacing defined by user
            textOffsetY += (int)((font.baseSize + font.baseSize/2)*scaleFactor);
            textOffsetX = 0.0f;
        }
        else
        {
            if ((codepoint != ' ') && (codepoint != '\t')) 
            {
                Rectangle rec = { position.x + textOffsetX + font.chars[index].offsetX*scaleFactor,
                                  position.y + textOffsetY + font.chars[index].offsetY*scaleFactor, 
                                  font.recs[index].width*scaleFactor, 
                                  font.recs[index].height*scaleFactor };
    
                DrawTexturePro(font.texture, font.recs[index], rec, (Vector2){ 0, 0 }, 0.0f, tint);
            }

            if (font.chars[index].advanceX == 0) textOffsetX += ((float)font.recs[index].width*scaleFactor + spacing);
            else textOffsetX += ((float)font.chars[index].advanceX*scaleFactor + spacing);
        }
        
        i += (codepointByteCount - 1);   // Move text bytes counter to next codepoint
    }
}

// Measure string size for Font
Vector2 MeasureTextEx(Font font, const char *text, float fontSize, float spacing)
{
    int len = strlen(text);
    int tempLen = 0;                // Used to count longer text line num chars
    int lenCounter = 0;

    float textWidth = 0.0f;
    float tempTextWidth = 0.0f;     // Used to count longer text line width

    float textHeight = (float)font.baseSize;
    float scaleFactor = fontSize/(float)font.baseSize;

    int letter = 0;                 // Current character
    int index = 0;                  // Index position in sprite font

    for (int i = 0; i < len; i++)
    {
        lenCounter++;

        int next = 0;
        letter = GetNextCodepoint(&text[i], &next);
        index = GetGlyphIndex(font, letter);

        // NOTE: normally we exit the decoding sequence as soon as a bad byte is found (and return 0x3f)
        // but we need to draw all of the bad bytes using the '?' symbol so to not skip any we set next = 1
        if (letter == 0x3f) next = 1;
        i += next - 1;

        if (letter != '\n')
        {
            if (font.chars[index].advanceX != 0) textWidth += font.chars[index].advanceX;
            else textWidth += (font.recs[index].width + font.chars[index].offsetX);
        }
        else
        {
            if (tempTextWidth < textWidth) tempTextWidth = textWidth;
            lenCounter = 0;
            textWidth = 0;
            textHeight += ((float)font.baseSize*1.5f); // NOTE: Fixed line spacing of 1.5 lines
        }

        if (tempLen < lenCounter) tempLen = lenCounter;
    }

    if (tempTextWidth < textWidth) tempTextWidth = textWidth;

    Vector2 vec = { 0 };
    vec.x = tempTextWidth*scaleFactor + (float)((tempLen - 1)*spacing); // Adds chars spacing to measure
    vec.y = textHeight*scaleFactor;

    return vec;
}

// Returns index position for a unicode character on spritefont
int GetGlyphIndex(Font font, int codepoint)
{
#define TEXT_CHARACTER_NOTFOUND     63      // Character: '?'
    
#define UNORDERED_CHARSET
#if defined(UNORDERED_CHARSET)
    int index = TEXT_CHARACTER_NOTFOUND;

    for (int i = 0; i < font.charsCount; i++)
    {
        if (font.chars[i].value == codepoint)
        {
            index = i;
            break;
        }
    }

    return index;
#else
    return (codepoint - 32);
#endif
}

// Returns next codepoint in a UTF8 encoded text, scanning until '\0' is found
// When a invalid UTF8 byte is encountered we exit as soon as possible and a '?'(0x3f) codepoint is returned
// Total number of bytes processed are returned as a parameter
// NOTE: the standard says U+FFFD should be returned in case of errors
// but that character is not supported by the default font in raylib
// TODO: optimize this code for speed!!
int GetNextCodepoint(const char *text, int *bytesProcessed)
{
/*
    UTF8 specs from https://www.ietf.org/rfc/rfc3629.txt

    Char. number range  |        UTF-8 octet sequence
      (hexadecimal)    |              (binary)
    --------------------+---------------------------------------------
    0000 0000-0000 007F | 0xxxxxxx
    0000 0080-0000 07FF | 110xxxxx 10xxxxxx
    0000 0800-0000 FFFF | 1110xxxx 10xxxxxx 10xxxxxx
    0001 0000-0010 FFFF | 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
*/
    // NOTE: on decode errors we return as soon as possible

    int code = 0x3f;   // Codepoint (defaults to '?')
    int octet = (unsigned char)(text[0]); // The first UTF8 octet
    *bytesProcessed = 1;

    if (octet <= 0x7f)
    {
        // Only one octet (ASCII range x00-7F)
        code = text[0];
    }
    else if ((octet & 0xe0) == 0xc0)
    {
        // Two octets
        // [0]xC2-DF    [1]UTF8-tail(x80-BF)
        unsigned char octet1 = text[1];

        if ((octet1 == '\0') || ((octet1 >> 6) != 2)) { *bytesProcessed = 2; return code; } // Unexpected sequence

        if ((octet >= 0xc2) && (octet <= 0xdf))
        {
            code = ((octet & 0x1f) << 6) | (octet1 & 0x3f);
            *bytesProcessed = 2;
        }
    }
    else if ((octet & 0xf0) == 0xe0)
    {
        // Three octets
        unsigned char octet1 = text[1];
        unsigned char octet2 = '\0';

        if ((octet1 == '\0') || ((octet1 >> 6) != 2)) { *bytesProcessed = 2; return code; } // Unexpected sequence

        octet2 = text[2];

        if ((octet2 == '\0') || ((octet2 >> 6) != 2)) { *bytesProcessed = 3; return code; } // Unexpected sequence

        /*
            [0]xE0    [1]xA0-BF       [2]UTF8-tail(x80-BF)
            [0]xE1-EC [1]UTF8-tail    [2]UTF8-tail(x80-BF)
            [0]xED    [1]x80-9F       [2]UTF8-tail(x80-BF)
            [0]xEE-EF [1]UTF8-tail    [2]UTF8-tail(x80-BF)
        */

        if (((octet == 0xe0) && !((octet1 >= 0xa0) && (octet1 <= 0xbf))) ||
            ((octet == 0xed) && !((octet1 >= 0x80) && (octet1 <= 0x9f)))) { *bytesProcessed = 2; return code; }

        if ((octet >= 0xe0) && (0 <= 0xef))
        {
            code = ((octet & 0xf) << 12) | ((octet1 & 0x3f) << 6) | (octet2 & 0x3f);
            *bytesProcessed = 3;
        }
    }
    else if ((octet & 0xf8) == 0xf0)
    {
        // Four octets
        if (octet > 0xf4) return code;

        unsigned char octet1 = text[1];
        unsigned char octet2 = '\0';
        unsigned char octet3 = '\0';

        if ((octet1 == '\0') || ((octet1 >> 6) != 2)) { *bytesProcessed = 2; return code; }  // Unexpected sequence

        octet2 = text[2];

        if ((octet2 == '\0') || ((octet2 >> 6) != 2)) { *bytesProcessed = 3; return code; }  // Unexpected sequence

        octet3 = text[3];

        if ((octet3 == '\0') || ((octet3 >> 6) != 2)) { *bytesProcessed = 4; return code; }  // Unexpected sequence

        /*
            [0]xF0       [1]x90-BF       [2]UTF8-tail  [3]UTF8-tail
            [0]xF1-F3    [1]UTF8-tail    [2]UTF8-tail  [3]UTF8-tail
            [0]xF4       [1]x80-8F       [2]UTF8-tail  [3]UTF8-tail
        */

        if (((octet == 0xf0) && !((octet1 >= 0x90) && (octet1 <= 0xbf))) ||
            ((octet == 0xf4) && !((octet1 >= 0x80) && (octet1 <= 0x8f)))) { *bytesProcessed = 2; return code; } // Unexpected sequence

        if (octet >= 0xf0)
        {
            code = ((octet & 0x7) << 18) | ((octet1 & 0x3f) << 12) | ((octet2 & 0x3f) << 6) | (octet3 & 0x3f);
            *bytesProcessed = 4;
        }
    }

    if (code > 0x10ffff) code = 0x3f;     // Codepoints after U+10ffff are invalid

    return code;
}
