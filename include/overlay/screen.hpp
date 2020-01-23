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

#pragma once

#include <switch.h>

#include <vector>
#include <string>
#include <unordered_map>

#include "overlay/constants.hpp"
#include "overlay/color.hpp"
#include "overlay/stb_truetype.h"

namespace tsl {
    
    class Screen {
    public:
        Screen();
        ~Screen();

        static Result initialize();
        static void exit();

        const inline u32 getWindowWidth()       const { return this->m_window.width; }
        const inline u32 getWindowHeight()      const { return this->m_window.height; }
        const inline u32 getWindowFormat()      const { return this->m_window.format; }
        const inline u32 getFramebufferWidth()  const { return this->m_frameBufferObject.width_aligned; }
        const inline u32 getFramebufferHeight() const { return this->m_frameBufferObject.height_aligned; }
        const inline u32 getFramebufferSize()   const { return this->m_frameBufferObject.fb_size; }
        const inline u32 getFramebufferStride() const { return this->m_frameBufferObject.stride; }

        inline void *getFramebufferAddress(u32 slot) const { return static_cast<u8*>(this->m_frameBufferObject.buf) + slot * this->m_frameBufferObject.fb_size; }
        inline u32 getCurFramebufferSlot()           const { return this->m_window.cur_slot; }
        inline u32 getNextFramebufferSlot()          const { return (getCurFramebufferSlot() + 1) % this->m_frameBufferObject.num_fbs; }

        void *getCurFramebuffer();
        inline void *getNextFramebuffer() const { return getFramebufferAddress(getNextFramebufferSlot()); }

        void flush();

        static inline Result waitForVsync()    { return eventWait(&Screen::s_vsyncEvent, U64_MAX); }

        inline const u16 getPixelRaw(u32 off)           { return static_cast<u16*>(getCurFramebuffer())[off]; }
        inline const u16 getPixelRaw(u32 x, u32 y)      { return getPixelRaw(getPixelOffset(x, y)); }
        inline const rgba4444_t getPixel(u32 off)       { return makeColor<rgba4444_t>(static_cast<u16*>(getCurFramebuffer())[off]); }
        inline const rgba4444_t getPixel(u32 x, u32 y)  { return getPixel(getPixelOffset(x, y)); }

        inline u8 blendColor(u32 src, u32 dst, u8 alpha) {
            u8 one_minus_alpha = static_cast<u8>(0xF) - alpha;

            return (dst*alpha + src*one_minus_alpha) / static_cast<float>(0xF);
        }

        inline void setPixelBlendSrc(u32 off, u16 color) {
            rgba4444_t src((static_cast<u16*>(getCurFramebuffer()))[off]);
            rgba4444_t dst(color);
            rgba4444_t end(0);

            end.r = blendColor(src.r, dst.r, dst.a);
            end.g = blendColor(src.g, dst.g, dst.a);
            end.b = blendColor(src.b, dst.b, dst.a);
            end.a = src.a;

            Screen::setPixel(off, end.rgba);
        }

        inline void setPixelBlendDst(u32 off, u16 color) {
            rgba4444_t src((static_cast<u16*>(getCurFramebuffer()))[off]);
            rgba4444_t dst(color);
            rgba4444_t end(0);

            end.r = blendColor(src.r, dst.r, dst.a);
            end.g = blendColor(src.g, dst.g, dst.a);
            end.b = blendColor(src.b, dst.b, dst.a);
            end.a = dst.a;

            Screen::setPixel(off, end.rgba);
        }

        inline void setPixel(u32 x, u32 y, u16 color) {
            setPixel(getPixelOffset(x, y), color);
        }

        inline void setPixel(u32 off, u16 color)                       { static_cast<u16*>(getCurFramebuffer())[off] = color; }
        inline void setPixel(u32 off, rgba4444_t color)                { setPixel(off, color.rgba); }
        inline void setPixel(u32 x, u32 y, rgba4444_t color)           { setPixel(getPixelOffset(x, y), color); }
        inline void setPixel(u32 off, rgba4444_t color, u8 alpha)      { setPixel(off, blend(color, getPixel(off), alpha).rgba); }
        inline void setPixel(u32 x, u32 y, rgba4444_t color, u8 alpha) { setPixel(getPixelOffset(x, y), color, alpha); }

        inline void setPixelBlendSrc(u32 x, u32 y, u16 color)                  { setPixelBlendSrc(getPixelOffset(x, y), color); }
        inline void setPixelBlendSrc(u32 off, rgba4444_t color)                { setPixelBlendSrc(off, color.rgba); }
        inline void setPixelBlendSrc(u32 x, u32 y, rgba4444_t color)           { setPixelBlendSrc(getPixelOffset(x, y), color); }
        inline void setPixelBlendSrc(u32 off, rgba4444_t color, u8 alpha)      { setPixelBlendSrc(off, blend(color, getPixel(off), alpha).rgba); }
        inline void setPixelBlendSrc(u32 x, u32 y, rgba4444_t color, u8 alpha) { setPixelBlendSrc(getPixelOffset(x, y), color, alpha); }

        inline void setPixelBlendDst(u32 x, u32 y, u16 color)                  { setPixelBlendDst(getPixelOffset(x, y), color); }
        inline void setPixelBlendDst(u32 off, rgba4444_t color)                { setPixelBlendDst(off, color.rgba); }
        inline void setPixelBlendDst(u32 x, u32 y, rgba4444_t color)           { setPixelBlendDst(getPixelOffset(x, y), color); }
        inline void setPixelBlendDst(u32 off, rgba4444_t color, u8 alpha)      { setPixelBlendDst(off, blend(color, getPixel(off), alpha).rgba); }
        inline void setPixelBlendDst(u32 x, u32 y, rgba4444_t color, u8 alpha) { setPixelBlendDst(getPixelOffset(x, y), color, alpha); }

        void clear();
        void fillScreen(u16 color);
        void fillScreen(rgba4444_t color);
        void drawRect(s32 x, s32 y, s32 w, s32 h, u16 color);
        void drawRect(s32 x, s32 y, s32 w, s32 h, rgba4444_t color);
        void mapArea(s32 x1, s32 y1, s32 x2, s32 y2, u16 *area);
        void mapArea(s32 x1, s32 y1, s32 x2, s32 y2, u8 *area);
        void mapArea(s32 x1, s32 y1, s32 x2, s32 y2, rgba4444_t *area);

        void drawRGBA8Image(s32 x, s32 y, s32 w, s32 h, const u8 *bmp);
        
        Result initFont();
        void drawGlyph(u32 codepoint, u32 x, u32 y, rgba4444_t color, stbtt_fontinfo *font, float fontSize);
        void drawString(const char *string, bool monospace, u32 x, u32 y, float fontSize, rgba4444_t color);

        float getOpacity() { return this->s_opacity; }
        void setOpacity(float opacity)  { this->s_opacity = opacity; }

        static rgba4444_t a(const rgba4444_t &c) {
            return (c.rgba & 0x0FFF) | (static_cast<u8>(c.a * Screen::s_opacity) << 12);
        }

        inline const u32 getPixelOffset(u32 x, u32 y) const {
            if (x > FB_WIDTH || y > FB_HEIGHT)
                return 0;

            if (Screen::s_scissoringEnabled) {
                if (x < Screen::s_scissorBounds[0] ||
                    y < Screen::s_scissorBounds[1] ||
                    x > Screen::s_scissorBounds[0] + Screen::s_scissorBounds[2] ||
                    y > Screen::s_scissorBounds[1] + Screen::s_scissorBounds[3])
                        return FB_WIDTH * FB_HEIGHT * 2 + 1;
            }

            u32 tmp_pos = ((y & 127) / 16) + (x / 32 * 8) + (( y / 16 / 8) * (((FB_WIDTH / 2) / 16 * 8)));
            tmp_pos *= 16 * 16 * 4;

            tmp_pos += ((y % 16) / 8) * 512 + ((x % 32) / 16) * 256 + ((y % 8) / 2) * 64 + ((x % 16) / 8) * 32 + (y % 2) * 16 + (x % 8) * 2;
            
            return tmp_pos / 2;
        }

        void enableScissoring(u16 x, u16 y, u16 w, u16 h) {
            Screen::s_scissoringEnabled = true;

            Screen::s_scissorBounds[0] = x;
            Screen::s_scissorBounds[1] = y;
            Screen::s_scissorBounds[2] = w;
            Screen::s_scissorBounds[3] = h;
        }

        void disableScissoring() {
            Screen::s_scissoringEnabled = false;
        }

    private:
        static inline ViDisplay s_display;
        static inline Event s_vsyncEvent;
        static inline PlFontData s_fontStd, s_fontExt;
        static inline stbtt_fontinfo s_stbFontStd, s_stbFontExt, s_stbFontMaterial;

        static inline float s_opacity = 0.0F;
        static inline bool s_scissoringEnabled;
        static inline u16 s_scissorBounds[4];

        ViLayer m_layer;
        NWindow m_window;
        Framebuffer m_frameBufferObject;

        void *m_frameBuffer = nullptr;
    };

}