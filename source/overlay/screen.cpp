/**
 * Copyright (C) 2019 averne
 * Copyright (C) 2020 WerWolv
 * 
 * This file is part of libtesla.
 * 
 * libtesla is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 * 
 * libtesla is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with libtesla.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <cstdlib>
#include <switch.h>
#include <cstring>
#include <algorithm>

#include "overlay/constants.hpp"
#include "overlay/color.hpp"
#include "helpers/vi_shim.hpp"

#include "overlay/screen.hpp"


#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#include "overlay/stb_truetype.h"

extern "C" u64 __nx_vi_layer_id;

namespace tsl {

    Result Screen::initialize() {  
        Result res;

        if(R_FAILED(res = viInitialize(ViServiceType_Manager)))
            fatalThrow(res);
        if(R_FAILED(res = viOpenDefaultDisplay(&Screen::s_display)))
            fatalThrow(res);
        if(R_FAILED(res = viGetDisplayVsyncEvent(&Screen::s_display, &Screen::s_vsyncEvent)))
            fatalThrow(res);

        return 0;
    }

    void Screen::exit() {
        eventClose(&Screen::s_vsyncEvent);
        viCloseDisplay(&Screen::s_display);
        viExit();
    }

    Screen::Screen() {
        Result res = 0;
        u64 layerId = 0;
        s32 layerZ = 0;
        
        smInitialize();

        if (R_FAILED(res = viCreateManagedLayer(&Screen::s_display, (ViLayerFlags)0, 0, &__nx_vi_layer_id)))
            goto fatal;

        if (R_FAILED(res = viCreateLayer(&Screen::s_display, &this->m_layer)))
            goto closeManagedLayer;

        if (R_FAILED(res = viSetLayerScalingMode(&this->m_layer, ViScalingMode_PreserveAspectRatio)))
            goto closeLayer;

        if (R_SUCCEEDED(res = viGetZOrderCountMax(&Screen::s_display, &layerZ)) && (layerZ > 0))
            if (R_FAILED(res = viSetLayerZ(&this->m_layer, layerZ)))
                goto closeLayer;

        if (R_FAILED(res = viSetLayerSize(&this->m_layer, LAYER_WIDTH, LAYER_HEIGHT)))
            goto closeLayer;

        if (R_FAILED(res = viSetLayerPosition(&this->m_layer, LAYER_X, LAYER_Y)))
            goto closeLayer;

        if (R_FAILED(res = nwindowCreateFromLayer(&this->m_window, &this->m_layer)))
            goto closeLayer;

        if (R_FAILED(res = framebufferCreate(&this->m_frameBufferObject, &this->m_window, FB_WIDTH, FB_HEIGHT, PIXEL_FORMAT_RGBA_4444, 2)))
            goto closeWindow;

        if (R_FAILED(initFont()))
            goto closeWindow;

        smExit();
        return;

        {

            closeWindow:
                nwindowClose(&this->m_window);
            closeLayer:
                layerId = this->m_layer.layer_id;
                viCloseLayer(&this->m_layer);
            closeManagedLayer:
                layerId = (layerId == 0) ? this->m_layer.layer_id : layerId;
                viDestroyManagedLayer(layerId);
            fatal:
                Screen::exit();
                smExit();
                fatalThrow(res);
        }
    }

    Screen::~Screen() {
        Screen::flush();
        u64 layerId = this->m_layer.layer_id;

        framebufferClose(&this->m_frameBufferObject);
        nwindowClose(&this->m_window);
        viCloseLayer(&this->m_layer);
        viDestroyManagedLayer(layerId);
    }

    Result Screen::initFont() {
        Result res;
        // Nintendo's default font
        if(R_FAILED(res = plGetSharedFontByType(&Screen::s_fontStd, PlSharedFontType_Standard)))
            return res;

        u8 *fontBuffer = reinterpret_cast<u8*>(Screen::s_fontStd.address);
        stbtt_InitFont(&Screen::s_stbFontStd, fontBuffer, stbtt_GetFontOffsetForIndex(fontBuffer, 0));
        
        // Nintendo's extended font containing a bunch of icons
        if(R_FAILED(res = plGetSharedFontByType(&Screen::s_fontExt, PlSharedFontType_NintendoExt)))
            return res;

        fontBuffer = reinterpret_cast<u8*>(Screen::s_fontExt.address);
        stbtt_InitFont(&Screen::s_stbFontExt, fontBuffer, stbtt_GetFontOffsetForIndex(fontBuffer, 0));

        return 0;
    }


    void *Screen::getCurFramebuffer() {
        if (!this->m_frameBuffer)
            this->m_frameBuffer = framebufferBegin(&this->m_frameBufferObject, NULL);
        return this->m_frameBuffer;
    }

    void Screen::flush() {
        std::memcpy(getNextFramebuffer(), getCurFramebuffer(), getFramebufferSize());
        Screen::waitForVsync();

        framebufferEnd(&this->m_frameBufferObject);
        this->m_frameBuffer = nullptr;
    }


    void Screen::clear() {
        Screen::fillScreen(0x0000);
    }

    void Screen::fillScreen(u16 color) {
        std::fill(static_cast<u16*>(getCurFramebuffer()), reinterpret_cast<u16*>((static_cast<u8*>(getCurFramebuffer()) + getFramebufferSize())), color);
    }

    void Screen::fillScreen(rgba4444_t color) {
        Screen::fillScreen(color.rgba);
    }


    void Screen::drawRect(s32 x, s32 y, s32 w, s32 h, u16 color) {
        for (s32 x1 = x; x1 < (x + w); x1++)
            for (s32 y1 = y; y1 < (y + h); y1++)
                Screen::setPixelBlendDst(x1, y1, color);
    }

    void Screen::drawRect(s32 x, s32 y, s32 w, s32 h, rgba4444_t color) {
        for (s32 x1 = x; x1 < (x + w); x1++)
            for (s32 y1 = y; y1 < (y + h); y1++)
                Screen::setPixelBlendDst(x1, y1, color.rgba);
    }


    void Screen::mapArea(s32 x1, s32 y1, s32 x2, s32 y2, u16 *area) {
        for (s32 y = y1; y < y2; y++) {
            for (s32 x = x1; x < x2; x++)
                Screen::setPixel(x, y, *(area++));
        }
    }

    void Screen::mapArea(s32 x1, s32 y1, s32 x2, s32 y2, rgba4444_t *area) {
        for (s32 y = y1; y < y2; y++)
            for (s32 x = x1; x < x2; x++)
                Screen::setPixel(x, y, (*(area++)).rgba);
    }

    void Screen::drawRGBA8Image(s32 x, s32 y, s32 w, s32 h, const u8 *bmp) {
        for (s32 y1 = 0; y1 < h; y1++)
            for (s32 x1 = 0; x1 < w; x1++) {
                const rgba4444_t color = { static_cast<u8>(bmp[0] >> 4), static_cast<u8>(bmp[1] >> 4), static_cast<u8>(bmp[2] >> 4), static_cast<u8>(bmp[3] >> 4) };
                setPixelBlendSrc(x + x1, y + y1, a(color));
                bmp += 4;
            }
    }


    void Screen::drawGlyph(u32 codepoint, u32 x, u32 y, rgba4444_t color, stbtt_fontinfo *font, float fontSize) {
        int width = 0, height = 0;

        u8 *glyphBmp = stbtt_GetCodepointBitmap(font, fontSize, fontSize, codepoint, &width, &height, nullptr, nullptr);
        
        for (s16 bmpY = 0; bmpY < height; bmpY++)
            for (s16 bmpX = 0; bmpX < width; bmpX++) {
                rgba4444_t tmpColor = color;
                tmpColor.a = (glyphBmp[width * bmpY + bmpX] >> 4) * (static_cast<float>(tmpColor.a) / 0xF);
                Screen::setPixelBlendSrc(x + bmpX, y + bmpY, tmpColor);
            }

        std::free(glyphBmp);
    }

    void Screen::drawString(const char *string, bool monospace, u32 x, u32 y, float fontSize, rgba4444_t color) {
        const size_t stringLength = std::strlen(string);

        u32 currX = x;
        u32 currY = y;
        u32 prevCharacter = 0;

        u32 i = 0;
        do {
            u32 currCharacter;
            ssize_t codepointWidth = decode_utf8(&currCharacter, reinterpret_cast<const u8*>(string + i));

            if (codepointWidth <= 0)
                break;

            i += codepointWidth;

            stbtt_fontinfo *currFont;
            float currFontSize = fontSize;

            if (stbtt_FindGlyphIndex(&Screen::s_stbFontStd, currCharacter)) {
                currFont = &Screen::s_stbFontStd;
                currFontSize /= 1000;
            }
            else if (stbtt_FindGlyphIndex(&Screen::s_stbFontExt, currCharacter)) {
                currFont = &Screen::s_stbFontExt;
                currFontSize /= 1000;
            }
            else if (stbtt_FindGlyphIndex(&Screen::s_stbFontMaterial, currCharacter)) {
                currFont = &Screen::s_stbFontMaterial;
                currFontSize /= 500;
            }
            else return;

            currX += currFontSize * stbtt_GetCodepointKernAdvance(currFont, prevCharacter, currCharacter);

            if (currCharacter == '\n') {
                currX = x;

                int ascent = 0;
                stbtt_GetFontVMetrics(currFont, &ascent, nullptr, nullptr);
                currY += ascent * currFontSize * 1.125F;
                continue;
            }

            int bounds[4] = { 0 };
            stbtt_GetCodepointBitmapBoxSubpixel(currFont, currCharacter, currFontSize, currFontSize,
                                                0, 0, &bounds[0], &bounds[1], &bounds[2], &bounds[3]);

            int xAdvance;
            stbtt_GetCodepointHMetrics(currFont, currCharacter, &xAdvance, nullptr);

            Screen::drawGlyph(currCharacter, currX + bounds[0], currY + bounds[1], color, currFont, currFontSize);

            currX += xAdvance * currFontSize;
            
        } while (i < stringLength);
    } 

}
