/**********************************************************************************************
*
*   raylib configuration flags
*
*   This file defines all the configuration flags for the different raylib modules
*
*   LICENSE: zlib/libpng
*
*   Copyright (c) 2018-2019 Ahmad Fatoum & Ramon Santamaria (@raysan5)
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

#define RAYLIB_VERSION  "2.6-dev"

//------------------------------------------------------------------------------------
// Module: core - Configuration Flags
//------------------------------------------------------------------------------------
// Reconfigure standard input to receive key inputs, works with SSH connection.
#define SUPPORT_SSH_KEYBOARD_RPI    1
// Draw a mouse reference on screen (square cursor box)
#define SUPPORT_MOUSE_CURSOR_RPI    1
// Use busy wait loop for timing sync, if not defined, a high-resolution timer is setup and used
//#define SUPPORT_BUSY_WAIT_LOOP      1
// Wait for events passively (sleeping while no events) instead of polling them actively every frame
//#define SUPPORT_EVENTS_WAITING      1
// Allow scale all the drawn content to match the high-DPI equivalent size (only PLATFORM_DESKTOP)
//#define SUPPORT_HIGH_DPI            1
//
//#define SUPPORT_GAMEPAD


//------------------------------------------------------------------------------------
// Module: shapes - Configuration Flags
//------------------------------------------------------------------------------------
// Draw rectangle shapes using font texture white character instead of default white texture
// Allows drawing rectangles and text with a single draw call, very useful for GUI systems!
#define SUPPORT_FONT_TEXTURE        1
// Use QUADS instead of TRIANGLES for drawing when possible
// Some lines-based shapes could still use lines
#define SUPPORT_QUADS_DRAW_MODE     1

//------------------------------------------------------------------------------------
// Module: textures - Configuration Flags
//------------------------------------------------------------------------------------
// Selecte desired fileformats to be supported for image data loading
#define SUPPORT_FILEFORMAT_PNG      1
#define SUPPORT_FILEFORMAT_BMP    1
#define SUPPORT_FILEFORMAT_JPG    1

// Support multiple image editing functions to scale, adjust colors, flip, draw on images, crop...
// If not defined only three image editing functions supported: ImageFormat(), ImageAlphaMask(), ImageToPOT()
#define SUPPORT_IMAGE_MANIPULATION  1

//------------------------------------------------------------------------------------
// Module: text - Configuration Flags
//------------------------------------------------------------------------------------
// Default font is loaded on window initialization to be available for the user to render simple text
// NOTE: If enabled, uses external module functions to load default raylib font
#define SUPPORT_DEFAULT_FONT        1
// Selected desired font fileformats to be supported for loading
#define SUPPORT_FILEFORMAT_FNT      1
#define SUPPORT_FILEFORMAT_TTF      1

//------------------------------------------------------------------------------------
// Module: audio - Configuration Flags
//------------------------------------------------------------------------------------
// Desired audio fileformats to be supported for loading
#define SUPPORT_FILEFORMAT_WAV      1
#define SUPPORT_FILEFORMAT_OGG      1
#define SUPPORT_FILEFORMAT_XM       1
#define SUPPORT_FILEFORMAT_MOD      1
#define SUPPORT_FILEFORMAT_FLAC     1
#define SUPPORT_FILEFORMAT_MP3      1


//------------------------------------------------------------------------------------
// Module: utils - Configuration Flags
//------------------------------------------------------------------------------------
// Show TraceLog() output messages
// NOTE: By default LOG_DEBUG traces not shown
#define SUPPORT_TRACELOG    1
