/**
 * Copyright (C) 2019 averne
 * Copyright (C) 2019 WerWolv
 * 
 * This file is part of EdiZon.
 * 
 * EdiZon is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 * 
 * EdiZon is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with EdiZon.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "overlay/color.hpp"

// Handheld and docked
#define SCREEN_WIDTH  1920
#define SCREEN_HEIGHT 1080

#define TS_WIDTH  1280
#define TS_HEIGHT 720

// Aligned width * height -> page-aligned (0x1000)
#define FB_WIDTH  448 // Aligned (*bpp) to 64
#define FB_HEIGHT 720 // Aligned to 128

#define LAYER_WIDTH  (SCREEN_HEIGHT * FB_WIDTH / FB_HEIGHT)
#define LAYER_HEIGHT SCREEN_HEIGHT
#define LAYER_X      0.0F
#define LAYER_Y      0.0F

#define TS_LAYER_WIDTH  (LAYER_WIDTH  * TS_WIDTH  / SCREEN_WIDTH)
#define TS_LAYER_HEIGHT (LAYER_HEIGHT * TS_HEIGHT / SCREEN_HEIGHT)
#define TS_LAYER_X      (LAYER_X      * TS_WIDTH  / SCREEN_WIDTH)
#define TS_LAYER_Y      (LAYER_Y      * TS_HEIGHT / SCREEN_HEIGHT)

#define BPP sizeof(rgb4444_t)

#define DEFAULT_FONT_HEIGHT 16
