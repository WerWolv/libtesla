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
#include <algorithm>
#include <strings.h>
#include <cstring>
#include <string>
#include <functional>
#include <type_traits>
#include <mutex>
#include <memory>


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include "stb_truetype.h"

#pragma GCC diagnostic pop


#define ASSERT_EXIT(x) if (R_FAILED(x)) std::exit(1)
#define ASSERT_FATAL(x) if (Result res = x; R_FAILED(res)) fatalThrow(res)


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

        enum class OverlayType : u8 {
            QuickSettings,
            HUD,
            FreeFloating
        };

        struct QuickSettingsConfig {
            const char* title;
            const char* subtitle;
        };

        struct HUDConfig {
            const u16 x, y;
            const u16 width, height;
        };

        struct FreeFloatingConfig {
            const u16 initialX, initialY;
            const u16 width, height;
        };

        struct DefaultOverlayConfig {
            static inline constexpr QuickSettingsConfig QuickSettings = { .title = "Tesla Overlay", .subtitle = "v1.0.0" };
            static inline constexpr HUDConfig HUD = { .x = 0, .y = 0, .width = 128, .height = 64 };
            static inline constexpr FreeFloatingConfig FreeFloating = { .initialX = 0, .initialY = 0, .width = 128, .height = 64 };
        };

    }

    // Declarations
    enum class FocusDirection;

    template <typename, cfg::OverlayType, typename, typename>
    class Overlay;


    // Helpers

    template<auto> struct dependent_false : std::false_type { };

    void doWithSmSession(std::function<void()> f) {
        smInitialize();
        f();
        smExit();
    }

    template<typename T>
    std::pair<Result, T> readSetting(std::string section, std::string key) {
        Result res;
        u64 valueSize;
        u64 actualSize;

        T buffer;
        std::memset(&buffer, 0x00, sizeof(buffer));
 
        res = setsysGetSettingsItemValueSize(section.c_str(), key.c_str(), &valueSize);

        if (valueSize != sizeof(T))
            return { 1, T() };

        if (R_SUCCEEDED(res))
            res = setsysGetSettingsItemValue(section.c_str(), key.c_str(), &buffer, valueSize, &actualSize);

        if (valueSize != actualSize)
            return { 1, T() };

        return { res, buffer };
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

    // Renderer
    extern "C" u64 __nx_vi_layer_id;

    namespace gfx {

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

            template <typename, cfg::OverlayType, typename>
            friend class tsl::Overlay;

            static Color a(const Color &c) {
                return (c.rgba & 0x0FFF) | (static_cast<u8>(c.a * Renderer::s_opacity) << 12);
            }

            template<cfg::OverlayType overlayType, typename OverlayConfig>
            void init() {

                if constexpr (overlayType == cfg::OverlayType::QuickSettings) {

                    cfg::LayerPosX = 0;
                    cfg::LayerPosY = 0;
                    cfg::FramebufferWidth  = 448;
                    cfg::FramebufferHeight = 720;
                    cfg::LayerWidth  = cfg::ScreenHeight * (float(cfg::FramebufferWidth) / float(cfg::FramebufferHeight));
                    cfg::LayerHeight = cfg::ScreenHeight;

                } else if constexpr (overlayType == cfg::OverlayType::HUD) {

                    static_assert(OverlayConfig::HUD::width % 64  == 0,  "Framebuffer width needs to be aligned to 64!");
                    static_assert(OverlayConfig::HUD::height % 128 == 0, "Framebuffer height needs to be aligned to 128!");

                    cfg::LayerWidth  = OverlayConfig::HUD::width;
                    cfg::LayerHeight = OverlayConfig::HUD::height;
                    cfg::FramebufferWidth  = OverlayConfig::HUD::width;
                    cfg::FramebufferHeight = OverlayConfig::HUD::height;
                    cfg::LayerPosX   = OverlayConfig::HUD::x;
                    cfg::LayerPosY   = OverlayConfig::HUD::y;

                } else if constexpr (overlayType == cfg::OverlayType::FreeFloating) {

                    static_assert(OverlayConfig::FreeFloating::width % 64  == 0,  "Framebuffer width needs to be aligned to 64!");
                    static_assert(OverlayConfig::FreeFloating::height % 128 == 0, "Framebuffer height needs to be aligned to 128!");
                
                    cfg::LayerWidth  = OverlayConfig::FreeFloating::width;
                    cfg::LayerHeight = OverlayConfig::FreeFloating::height;
                    cfg::FramebufferWidth  = OverlayConfig::HUD::width;
                    cfg::FramebufferHeight = OverlayConfig::HUD::height;
                    cfg::LayerPosX   = OverlayConfig::HUD::initialX;
                    cfg::LayerPosY   = OverlayConfig::HUD::initialY;

                } else static_assert(dependent_false<overlayType>::value, "Invalid Gui Type specified");

                if (this->m_initialized)
                    return;

                tsl::doWithSmSession([this]{
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
                static_cast<Color*>(this->getCurrentFramebuffer())[this->getPixelOffset(x, y)] = color;
            }

            inline u8 blendColor(u8 src, u8 dst, u8 alpha) {
                u8 oneMinusAlpha = 0x0F - alpha;

                return (dst * alpha + src * oneMinusAlpha) / float(0xF);
            }

            inline void setPixelBlendSrc(u16 x, u16 y, Color color) {
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
                Color src((static_cast<u16*>(this->getCurrentFramebuffer()))[this->getPixelOffset(x, y)]);
                Color dst(color);
                Color end(0);

                end.r = this->blendColor(src.r, dst.r, dst.a);
                end.g = this->blendColor(src.g, dst.g, dst.a);
                end.b = this->blendColor(src.b, dst.b, dst.a);
                end.a = dst.a;

                this->setPixel(x, y, end);
            }

            inline void drawRect(u16 x1, u16 y1, u16 x2, u16 y2, Color color) {
                for (; x1 < x2; x1++)
                    for (; y1 < y2; y1++)
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
                    float currFontSize = fontSize / 1000;

                    if (stbtt_FindGlyphIndex(&this->m_stdFont, currCharacter)) {
                        currFont = &this->m_stdFont;
                    }
                    else if (stbtt_FindGlyphIndex(&this->m_extFont, currCharacter)) {
                        currFont = &this->m_extFont;
                    }
                    else return;

                    currX += currFontSize * stbtt_GetCodepointKernAdvance(currFont, prevCharacter, currCharacter);

                    int bounds[4] = { 0 };
                    stbtt_GetCodepointBitmapBoxSubpixel(currFont, currCharacter, currFontSize, currFontSize,
                                                        0, 0, &bounds[0], &bounds[1], &bounds[2], &bounds[3]);

                    int xAdvance;
                    stbtt_GetCodepointHMetrics(currFont, currCharacter, &xAdvance, nullptr);

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

            static inline float s_opacity = 0.0F;

                    
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
                if (x >= cfg::FramebufferWidth)
                    return 0;

                if (y >= cfg::FramebufferHeight)
                    return 0;

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

    static auto &a = tsl::gfx::Renderer::a;

    // Elements

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
        virtual void layout() = 0;

        virtual void drawHighlight(gfx::Renderer *renderer) {

        }

        virtual Element* requestFocus(tsl::FocusDirection direction, Element *oldFocus) final {
            return this;
        }

        virtual void shakeHighlight() final {
            
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
    private:
        friend class Gui;

        u16 m_x, m_y, m_width, m_height;
        Element *m_parent;
    };

    // GUI

    enum class FocusDirection {
        None,
        Up,
        Down,
        Left,
        Right
    };

    class Gui {
    public:
        Gui() { }

        virtual ~Gui() {
            if (this->m_topElement != nullptr)
                delete this->m_topElement;
        }

        virtual void initServices() {}
        virtual void exitServices() {}

        virtual Element* createUI() = 0;
        virtual void update() {}
        virtual void onInput(u64 keysDown, u64 keysHeld, JoystickPosition leftJoyStick, JoystickPosition rightJoyStick, touchPosition touchInput) {}

        virtual Element* getTopElement() final {
            return this->m_topElement;
        }

        virtual void requestFocus(Element *element, FocusDirection direction) {
            Element *oldFocus = this->m_focusedElement;
            this->m_focusedElement = element->requestFocus(direction, oldFocus);

            if (oldFocus == this->m_focusedElement && this->m_focusedElement != nullptr)
                this->m_focusedElement->shakeHighlight();
        }

        virtual void removeFocus(Element* element = nullptr) {
            if (element == nullptr || element == this->m_focusedElement)
                this->m_focusedElement = nullptr;
        }

    private:
        Element *m_topElement = nullptr;
        Element *m_focusedElement = nullptr;

        void drawQuickSettingsBackground(gfx::Renderer *renderer, const char *title, const char *subtitle) {
            renderer->fillScreen({ 0x0, 0x0, 0x0, 0xD });
            renderer->drawString("Hello", false, 20, 50, 30, 0xFFFF);

            /*renderer->drawString(title, false, 20, 50, 30, a(0xFFFF));
            renderer->drawString(subtitle, false, 20, 70, 15, a(0xFFFF));*/

            /*renderer->drawRect(15, 720 - 73, tsl::cfg::FramebufferWidth - 30, 1, a(0xFFFF));
            renderer->drawString("\uE0E1  Back     \uE0E0  OK", false, 30, 693, 23, a(0xFFFF));*/
        }

        template <typename, cfg::OverlayType, typename, typename>
        friend class Overlay;
        friend class Renderer;
    };


    // Overlay

    template<typename Gui, cfg::OverlayType overlayType, typename OverlayConfig, typename Enabled = void>
    class Overlay;

    template <typename T, cfg::OverlayType overlayType, typename OverlayConfig>
    class Overlay<T, overlayType, OverlayConfig, std::enable_if_t<std::is_base_of_v<tsl::Gui, T>>> {
    public:
        Overlay() {}                // Called once when overlay gets loaded
        virtual ~Overlay() {}       // Called once before overlay exits and 

        virtual void onShow() {}    // Called before overlay wants to change from invisible to visible state
        virtual void onHide() {}    // Called before overlay wants to change from visible to invisible state

        virtual Gui* getCurrentGui() final {
            return this->m_guiStack[this->m_guiStack.size() - 1];
        }

        virtual void hide() final {
            this->m_shouldHide = true;
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

    protected:
        template<typename G>
        Gui* changeTo() {
            auto newGui = new G();
            newGui->m_topElement = newGui->createUI();

            this->m_guiStack.push_back(newGui);

            return newGui;
        }

        void goBack() {
            auto &topGui = this->m_guiStack.back();

            delete topGui;
        }

    private:       
        std::vector<Gui*> m_guiStack;

        bool m_shouldHide = false;
        bool m_shouldClose = false;
        

        virtual void loadDefaultGui() final { 
            if (this->m_guiStack.size() != 0) 
                return;

            this->changeTo<T>();
        }

        virtual void initScreen() final {
            gfx::Renderer::get().init<overlayType, OverlayConfig>();
        }

        virtual void exitScreen() final {
            gfx::Renderer::get().exit();
        }

        virtual void loop() final {
            gfx::Renderer::get().startFrame();

            if constexpr (overlayType == cfg::OverlayType::QuickSettings)
                this->getCurrentGui()->drawQuickSettingsBackground(&gfx::Renderer::get(), OverlayConfig::QuickSettings.title, OverlayConfig::QuickSettings.subtitle);

            /*for (int x = 0; x < tsl::gfx::FramebufferWidth; x++)
                for (int y = 0; y < tsl::gfx::FramebufferHeight; y++) 
                    Renderer::setPixel(x, y, { 0x0, 0x0, 0x0, 0x0 });*/

            /*for (int x = 0; x < tsl::cfg::FramebufferWidth - 250; x++)
                for (int y = 0; y < tsl::cfg::FramebufferHeight; y++) 
                    gfx::Renderer::get().setPixel(x, y, { 0xF, 0xF, 0x0, 0xF });*/
            //Renderer::fillScreen({ 0xF, 0xF, 0x0, 0xF});

            gfx::Renderer::get().endFrame();
        }

        virtual void clearScreen() final {
            gfx::Renderer::get().startFrame();
            gfx::Renderer::get().clearScreen();
            gfx::Renderer::get().endFrame();
        }

        template<typename>
        friend int loop(int argv, char** argc);
        friend class Gui;
    };

    // Main Loop
    
    struct SharedThreadData {
        std::mutex dataMutex;
        bool running = false;

        Event comboEvent;
        u64 unlockKeys = 0, triggerKey = 0;
        bool overlayOpen = false;

        u64 keysDown = 0;
        u64 keysHeld = 0;
        touchPosition touchPos = { 0 };
        JoystickPosition joyStickPosLeft = { 0 }, joyStickPosRight = { 0 };
    };

    static u64 stringToKeyCode(std::string &value) {
        if (strcasecmp(value.c_str(), "A")             == 0)
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

    static void parseOverlaySettings(u64 &unlockKeys, u64 &triggerKey) {
        std::string unlockKeysStr;
        std::string triggerKeyStr;

        {
            auto [result, setting] = readSetting<std::array<char, 20>>("tesla", "combo_unlock_keys");

            if (R_SUCCEEDED(result))
                unlockKeysStr = std::string(setting.begin(), setting.end());
            
            if (R_FAILED(result) || unlockKeysStr == "")
                unlockKeysStr = "L&DDOWN";
        }

        {
            auto [result, setting] = readSetting<std::array<char, 20>>("tesla", "combo_trigger_key");

            if (R_SUCCEEDED(result))
                triggerKeyStr = std::string(setting.begin(), setting.end());
            
            if (R_FAILED(result) || triggerKeyStr == "")
                triggerKeyStr = "RS";
        }

        for (std::string key : tsl::split(unlockKeysStr, '&'))
            unlockKeys |= tsl::stringToKeyCode(key);

        triggerKey = tsl::stringToKeyCode(triggerKeyStr);
    }

    static void handleHidInput(void *args) {
        SharedThreadData &shData = *static_cast<SharedThreadData*>(args);

        eventCreate(&shData.comboEvent, false);

        JoystickPosition tmpJoyStickPosition[2] = { 0 };
        touchPosition tmpTouchPosition = { 0 };
        
        // Parse Tesla settings
        tsl::parseOverlaySettings(shData.unlockKeys, shData.triggerKey);

        // Drop all inputs from the previous overlay
        hidScanInput();

        while (shData.running) {
            
            // Scan for button presses
            hidScanInput();

            // Read in touch positions
            if (hidTouchCount() > 0)
                hidTouchRead(&tmpTouchPosition, 0);
            else 
                tmpTouchPosition = { 0 };
            // Read in joystick values
            hidJoystickRead(&tmpJoyStickPosition[HidControllerJoystick::JOYSTICK_LEFT], CONTROLLER_HANDHELD, HidControllerJoystick::JOYSTICK_LEFT);
            hidJoystickRead(&tmpJoyStickPosition[HidControllerJoystick::JOYSTICK_RIGHT], CONTROLLER_HANDHELD, HidControllerJoystick::JOYSTICK_RIGHT);

            {
                std::scoped_lock lock(shData.dataMutex);

                shData.keysDown = 0;
                shData.keysHeld = 0;

                for (u8 controller = 0; controller < 8; controller++) {
                    if (hidIsControllerConnected(static_cast<HidControllerID>(controller))) {
                        shData.keysDown |= hidKeysDown(static_cast<HidControllerID>(controller));
                        shData.keysHeld |= hidKeysHeld(static_cast<HidControllerID>(controller));
                    }
                }

                if (hidIsControllerConnected(CONTROLLER_HANDHELD)) {
                    shData.keysDown |= hidKeysDown(CONTROLLER_HANDHELD);
                    shData.keysHeld |= hidKeysHeld(CONTROLLER_HANDHELD);
                }


                shData.touchPos         = tmpTouchPosition;

                shData.joyStickPosLeft  = tmpJoyStickPosition[HidControllerJoystick::JOYSTICK_LEFT];
                shData.joyStickPosRight = tmpJoyStickPosition[HidControllerJoystick::JOYSTICK_RIGHT];

            }

            if ((shData.keysHeld & shData.unlockKeys) == shData.unlockKeys)
                if ((shData.keysDown & shData.triggerKey) == shData.triggerKey)
                    eventFire(&shData.comboEvent);

            svcSleepThread(20'000'000);
        }
    }

    template<typename T>   
    inline int loop(int argc, char** argv) {
        SharedThreadData shData;

        Thread hidThread;
        threadCreate(&hidThread, handleHidInput, &shData, nullptr, 0x1000, 0x2C, -2);
        shData.running = true;

        auto overlay = new T();
        overlay->initScreen();
        overlay->loadDefaultGui();


        threadStart(&hidThread);
        while (shData.running) {

            eventWait(&shData.comboEvent, UINT64_MAX);
            eventClear(&shData.comboEvent);
            shData.overlayOpen = true;

            overlay->m_shouldHide = false;
            overlay->onShow();

            overlay->clearScreen();

            while (shData.running) {
                overlay->loop();

                if (overlay->shouldHide())
                    break;
                
                if (overlay->shouldClose())
                    shData.running = false;
            }

            overlay->clearScreen();
            shData.overlayOpen = false;

        }


        threadWaitForExit(&hidThread);
        threadClose(&hidThread);

        overlay->exitScreen();
        
        return 0;
    }

}

extern "C" {

    u32 __nx_applet_type = AppletType_None;
    u32 __nx_nv_transfermem_size = 0x15000;


    void __appInit(void) {
        tsl::doWithSmSession([]{
            ASSERT_FATAL(hidInitialize());      // Controller inputs and Touch
            ASSERT_FATAL(plInitialize());       // Font data
            ASSERT_FATAL(pmdmntInitialize());   // PID querying
            ASSERT_FATAL(hidsysInitialize());   // Focus control
            ASSERT_FATAL(setsysInitialize());   // Settings querying
        });
    }

    void __appExit(void) {
        hidExit();
        plExit();
        pmdmntExit();
        hidsysExit();
    }
}