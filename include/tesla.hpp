/**
 * Copyright (C) 2020 werwolv
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

#include <stdlib.h>
#include <strings.h>

#include <algorithm>
#include <cstring>
#include <cwctype>
#include <string>
#include <functional>
#include <type_traits>
#include <mutex>
#include <memory>
#include <chrono>
#include <map>


// Define this makro before including tesla.hpp in your main file. If you intend
// to use the tesla.hpp header in more than one source file, only define it once!
// #define TESLA_INIT_IMPL

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

#ifdef TESLA_INIT_IMPL
    #define STB_TRUETYPE_IMPLEMENTATION
#endif
#include "stb_truetype.h"

#pragma GCC diagnostic pop


#define ASSERT_EXIT(x) if (R_FAILED(x)) std::exit(1)
#define ASSERT_FATAL(x) if (Result res = x; R_FAILED(res)) fatalThrow(res)

using namespace std::literals::chrono_literals;

namespace tsl {

    // Constants

    namespace cfg {

        constexpr u32 ScreenWidth = 1920;
        constexpr u32 ScreenHeight = 1080;

        static u16 LayerWidth  = 0;
        static u16 LayerHeight = 0;
        static u16 LayerPosX   = 0;
        static u16 LayerPosY   = 0;
        static u16 FramebufferWidth  = 0;
        static u16 FramebufferHeight = 0;

    }

    namespace style {
        constexpr u32 ListItemDefaultHeight = 72;
    }

    // Declarations
    enum class FocusDirection {
        None,
        Up,
        Down,
        Left,
        Right
    };

    template <typename, typename>
    class Overlay;

    

    namespace impl { enum class LaunchFlags : u8; }


    // Helpers

    namespace hlp {

        template<auto> struct dependent_false : std::false_type { };
        struct OverlayBase { };

        void doWithSmSession(std::function<void()> f) {
            smInitialize();
            f();
            smExit();
        }

        Result hidsysEnableAppletToGetInput(bool enable, u64 aruid) {  
            const struct {
                u8 permitInput;
                u64 appletResourceUserId;
            } in = { enable != 0, aruid };

            return serviceDispatchIn(hidsysGetServiceSession(), 503, in);
        }

        void requestForground(bool enabled) {
            u64 applicationAruid = 0, appletAruid = 0;

            for (u64 programId = 0x0100000000001000ul; programId < 0x0100000000001020ul; programId++) {
                pmdmntGetProcessId(&appletAruid, programId);
                
                if (appletAruid != 0)
                    hidsysEnableAppletToGetInput(!enabled, appletAruid);
            }

            pmdmntGetApplicationProcessId(&applicationAruid);
            hidsysEnableAppletToGetInput(!enabled, applicationAruid);

            hidsysEnableAppletToGetInput(true, 0);
        }

        std::vector<std::string> split(const std::string& str, char delim = ' ') {
            std::vector<std::string> out;

            std::size_t current, previous = 0;
            current = str.find(delim);
            while (current != std::string::npos) {
                out.push_back(str.substr(previous, current - previous));
                previous = current + 1;
                current = str.find(delim, previous);
            }
            out.push_back(str.substr(previous, current - previous));

            return out;
        }

        using IniData = std::map<std::string, std::map<std::string, std::string>>;

        IniData parseIni(std::string &str) {
            IniData iniData;
            
            auto lines = split(str, '\n');

            std::string lastHeader = "";
            for (auto& line : lines) {
                line.erase(std::remove_if(line.begin(), line.end(), ::isspace), line.end());

                if (line[0] == '[' && line[line.size() - 1] == ']') {
                    lastHeader = line.substr(1, line.size() - 2);
                    iniData.emplace(lastHeader, std::map<std::string, std::string>{});
                }
                else if (auto keyValuePair = split(line, '='); keyValuePair.size() == 2) {
                    iniData[lastHeader].emplace(keyValuePair[0], keyValuePair[1]);
                }
            }

            return iniData;
        }

        u64 stringToKeyCode(std::string &value) {
            if (strcasecmp(value.c_str(), "A")           == 0)
                return KEY_A;
            else if (strcasecmp(value.c_str(), "B")      == 0)
                return KEY_B;
            else if (strcasecmp(value.c_str(), "X")      == 0)
                return KEY_X;
            else if (strcasecmp(value.c_str(), "Y")      == 0)
                return KEY_Y;
            else if (strcasecmp(value.c_str(), "LS")     == 0)
                return KEY_LSTICK;
            else if (strcasecmp(value.c_str(), "RS")     == 0)
                return KEY_RSTICK;
            else if (strcasecmp(value.c_str(), "L")      == 0)
                return KEY_L;
            else if (strcasecmp(value.c_str(), "R")      == 0)
                return KEY_R;
            else if (strcasecmp(value.c_str(), "ZL")     == 0)
                return KEY_ZL;
            else if (strcasecmp(value.c_str(), "ZR")     == 0)
                return KEY_ZR;
            else if (strcasecmp(value.c_str(), "PLUS")   == 0)
                return KEY_PLUS;
            else if (strcasecmp(value.c_str(), "MINUS")  == 0)
                return KEY_MINUS;
            else if (strcasecmp(value.c_str(), "DLEFT")  == 0)
                return KEY_DLEFT;
            else if (strcasecmp(value.c_str(), "DUP")    == 0)
                return KEY_DUP;
            else if (strcasecmp(value.c_str(), "DRIGHT") == 0)
                return KEY_DRIGHT;
            else if (strcasecmp(value.c_str(), "DDOWN")  == 0)
                return KEY_DDOWN;
            else if (strcasecmp(value.c_str(), "SL")     == 0)
                return KEY_SL;
            else if (strcasecmp(value.c_str(), "SR")     == 0)
                return KEY_SR;
            else return 0;
        }

    }

    // Renderer

    namespace gfx {

        extern "C" u64 __nx_vi_layer_id;

        struct Color {

            union {
                struct {
                    u16 r: 4, g: 4, b: 4, a: 4;
                } PACKED;
                u16 rgba;
            };

            inline Color(u16 raw): rgba(raw) {}
            inline Color(u8 r, u8 g, u8 b, u8 a): r(r), g(g), b(b), a(a) {}
        };

        class Renderer final {
        public:
            Renderer& operator=(Renderer&) = delete;

            template <typename>
            friend class tsl::Overlay;

            static Color a(const Color &c) {
                return (c.rgba & 0x0FFF) | (static_cast<u8>(c.a * Renderer::s_opacity) << 12);
            }

            void init() {

                cfg::LayerPosX = 0;
                cfg::LayerPosY = 0;
                cfg::FramebufferWidth  = 448;
                cfg::FramebufferHeight = 720;
                cfg::LayerWidth  = cfg::ScreenHeight * (float(cfg::FramebufferWidth) / float(cfg::FramebufferHeight));
                cfg::LayerHeight = cfg::ScreenHeight;

                if (this->m_initialized)
                    return;

                tsl::hlp::doWithSmSession([this]{
                    ASSERT_FATAL(viInitialize(ViServiceType_Manager));
                    ASSERT_FATAL(viOpenDefaultDisplay(&this->m_display));
                    ASSERT_FATAL(viGetDisplayVsyncEvent(&this->m_display, &this->m_vsyncEvent));
                    ASSERT_FATAL(viCreateManagedLayer(&this->m_display, static_cast<ViLayerFlags>(0), 0, &__nx_vi_layer_id));
                    ASSERT_FATAL(viCreateLayer(&this->m_display, &this->m_layer));
                    ASSERT_FATAL(viSetLayerScalingMode(&this->m_layer, ViScalingMode_PreserveAspectRatio));

                    if (s32 layerZ = 0; R_SUCCEEDED(viGetZOrderCountMax(&this->m_display, &layerZ)) && layerZ > 0)
                        ASSERT_FATAL(viSetLayerZ(&this->m_layer, layerZ));

                    ASSERT_FATAL(viSetLayerSize(&this->m_layer, cfg::LayerWidth, cfg::LayerHeight));
                    ASSERT_FATAL(viSetLayerPosition(&this->m_layer, cfg::LayerPosX, cfg::LayerPosY));
                    ASSERT_FATAL(nwindowCreateFromLayer(&this->m_window, &this->m_layer));
                    ASSERT_FATAL(framebufferCreate(&this->m_framebuffer, &this->m_window, cfg::FramebufferWidth, cfg::FramebufferHeight, PIXEL_FORMAT_RGBA_4444, 2));
                    ASSERT_FATAL(this->initFonts());
                });

                this->m_initialized = true;
            }

            void exit() {
                if (!this->m_initialized)
                    return;

                ViLayer layerCopy = this->m_layer;

                framebufferClose(&this->m_framebuffer);
                nwindowClose(&this->m_window);
                viCloseLayer(&this->m_layer);
                viDestroyManagedLayer(&layerCopy); // Copy is required because viCloseLayer wipes the passed layer object
                viExit();
            }

            void startFrame() {
                this->m_currentFramebuffer = framebufferBegin(&this->m_framebuffer, nullptr);
            }

            void endFrame() {
                std::memcpy(this->getNextFramebuffer(), this->getCurrentFramebuffer(), this->getFramebufferSize());
                this->waitForVSync();
                framebufferEnd(&this->m_framebuffer);

                this->m_currentFramebuffer = nullptr;
            }

            void enableScissoring(u16 x, u16 y, u16 w, u16 h) {
                this->m_scissoring = true;

                this->m_scissorBounds[0] = x;
                this->m_scissorBounds[1] = y;
                this->m_scissorBounds[2] = w;
                this->m_scissorBounds[3] = h;
            }

            void disableScissoring() {
                this->m_scissoring = false;
            }


            // Drawing functions
            inline void setPixel(u16 x, u16 y, Color color) {
                if (x >= cfg::FramebufferWidth || y >= cfg::FramebufferHeight)
                    return;

                static_cast<Color*>(this->getCurrentFramebuffer())[this->getPixelOffset(x, y)] = color;
            }

            inline u8 blendColor(u8 src, u8 dst, u8 alpha) {
                u8 oneMinusAlpha = 0x0F - alpha;

                return (dst * alpha + src * oneMinusAlpha) / float(0xF);
            }

            inline void setPixelBlendSrc(u16 x, u16 y, Color color) {
                if (x >= cfg::FramebufferWidth || y >= cfg::FramebufferHeight)
                    return;

                Color src((static_cast<u16*>(this->getCurrentFramebuffer()))[this->getPixelOffset(x, y)]);
                Color dst(color);
                Color end(0);

                end.r = this->blendColor(src.r, dst.r, dst.a);
                end.g = this->blendColor(src.g, dst.g, dst.a);
                end.b = this->blendColor(src.b, dst.b, dst.a);
                end.a = src.a;

                this->setPixel(x, y, end);
            }

            inline void setPixelBlendDst(u16 x, u16 y, Color color) {
                if (x >= cfg::FramebufferWidth || y >= cfg::FramebufferHeight)
                    return;

                Color src((static_cast<u16*>(this->getCurrentFramebuffer()))[this->getPixelOffset(x, y)]);
                Color dst(color);
                Color end(0);

                end.r = this->blendColor(src.r, dst.r, dst.a);
                end.g = this->blendColor(src.g, dst.g, dst.a);
                end.b = this->blendColor(src.b, dst.b, dst.a);
                end.a = dst.a;

                this->setPixel(x, y, end);
            }

            inline void drawRect(u16 x, u16 y, u16 w, u16 h, Color color) {
                for (s16 x1 = x; x1 < (x + w); x1++)
                    for (s16 y1 = y; y1 < (y + h); y1++)
                        this->setPixelBlendDst(x1, y1, color);
            }

            inline void fillScreen(Color color) {
                std::fill_n(static_cast<Color*>(this->getCurrentFramebuffer()), this->getFramebufferSize() / sizeof(Color), color);
            }

            inline void clearScreen() {
                this->fillScreen({ 0x00, 0x00, 0x00, 0x00 });
            }


            inline Result initFonts() {
                Result res;

                static PlFontData stdFontData, extFontData;

                // Nintendo's default font
                if(R_FAILED(res = plGetSharedFontByType(&stdFontData, PlSharedFontType_Standard)))
                    return res;

                u8 *fontBuffer = reinterpret_cast<u8*>(stdFontData.address);
                stbtt_InitFont(&this->m_stdFont, fontBuffer, stbtt_GetFontOffsetForIndex(fontBuffer, 0));
                
                // Nintendo's extended font containing a bunch of icons
                if(R_FAILED(res = plGetSharedFontByType(&extFontData, PlSharedFontType_NintendoExt)))
                    return res;

                fontBuffer = reinterpret_cast<u8*>(extFontData.address);
                stbtt_InitFont(&this->m_extFont, fontBuffer, stbtt_GetFontOffsetForIndex(fontBuffer, 0));

                return res;
            }

            inline void drawGlyph(s32 codepoint, u32 x, u32 y, Color color, stbtt_fontinfo *font, float fontSize) {
                int width = 10, height = 10;

                u8 *glyphBmp = stbtt_GetCodepointBitmap(font, fontSize, fontSize, codepoint, &width, &height, nullptr, nullptr);
                
                if (glyphBmp == nullptr)
                    return;

                for (s16 bmpY = 0; bmpY < height; bmpY++) {
                    for (s16 bmpX = 0; bmpX < width; bmpX++) {
                        Color tmpColor = color;
                        tmpColor.a = (glyphBmp[width * bmpY + bmpX] >> 4) * (float(tmpColor.a) / 0xF);
                        this->setPixelBlendSrc(x + bmpX, y + bmpY, tmpColor);
                    }
                }

                std::free(glyphBmp);

            }

            inline std::pair<u32, u32> getStringBounds(const char* string, bool monospace, float fontSize) {
                const size_t stringLength = strlen(string);

                u32 maxX = 0;

                u32 currX = 0;
                u32 currY = 0;
                u32 prevCharacter = 0;

                u32 i = 0;

                do {
                    u32 currCharacter;
                    ssize_t codepointWidth = decode_utf8(&currCharacter, reinterpret_cast<const u8*>(string + i));

                    if (codepointWidth <= 0)
                        break;

                    i += codepointWidth;

                    stbtt_fontinfo *currFont = nullptr;

                    if (stbtt_FindGlyphIndex(&this->m_extFont, currCharacter))
                        currFont = &this->m_extFont;
                    else
                        currFont = &this->m_stdFont;

                    float currFontSize = stbtt_ScaleForPixelHeight(currFont, fontSize);
                    currX += currFontSize * stbtt_GetCodepointKernAdvance(currFont, prevCharacter, currCharacter);

                    int xAdvance = 0;
                    stbtt_GetCodepointHMetrics(currFont, monospace ? 'A' : currCharacter, &xAdvance, nullptr);

                    if (currCharacter == '\n') {
                        if (currX > maxX)
                            maxX = currX;

                        currX = 0;
                        currY += fontSize;

                        continue;
                    }

                    currX += xAdvance * currFontSize;
                    
                } while (i < stringLength);

                if (currX > maxX)
                    maxX = currX;

                return { maxX, currY }; 
            }

            inline void drawString(const char* string, bool monospace, u32 x, u32 y, float fontSize, Color color) {
                const size_t stringLength = strlen(string);

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

                    stbtt_fontinfo *currFont = nullptr;

                    if (stbtt_FindGlyphIndex(&this->m_extFont, currCharacter))
                        currFont = &this->m_extFont;
                    else
                        currFont = &this->m_stdFont;

                    float currFontSize = stbtt_ScaleForPixelHeight(currFont, fontSize);
                    currX += currFontSize * stbtt_GetCodepointKernAdvance(currFont, prevCharacter, currCharacter);

                    int bounds[4] = { 0 };
                    stbtt_GetCodepointBitmapBoxSubpixel(currFont, currCharacter, currFontSize, currFontSize,
                                                        0, 0, &bounds[0], &bounds[1], &bounds[2], &bounds[3]);

                    int xAdvance = 0, yAdvance = 0;
                    stbtt_GetCodepointHMetrics(currFont, monospace ? 'A' : currCharacter, &xAdvance, &yAdvance);

                    if (currCharacter == '\n') {
                        currX = x;
                        currY += fontSize;

                        continue;
                    }

                   if (!std::iswspace(currCharacter))
                        this->drawGlyph(currCharacter, currX + bounds[0], currY + bounds[1], color, currFont, currFontSize);

                    currX += xAdvance * currFontSize;
                    
                } while (i < stringLength);
            }
            
        private:
            Renderer() {}

            static Renderer& get() {
                static Renderer renderer;

                return renderer;
            }

            static void setOpacity(float opacity) {
                opacity = std::clamp(opacity, 0.0F, 1.0F);

                Renderer::s_opacity = opacity;
            }

            bool m_initialized = false;
            ViDisplay m_display;
            ViLayer m_layer;
            Event m_vsyncEvent;

            NWindow m_window;
            Framebuffer m_framebuffer;
            void *m_currentFramebuffer = nullptr;
            
            bool m_scissoring = false;
            u16 m_scissorBounds[4];

            stbtt_fontinfo m_stdFont, m_extFont;

            static inline float s_opacity = 1.0F;

                    
            void* getCurrentFramebuffer() {
                return this->m_currentFramebuffer;
            }

            void* getNextFramebuffer() {
                return static_cast<u8*>(this->m_framebuffer.buf) + this->getNextFramebufferSlot() * this->getFramebufferSize();
            }

            size_t getFramebufferSize() {
                return this->m_framebuffer.fb_size;
            }

            size_t getFramebufferCount() {
                return this->m_framebuffer.num_fbs;
            }

            u8 getCurrentFramebufferSlot() {
                return this->m_window.cur_slot;
            }

            u8 getNextFramebufferSlot() {
                return (this->getCurrentFramebufferSlot() + 1) % this->getFramebufferCount();
            }

            void waitForVSync() {
                eventWait(&this->m_vsyncEvent, UINT64_MAX);
            }

            inline const u32 getPixelOffset(u32 x, u32 y) {
                if (this->m_scissoring) {
                    if (x < this->m_scissorBounds[0] ||
                        y < this->m_scissorBounds[1] ||
                        x > this->m_scissorBounds[0] + this->m_scissorBounds[2] ||
                        y > this->m_scissorBounds[1] + this->m_scissorBounds[3])
                            return cfg::FramebufferWidth * cfg::FramebufferHeight * 2 + 1;
                }

                u32 tmpPos = ((y & 127) / 16) + (x / 32 * 8) + ((y / 16 / 8) * (((cfg::FramebufferWidth / 2) / 16 * 8)));
                tmpPos *= 16 * 16 * 4;

                tmpPos += ((y % 16) / 8) * 512 + ((x % 32) / 16) * 256 + ((y % 8) / 2) * 64 + ((x % 16) / 8) * 32 + (y % 2) * 16 + (x % 8) * 2;
                
                return tmpPos / 2;
            }
        };

    }

    // Elements

    namespace elm {

        class Element {
        public:
            Element() {}
            virtual ~Element() {}

            virtual Element* requestFocus(Element *oldFocus, FocusDirection direction) {
                return nullptr;
            }

            virtual bool onClick(u64 keys) {
                return false;
            }

            virtual bool onTouch(u32 x, u32 y) {
                return false;
            }

            virtual void draw(gfx::Renderer *renderer) = 0;
            virtual void layout(u16 parentX, u16 parentY, u16 parentWidth, u16 parentHeight) = 0;

            virtual void frame(gfx::Renderer *renderer) final {
                if (this->m_focused)
                    this->drawHighlight(renderer);

                this->draw(renderer);
            }

            virtual void invalidate() final {
                const auto& parent = this->getParent();

                if (parent == nullptr)
                    this->layout(0, 0, cfg::FramebufferWidth, cfg::FramebufferHeight);
                else
                    this->layout(parent->getX(), parent->getY(), parent->getWidth(), parent->getHeight());
            }

            virtual void shakeHighlight(FocusDirection direction) final {
                this->m_highlightShaking = true;
                this->m_highlightShakingDirection = direction;
                this->m_highlightShakingStartTime = std::chrono::system_clock::now();
            }

            virtual void drawHighlight(gfx::Renderer *renderer) {
                static float counter = 0;
                const float progress = (std::sin(counter) + 1) / 2;
                gfx::Color highlightColor = {   static_cast<u8>((0x2 - 0x8) * progress + 0x8),
                                                static_cast<u8>((0x8 - 0xF) * progress + 0xF), 
                                                static_cast<u8>((0xC - 0xF) * progress + 0xF), 
                                                0xF };

                counter += 0.1F;

                s32 x = 0, y = 0;

                if (this->m_highlightShaking) {
                    auto t = (std::chrono::system_clock::now() - this->m_highlightShakingStartTime);
                    if (t >= 100ms)
                        this->m_highlightShaking = false;
                    else {
                        s32 amplitude = std::rand() % 5 + 5;

                        switch (this->m_highlightShakingDirection) {
                            case FocusDirection::Up:
                                y -= shakeAnimation(t, amplitude);
                                break;
                            case FocusDirection::Down:
                                y += shakeAnimation(t, amplitude);
                                break;
                            case FocusDirection::Left:
                                x -= shakeAnimation(t, amplitude);
                                break;
                            case FocusDirection::Right:
                                x += shakeAnimation(t, amplitude);
                                break;
                            default:
                                break;
                        }

                        x = std::clamp(x, -amplitude, amplitude);
                        y = std::clamp(y, -amplitude, amplitude);
                    }
                }

                renderer->drawRect(this->m_x, this->m_y, this->m_width, this->m_height, a(0xF000));

                renderer->drawRect(this->m_x + x - 4, this->m_y + y - 4, this->m_width + 8, 4, a(highlightColor));
                renderer->drawRect(this->m_x + x - 4, this->m_y + y + this->m_height, this->m_width + 8, 4, a(highlightColor));
                renderer->drawRect(this->m_x + x - 4, this->m_y + y, 4, this->m_height, a(highlightColor));
                renderer->drawRect(this->m_x + x + this->m_width, this->m_y + y, 4, this->m_height, a(highlightColor));
            }

            virtual void setBoundaries(u16 x, u16 y, u16 width, u16 height) final {
                this->m_x = x;
                this->m_y = y;
                this->m_width = width;
                this->m_height = height;
            }

            virtual u16 getX() final { return this->m_x; }
            virtual u16 getY() final { return this->m_y; }
            virtual u16 getWidth()  final { return this->m_width;  }
            virtual u16 getHeight() final { return this->m_height; }

            virtual void setParent(Element *parent) final { this->m_parent = parent; }
            virtual Element* getParent() final { return this->m_parent; }

            virtual void setFocused(bool focused) { this->m_focused = focused; }

        protected:
            constexpr static inline auto a = &gfx::Renderer::a;

        private:
            friend class Gui;

            u16 m_x = 0, m_y = 0, m_width = 0, m_height = 0;
            Element *m_parent = nullptr;
            bool m_focused = false;

            // Highlight shake animation
            bool m_highlightShaking = false;
            std::chrono::system_clock::time_point m_highlightShakingStartTime;
            FocusDirection m_highlightShakingDirection;

            int shakeAnimation(std::chrono::system_clock::duration t, float a) {
                float w = 0.2F;
                float tau = 0.05F;

                int t_ = t.count() / 1'000'000;

                return roundf(a * exp(-(tau * t_) * sin(w * t_)));
            }
        };


        class OverlayFrame : public Element {
        public:
            OverlayFrame(std::string title, std::string subtitle) : Element(), m_title(title), m_subtitle(subtitle) {}
            ~OverlayFrame() {
                if (this->m_contentElement != nullptr)
                    delete this->m_contentElement;
            }

            virtual void draw(gfx::Renderer *renderer) override {
                renderer->fillScreen(a({ 0x0, 0x0, 0x0, 0xD }));

                renderer->drawString(this->m_title.c_str(), false, 20, 50, 30, a(0xFFFF));
                renderer->drawString(this->m_subtitle.c_str(), false, 20, 70, 15, a(0xFFFF));

                renderer->drawRect(15, 720 - 73, tsl::cfg::FramebufferWidth - 30, 1, a(0xFFFF));
                renderer->drawString("\uE0E1  Back     \uE0E0  OK", false, 30, 693, 23, a(0xFFFF));

                if (this->m_contentElement != nullptr)
                    this->m_contentElement->frame(renderer);
            }

            virtual void layout(u16 parentX, u16 parentY, u16 parentWidth, u16 parentHeight) override {
                this->setBoundaries(parentX, parentY, parentWidth, parentHeight);

                if (this->m_contentElement != nullptr) {
                    this->m_contentElement->setBoundaries(parentX + 35, parentY + 175, parentWidth - 85, parentHeight - 90 - 100);
                    this->m_contentElement->invalidate();
                }
            }

            virtual Element* requestFocus(Element *oldFocus, FocusDirection direction) override {
                return this->m_contentElement->requestFocus(oldFocus, direction);
            }

            virtual void setContent(Element *content) final {
                if (this->m_contentElement != nullptr)
                    delete this->m_contentElement;

                this->m_contentElement = content;

                if (content != nullptr) {
                    this->m_contentElement->setParent(this);
                    this->invalidate();
                }
            }

        private:
            Element *m_contentElement = nullptr;

            std::string m_title, m_subtitle;
        };


        class DebugRectangle : public Element {
        public:
            DebugRectangle(gfx::Color color) : Element(), m_color(color) {}
            ~DebugRectangle() {}

            virtual void draw(gfx::Renderer *renderer) override {
                renderer->drawRect(this->getX(), this->getY(), this->getWidth(), this->getHeight(), a(this->m_color));
            }

            virtual void layout(u16 parentX, u16 parentY, u16 parentWidth, u16 parentHeight) override {}

        private:
            gfx::Color m_color;
        };


        class ListItem : public Element {
        public:
            ListItem(std::string text) : Element(), m_text(text) {}
            ~ListItem() {}

            virtual void draw(gfx::Renderer *renderer) override {
                if (this->m_valueWidth == 0) {
                    auto [width, height] = renderer->getStringBounds(this->m_value.c_str(), false, 20);
                    this->m_valueWidth = width;
                }

                renderer->drawRect(this->getX(), this->getY(), this->getWidth(), 1, a({ 0x5, 0x5, 0x5, 0xF }));
                renderer->drawRect(this->getX(), this->getY() + this->getHeight(), this->getWidth(), 1, a({ 0x5, 0x5, 0x5, 0xF }));

                renderer->drawString(this->m_text.c_str(), false, this->getX() + 20, this->getY() + 45, 23, a({ 0xF, 0xF, 0xF, 0xF }));

                renderer->drawString(this->m_value.c_str(), false, this->getX() + this->getWidth() - this->m_valueWidth - 20, this->getY() + 45, 20, this->m_faint ? a({ 0x6, 0x6, 0x6, 0xF }) : a({ 0x5, 0xC, 0xA, 0xF }));
            }

            virtual void layout(u16 parentX, u16 parentY, u16 parentWidth, u16 parentHeight) override {
                
            }

            virtual Element* requestFocus(Element *oldFocus, FocusDirection direction) override {
                return this;
            }


            virtual void setText(std::string text) final { 
                this->m_text = text;
            }

            virtual void setValue(std::string value, bool faint = false) final { 
                this->m_value = value;
                this->m_faint = faint;
                this->m_valueWidth = 0;
            }

        private:
            std::string m_text;
            std::string m_value = "";
            bool m_faint = false;

            u16 m_valueWidth = 0;
        };


        class ToggleListItem : public ListItem {
        public:
            ToggleListItem(std::string text, bool initialState, std::string onValue = "On", std::string offValue = "Off")
                : ListItem(text), m_state(initialState), m_onValue(onValue), m_offValue(offValue) {
                
                this->setState(this->m_state);
            }

            ~ToggleListItem() {}

            virtual bool onClick(u64 keys) {
                if (keys & KEY_A) {
                    this->m_state = !this->m_state;

                    this->setState(this->m_state);

                    return true;
                }

                return false;
            }

            virtual bool getState() final {
                return this->m_state;
            }

            virtual void setState(bool state) final {
                if (state)
                    this->setValue(this->m_onValue, false);
                else
                    this->setValue(this->m_offValue, true);
            }

        private:
            bool m_state = true;
            std::string m_onValue, m_offValue;
        };


        class List : public Element {
        public:
            List(u16 entriesShown = 5) : Element(), m_entriesShown(entriesShown) {}
            ~List() {
                for (auto& item : this->m_items)
                    delete item.element;
            }

            virtual void draw(gfx::Renderer *renderer) override {
                u16 i = 0;
                for (auto &entry : this->m_items) {
                    i++;
                    if (i < this->m_offset || i > this->m_entriesShown + this->m_offset) continue;
                    entry.element->frame(renderer);
                }
            }

            virtual void layout(u16 parentX, u16 parentY, u16 parentWidth, u16 parentHeight) override {
                u16 y = this->getY();
                u16 i = 0;
                for (auto &entry : this->m_items) {
                    i++;
                    if (i < this->m_offset || i > this->m_entriesShown + this->m_offset) continue;
                    entry.element->setBoundaries(this->getX(), y, this->getWidth(), entry.height);
                    entry.element->invalidate();

                    y += entry.height;
                }
            }

            virtual void addItem(Element *element, u16 height = 0) final {
                if (height == 0) {
                    if (dynamic_cast<ListItem*>(element) != nullptr)
                        height = tsl::style::ListItemDefaultHeight;
                }

                if (element != nullptr && height > 0) {
                    element->setParent(this);
                    this->m_items.push_back({ element, height });
                    this->invalidate();
                }
            }   

            virtual Element* requestFocus(Element *oldFocus, FocusDirection direction) override {
                if (this->m_items.size() == 0)
                    return nullptr;

                auto it = std::find(this->m_items.begin(), this->m_items.end(), oldFocus);

                if (it == this->m_items.end() || direction == FocusDirection::None)
                    return this->m_items[0].element;

                if (direction == FocusDirection::Up) {
                    if (it == this->m_items.begin())
                        return this->m_items[0].element;
                    else {
                        if (oldFocus == (this->m_items.begin() + this->m_offset)->element)
                            if (this->m_offset > 1) {
                                this->m_offset--;
                                this->invalidate();
                            }

                        return (it - 1)->element;
                    }
                } else if (direction == FocusDirection::Down) {
                    if (it == (this->m_items.end() - 1)) {
                        if (this->m_items.size() > 0)
                            return this->m_items[this->m_items.size() - 1].element;
                        else return nullptr;
                    }
                    else {
                        // there are more items hidden and old focus was on second last item
                        if (this->m_items.size() > size_t(this->m_offset + this->m_entriesShown) && oldFocus == (this->m_items.begin() + this->m_offset + this->m_entriesShown - 2)->element) {
                            this->m_offset++;
                            this->invalidate();
                        }

                        return (it + 1)->element;
                    }
                }
                
                return it->element;
            }


            class CustomDrawer : public Element {
            public:
                CustomDrawer(std::function<void(gfx::Renderer*, u16 x, u16 y, u16 w, u16 h)> renderFunc) : Element(), m_renderFunc(renderFunc) {}
                ~CustomDrawer() {}

                virtual void draw(gfx::Renderer* renderer) override {
                    this->m_renderFunc(renderer, this->getX(), this->getY(), this->getWidth(), this->getHeight());
                }

                virtual void layout(u16 parentX, u16 parentY, u16 parentWidth, u16 parentHeight) override {}

            private:
                std::function<void(gfx::Renderer*, u16 x, u16 y, u16 w, u16 h)> m_renderFunc;
            };


        private:
            struct ListEntry {
                Element *element;
                u16 height;

                bool operator==(Element *other) {
                    return this->element == other;
                }
            };

            std::vector<ListEntry> m_items;
            u16 m_focusedElement = 0;

            u16 m_offset = 1;
            u16 m_entriesShown = 5;
        };

    }

    // GUI

    class Gui {
    public:
        Gui() { }

        virtual ~Gui() {
            if (this->m_topElement != nullptr)
                delete this->m_topElement;
        }

        virtual elm::Element* createUI() = 0;
        virtual void update() {}
        virtual bool handleInput(u64 keysDown, u64 keysHeld, touchPosition touchInput, JoystickPosition leftJoyStick, JoystickPosition rightJoyStick) {
            return false;
        }

        virtual void draw(gfx::Renderer *renderer) final {
            if (this->m_topElement != nullptr)
                this->m_topElement->draw(renderer);
        }

        virtual elm::Element* getTopElement() final {
            return this->m_topElement;
        }

        virtual elm::Element* getFocusedElement() final {
            return this->m_focusedElement;
        }

        virtual void requestFocus(elm::Element *element, FocusDirection direction) {
            elm::Element *oldFocus = this->m_focusedElement;

            if (element != nullptr) {
                this->m_focusedElement = element->requestFocus(oldFocus, direction);

                if (oldFocus != nullptr)
                    oldFocus->setFocused(false);

                if (this->m_focusedElement != nullptr) {
                    this->m_focusedElement->setFocused(true);
                }
            }

            if (oldFocus == this->m_focusedElement && this->m_focusedElement != nullptr)
                this->m_focusedElement->shakeHighlight(direction);
        }

        virtual void removeFocus(elm::Element* element = nullptr) {
            if (element == nullptr || element == this->m_focusedElement)
                this->m_focusedElement = nullptr;
        }

    protected:
        constexpr static inline auto a = &gfx::Renderer::a;

    private:
        elm::Element *m_focusedElement = nullptr;
        elm::Element *m_topElement = nullptr;

        template <typename, typename>
        friend class Overlay;
        friend class gfx::Renderer;
    };


    // Overlay

    template<typename Gui, typename Enabled = void>
    class Overlay;

    template <typename Gui>
    class Overlay<Gui, std::enable_if_t<std::is_base_of_v<tsl::Gui, Gui>>> : private hlp::OverlayBase {
    public:
        virtual void initServices() {}  // Called at the start to initialize all services necessary for this Overlay
        virtual void exitServices() {}  // Callet at the end to clean up all services previously initialized

        virtual void onShow() {}        // Called before overlay wants to change from invisible to visible state
        virtual void onHide() {}        // Called before overlay wants to change from visible to invisible state

        virtual std::unique_ptr<tsl::Gui>& getCurrentGui() final {
            return this->m_guiStack[this->m_guiStack.size() - 1];
        }

        virtual void show() final {
            if (this->m_disableNextAnimation) {
                this->m_animationCounter = 5;
                this->m_disableNextAnimation = false;
            }
            else {
                this->m_fadeInAnimationPlaying = true;
                this->m_animationCounter = 0;
            }

            this->onShow();
        }

        virtual void hide() final {
            if (this->m_disableNextAnimation) {
                this->m_animationCounter = 0;
                this->m_disableNextAnimation = false;
            }
            else {
                this->m_fadeOutAnimationPlaying = true;
                this->m_animationCounter = 5;
            }

            this->onHide();
        }

        virtual void close() final {
            this->m_shouldClose = true;
        }

        virtual bool shouldHide() final {
            return this->m_shouldHide;
        }

        virtual bool shouldClose() final {
            return this->m_shouldClose;
        }

        static auto& get() {
            static Overlay overlay;

            return overlay;
        }

    protected:
        Overlay() {}                // Called once when overlay gets loaded
        virtual ~Overlay() {}       // Called once before overlay exits and 

        template<typename G>
        std::unique_ptr<tsl::Gui>& changeTo() {
            auto newGui = std::make_unique<G>();
            newGui->m_topElement = newGui->createUI();
            newGui->requestFocus(newGui->m_topElement, FocusDirection::None);

            this->m_guiStack.push_back(std::move(newGui));

            return this->m_guiStack.back();
        }

        void goBack() {
            if (this->m_guiStack.size() > 0)
                this->m_guiStack.pop_back();

            if (this->m_guiStack.size() == 0)
                this->close();
        }

    private:       
        std::vector<std::unique_ptr<tsl::Gui>> m_guiStack;
        
        bool m_fadeInAnimationPlaying = true, m_fadeOutAnimationPlaying = false;
        u8 m_animationCounter = 0;

        bool m_shouldHide = false;
        bool m_shouldClose = false;

        bool m_disableNextAnimation = false;
        
        virtual void loadDefaultGui() final { 
            if (this->m_guiStack.size() != 0) 
                return;

            this->changeTo<Gui>();
        }

        virtual void initScreen() final {
            gfx::Renderer::get().init();
        }

        virtual void exitScreen() final {
            gfx::Renderer::get().exit();
        }

        virtual void animationLoop() final {
            if (this->m_fadeInAnimationPlaying) {
                this->m_animationCounter++;

                if (this->m_animationCounter >= 5)
                    this->m_fadeInAnimationPlaying = false;
            }

            if (this->m_fadeOutAnimationPlaying) {
                this->m_animationCounter--;

                if (this->m_animationCounter == 0) {
                    this->m_fadeOutAnimationPlaying = false;
                    this->m_shouldHide = true;
                }
            }

            gfx::Renderer::setOpacity(0.2 * this->m_animationCounter);
        }

        virtual void loop() final {
            auto& renderer = gfx::Renderer::get();

            renderer.startFrame();


            this->animationLoop();
            this->getCurrentGui()->update();
            this->getCurrentGui()->draw(&renderer);

            renderer.endFrame();
        }

        virtual void handleInput(u64 keysDown, u64 keysHeld, touchPosition touchPos, JoystickPosition joyStickPosLeft, JoystickPosition joyStickPosRight) final {
            auto& currentGui = this->getCurrentGui();
            auto currentFocus = currentGui->getFocusedElement();

            if (currentFocus == nullptr) {
                if (elm::Element* topElement = currentGui->getTopElement(); topElement == nullptr) {
                    if (keysDown & KEY_B) 
                        this->goBack();

                    return;
                }
                else
                    currentFocus = topElement;
            }

            bool handled = false;
            elm::Element *parentElement = currentFocus;
            do {
                handled = parentElement->onClick(keysDown);
                parentElement = parentElement->getParent();
            } while (!handled && parentElement != nullptr);

            handled = handled | currentGui->handleInput(keysDown, keysHeld, touchPos, joyStickPosLeft, joyStickPosRight);

            if (!handled) {
                if (keysDown & KEY_UP)
                    currentGui->requestFocus(currentFocus->getParent(), FocusDirection::Up);
                else if (keysDown & KEY_DOWN)
                    currentGui->requestFocus(currentFocus->getParent(), FocusDirection::Down);
                else if (keysDown & KEY_LEFT)
                    currentGui->requestFocus(currentFocus->getParent(), FocusDirection::Left);
                else if (keysDown & KEY_RIGHT)
                    currentGui->requestFocus(currentFocus->getParent(), FocusDirection::Right);
                else if (keysDown & KEY_B) 
                    this->goBack();
            }
        }

        virtual void clearScreen() final {
            auto& renderer = gfx::Renderer::get();

            renderer.startFrame();
            renderer.clearScreen();
            renderer.endFrame();
        }

        virtual void resetFlags() final {
            this->m_shouldHide = false;
            this->m_shouldClose = false;
        }

        virtual void disableNextAnimation() final {
            this->m_disableNextAnimation = true;
        }

        template<typename, impl::LaunchFlags launchFlags>
        friend int loop(int argv, char** argc);

        friend class tsl::Gui;
    };

    
    namespace impl {
        
        enum class LaunchFlags : u8 {
            None = 0,
            SkipComboInitially = BIT(0)
        };

        LaunchFlags operator|(LaunchFlags lhs, LaunchFlags rhs) {
            return static_cast<LaunchFlags>(u8(lhs) | u8(rhs));
        }

        struct SharedThreadData {
            bool running = false;

            Event comboEvent = { 0 }, homeButtonPressEvent = { 0 }, powerButtonPressEvent = { 0 };

            u64 launchCombo = KEY_L | KEY_DDOWN | KEY_RSTICK;
            bool overlayOpen = false;

            std::mutex dataMutex;
            u64 keysDown = 0;
            u64 keysHeld = 0;
            touchPosition touchPos = { 0 };
            JoystickPosition joyStickPosLeft = { 0 }, joyStickPosRight = { 0 };
        };

        static void parseOverlaySettings(u64 &launchCombo) {
            FILE *configFile = fopen("sdmc:/config/tesla/config.ini", "r");

            if (configFile == nullptr)
                return;

            fseek(configFile, 0, SEEK_END);
            size_t configFileSize = ftell(configFile);
            rewind(configFile);

            std::string configFileData(configFileSize, '\0');
            fread(&configFileData[0], sizeof(char), configFileSize, configFile);
            fclose(configFile);

            hlp::IniData parsedConfig = hlp::parseIni(configFileData);

            launchCombo = 0x00;
            for (std::string key : hlp::split(parsedConfig["tesla"]["key_combo"], '+'))
                launchCombo |= hlp::stringToKeyCode(key);
        }

        template<typename Overlay, impl::LaunchFlags launchFlags>
        static void hidInputPoller(void *args) {
            SharedThreadData *shData = static_cast<SharedThreadData*>(args);

            eventCreate(&shData->comboEvent, false);
            
            if constexpr (u8(launchFlags) & u8(impl::LaunchFlags::SkipComboInitially))
                eventFire(&shData->comboEvent);

            // Parse Tesla settings
            impl::parseOverlaySettings(shData->launchCombo);

            // Drop all inputs from the previous overlay
            hidScanInput();

            while (shData->running) {
                
                // Scan for input changes
                hidScanInput();

                // Read in HID values
                {
                    std::scoped_lock lock(shData->dataMutex);

                    shData->keysDown = 0;
                    shData->keysHeld = 0;

                    // Combine input from all controllers
                    for (u8 controller = 0; controller < 8; controller++) {
                        if (hidIsControllerConnected(static_cast<HidControllerID>(controller))) {
                            shData->keysDown |= hidKeysDown(static_cast<HidControllerID>(controller));
                            shData->keysHeld |= hidKeysHeld(static_cast<HidControllerID>(controller));
                        }
                    }

                    if (hidIsControllerConnected(CONTROLLER_HANDHELD)) {
                        shData->keysDown |= hidKeysDown(CONTROLLER_HANDHELD);
                        shData->keysHeld |= hidKeysHeld(CONTROLLER_HANDHELD);
                    }

                    // Read in touch positions
                    if (hidTouchCount() > 0)
                        hidTouchRead(&shData->touchPos, 0);
                    else 
                        shData->touchPos = { 0 };

                    // Read in joystick values
                    hidJoystickRead(&shData->joyStickPosLeft, CONTROLLER_HANDHELD, HidControllerJoystick::JOYSTICK_LEFT);
                    hidJoystickRead(&shData->joyStickPosRight, CONTROLLER_HANDHELD, HidControllerJoystick::JOYSTICK_RIGHT);

                }

                if (((shData->keysHeld & shData->launchCombo) == shData->launchCombo) && shData->keysDown & shData->launchCombo) {
                    if (shData->overlayOpen) {
                        Overlay::get().hide();
                        shData->overlayOpen = false;
                    }
                    else
                        eventFire(&shData->comboEvent);
                }

                if (shData->touchPos.px >= cfg::FramebufferWidth && shData->overlayOpen) {
                    if (shData->overlayOpen) {
                        Overlay::get().hide();
                        shData->overlayOpen = false;
                    }
                }

                //20 ms
                svcSleepThread(20E6);
            }
        }

        template<typename Overlay>
        static void homeButtonDetector(void *args) {
            SharedThreadData *shData = static_cast<SharedThreadData*>(args);

            // To prevent focus glitchout, close the overlay immediately when the home button gets pressed
            hidsysAcquireHomeButtonEventHandle(&shData->homeButtonPressEvent);
            eventClear(&shData->homeButtonPressEvent);

            while (shData->running) {
                if (R_SUCCEEDED(eventWait(&shData->homeButtonPressEvent, 100'000'000))) {
                    eventClear(&shData->homeButtonPressEvent);

                    if (shData->overlayOpen) {
                        Overlay::get().hide();
                        shData->overlayOpen = false;
                    }
                }
            }

        }

        template<typename Overlay>
        static void powerButtonDetector(void *args) {
            SharedThreadData *shData = static_cast<SharedThreadData*>(args);

            // To prevent focus glitchout, close the overlay immediately when the power button gets pressed
            hidsysAcquireSleepButtonEventHandle(&shData->powerButtonPressEvent);
            eventClear(&shData->powerButtonPressEvent);

            while (shData->running) {
                if (R_SUCCEEDED(eventWait(&shData->powerButtonPressEvent, 100'000'000))) {
                    eventClear(&shData->powerButtonPressEvent);

                    if (shData->overlayOpen) {
                        Overlay::get().hide();
                        shData->overlayOpen = false;
                    }
                }
            }

        }

    }



    template<typename Overlay, impl::LaunchFlags launchFlags = impl::LaunchFlags::SkipComboInitially>   
    static inline int loop(int argc, char** argv) {
        static_assert(std::is_base_of_v<tsl::hlp::OverlayBase, Overlay>, "tsl::loop expects a type derived from tsl::Overlay");

        impl::SharedThreadData shData;

        shData.running = true;

        Thread hidPollerThread, homeButtonDetectorThread, powerButtonDetectorThread;
        threadCreate(&hidPollerThread, impl::hidInputPoller<Overlay, launchFlags>, &shData, nullptr, 0x1000, 0x2C, -2);
        threadCreate(&homeButtonDetectorThread, impl::homeButtonDetector<Overlay>, &shData, nullptr, 0x1000, 0x2C, -2);
        threadCreate(&powerButtonDetectorThread, impl::powerButtonDetector<Overlay>, &shData, nullptr, 0x1000, 0x2C, -2);
        threadStart(&hidPollerThread);
        threadStart(&homeButtonDetectorThread);
        threadStart(&powerButtonDetectorThread);


        auto& overlay = Overlay::get();
        overlay.initServices();
        overlay.initScreen();
        overlay.loadDefaultGui();

        if (u8(launchFlags) & u8(impl::LaunchFlags::SkipComboInitially))
            overlay.disableNextAnimation();

        while (shData.running) {
            
            eventWait(&shData.comboEvent, UINT64_MAX);
            eventClear(&shData.comboEvent);
            shData.overlayOpen = true;
            

            hlp::requestForground(true);

            overlay.show();
            overlay.clearScreen();

            while (shData.running) {
                overlay.loop();

                {
                    std::scoped_lock lock(shData.dataMutex);
                    overlay.handleInput(shData.keysDown, shData.keysHeld, shData.touchPos, shData.joyStickPosLeft, shData.joyStickPosRight);

                    shData.keysDown = 0x00;
                    shData.keysHeld = 0x00;
                }

                if (overlay.shouldHide())
                    break;
                
                if (overlay.shouldClose())
                    shData.running = false;
            }

            overlay.clearScreen();
            overlay.resetFlags();

            hlp::requestForground(false);

            shData.overlayOpen = false;
            eventClear(&shData.comboEvent);
        }

        eventClose(&shData.homeButtonPressEvent);
        eventClose(&shData.powerButtonPressEvent);
        eventClose(&shData.comboEvent);

        threadWaitForExit(&hidPollerThread);
        threadClose(&hidPollerThread);
        threadWaitForExit(&homeButtonDetectorThread);
        threadClose(&homeButtonDetectorThread);
        threadWaitForExit(&powerButtonDetectorThread);
        threadClose(&powerButtonDetectorThread);

        overlay.exitScreen();
        overlay.exitServices();
        
        return 0;
    }

}


#ifdef TESLA_INIT_IMPL

extern "C" {

    u32 __nx_applet_type = AppletType_None;
    u32  __nx_nv_transfermem_size = 0x40000;
    ViLayerFlags __nx_vi_stray_layer_flags = (ViLayerFlags)0;

    void __appInit(void) {
        tsl::hlp::doWithSmSession([]{
            ASSERT_FATAL(fsInitialize());
            ASSERT_FATAL(fsdevMountSdmc());
            ASSERT_FATAL(hidInitialize());      // Controller inputs and Touch
            ASSERT_FATAL(plInitialize());       // Font data
            ASSERT_FATAL(pmdmntInitialize());   // PID querying
            ASSERT_FATAL(hidsysInitialize());   // Focus control
            ASSERT_FATAL(setsysInitialize());   // Settings querying
        });
    }

    void __appExit(void) {
        fsExit();
        hidExit();
        plExit();
        pmdmntExit();
        hidsysExit();
        setsysExit();
    }

}

#endif
