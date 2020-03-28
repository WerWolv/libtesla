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
#include <math.h>

#include <algorithm>
#include <cstring>
#include <cwctype>
#include <string>
#include <sstream>
#include <functional>
#include <type_traits>
#include <mutex>
#include <memory>
#include <chrono>
#include <list>
#include <stack>
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

/// Evaluates an expression that returns a result, and returns the result if it would fail.
#define R_TRY(resultExpr)               \
    ({                                  \
        const auto result = resultExpr; \
        if (R_FAILED(result)) {         \
            return result;              \
        }                               \
    })

using namespace std::literals::chrono_literals;

namespace tsl {

    // Constants

    namespace cfg {

        constexpr u32 ScreenWidth = 1920;       ///< Width of the Screen
        constexpr u32 ScreenHeight = 1080;      ///< Height of the Screen

        extern u16 LayerWidth;                  ///< Width of the Tesla layer
        extern u16 LayerHeight;                 ///< Height of the Tesla layer
        extern u16 LayerPosX;                   ///< X position of the Tesla layer
        extern u16 LayerPosY;                   ///< Y position of the Tesla layer
        extern u16 FramebufferWidth;            ///< Width of the framebuffer
        extern u16 FramebufferHeight;           ///< Height of the framebuffer
        extern u64 launchCombo;                 ///< Overlay activation key combo

    }

    namespace style {
        constexpr u32 ListItemDefaultHeight = 72;       ///< Height of a standard ListItem
        constexpr u8 ListItemHighlightSaturation = 0x6; ///< Maximum saturation of Listitem highlights
        constexpr u8 ListItemHighlightLength = 22;      ///< Maximum length of Listitem highlights

        namespace color {
            constexpr u16 ColorFrameBackground  = 0xD000;   ///< Overlay frame background color
            constexpr u16 ColorTransparent      = 0x0000;   ///< Transparent color
            constexpr u16 ColorHighlight        = 0xFDF0;   ///< Greenish highlight color
            constexpr u16 ColorFrame            = 0xF777;   ///< Outer boarder color
            constexpr u16 ColorHandle           = 0xF555;   ///< Track bar handle color
            constexpr u16 ColorText             = 0xFFFF;   ///< Standard text color
            constexpr u16 ColorDescription      = 0xFAAA;   ///< Description text color
            constexpr u16 ColorHeaderBar        = 0xFCCC;   ///< Category header rectangle color
        }
    }

    // Declarations

    /**
     * @brief Direction in which focus moved before landing on
     *        the currently focused element
     */
    enum class FocusDirection {
        None,                       ///< Focus was placed on the element programatically without user input
        Up,                         ///< Focus moved upwards
        Down,                       ///< Focus moved downwards
        Left,                       ///< Focus moved from left to rigth
        Right                       ///< Focus moved from right to left
    };

    class Overlay;

    namespace impl { 
        
        /**
         * @brief Overlay launch parameters
         */
        enum class LaunchFlags : u8 {
            None = 0,                       ///< Do nothing special at launch
            CloseOnExit        = BIT(0)     ///< Close the overlay the last Gui gets poped from the stack
        };

        [[maybe_unused]] static constexpr LaunchFlags operator|(LaunchFlags lhs, LaunchFlags rhs) {
            return static_cast<LaunchFlags>(u8(lhs) | u8(rhs));
        }

        /**
         * @brief Combo key mapping
         */
        struct KeyInfo {
            u64 key;
            const char* name;
            const char* glyph;
        };

        /**
         * @brief Combo key mappings
         * 
         * Ordered as they should be displayed
         */
        static const std::list<KeyInfo> KEYS_INFO = {
            { KEY_L, "L", "\uE0A4" }, { KEY_R, "R", "\uE0A5" },
            { KEY_ZL, "ZL", "\uE0A6" }, { KEY_ZR, "ZR", "\uE0A7" },
            { KEY_SL, "SL", "\uE0A8" }, { KEY_SR, "SR", "\uE0A9" },
            { KEY_DLEFT, "DLEFT", "\uE07B" }, { KEY_DUP, "DUP", "\uE079" }, { KEY_DRIGHT, "DRIGHT", "\uE07C" }, { KEY_DDOWN, "DDOWN", "\uE07A" },
            { KEY_A, "A", "\uE0A0" }, { KEY_B, "B", "\uE0A1" }, { KEY_X, "X", "\uE0A2" }, { KEY_Y, "Y", "\uE0A3" },
            { KEY_LSTICK, "LS", "\uE08A" }, { KEY_RSTICK, "RS", "\uE08B" },
            { KEY_MINUS, "MINUS", "\uE0B6" }, { KEY_PLUS, "PLUS", "\uE0B5" }
        };

    }

    [[maybe_unused]] static void goBack();

    [[maybe_unused]] static void setNextOverlay(const std::string& ovlPath, std::string args = "");

    template<typename TOverlay, impl::LaunchFlags launchFlags = impl::LaunchFlags::CloseOnExit>   
    int loop(int argc, char** argv);

    // Helpers

    namespace hlp {
        
        /**
         * @brief Wrapper for service initialization
         * 
         * @param f wrapped function
         */
        static inline void doWithSmSession(std::function<void()> f) {
            smInitialize();
            f();
            smExit();
        }

        /**
         * @brief Guard that will execute a passed function at the end of the current scope
         *
         * @param f wrapped function
         */
        class ScopeGuard {
            ScopeGuard(const ScopeGuard&) = delete;
            ScopeGuard& operator=(const ScopeGuard&) = delete;
            private:
                std::function<void()> f;
            public:
                __attribute__((always_inline)) ScopeGuard(std::function<void()> f) : f(std::move(f)) { }
                __attribute__((always_inline)) ~ScopeGuard() { if (f) { f(); } }
                void dismiss() { f = nullptr; }
        };

        /**
         * @brief libnx hid:sys shim that gives or takes away frocus to or from the process with the given aruid
         * 
         * @param enable Give focus or take focus
         * @param aruid Aruid of the process to focus/unfocus
         * @return Result Result
         */
        static Result hidsysEnableAppletToGetInput(bool enable, u64 aruid) {  
            const struct {
                u8 permitInput;
                u64 appletResourceUserId;
            } in = { enable != 0, aruid };

            return serviceDispatchIn(hidsysGetServiceSession(), 503, in);
        }

        /**
         * @brief Toggles focus between the Tesla overlay and the rest of the system
         * 
         * @param enabled Focus Tesla?
         */
        static void requestForeground(bool enabled) {
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

        /**
         * @brief Splits a string at the given delimeters
         * 
         * @param str String to split
         * @param delim Delimeter
         * @return Vector containing the split tokens
         */
        static std::vector<std::string> split(const std::string& str, char delim = ' ') {
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

        /**
         * @brief Limit a strings length and end it with "…"
         * 
         * @param string String to truncate
         * @param maxLength Maximum length of string
         */
        static std::string limitStringLength(std::string string, size_t maxLength) {
            if (string.length() <= maxLength)
                return string;

            std::strcpy(&string[maxLength - 2], "…");

            return string;
        }

        namespace ini {

            /**
             * @brief Ini file type
             */
            using IniData = std::map<std::string, std::map<std::string, std::string>>;

            /**
             * @brief Tesla config file
             */
            static const char* CONFIG_FILE = "/config/tesla/config.ini";

            /**
             * @brief Parses a ini string
             * 
             * @param str String to parse
             * @return Parsed data
             */
            static IniData parseIni(const std::string &str) {
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

            /**
             * @brief Unparses ini data into a string
             * 
             * @param iniData Ini data
             * @return Ini string
             */
            static std::string unparseIni(IniData const &iniData) {
                std::stringstream ss;
                bool addSectionGap = false;
                for (auto &section : iniData) {
                    if (addSectionGap)
                        ss << "\n";
                    ss << "[" << section.first << "]\n";
                    for (auto &keyValue : section.second) {
                        ss << keyValue.first << "=" << keyValue.second << "\n";
                    }
                }
                return ss.str();
            }

            /**
             * @brief Read Tesla settings file
             * 
             * @return Settings data
             */
            static IniData readOverlaySettings() {
                /* Open Sd card filesystem. */
                FsFileSystem fsSdmc;
                if (R_FAILED(fsOpenSdCardFileSystem(&fsSdmc)))
                    return {};
                hlp::ScopeGuard fsGuard([&] { fsFsClose(&fsSdmc); });

                /* Open config file. */
                FsFile fileConfig;
                if (R_FAILED(fsFsOpenFile(&fsSdmc, CONFIG_FILE, FsOpenMode_Read, &fileConfig)))
                    return {};
                hlp::ScopeGuard fileGuard([&] { fsFileClose(&fileConfig); });

                /* Get config file size. */
                s64 configFileSize;
                if (R_FAILED(fsFileGetSize(&fileConfig, &configFileSize)))
                    return {};

                /* Read and parse config file. */
                std::string configFileData(configFileSize, '\0');
                u64 readSize;
                Result rc = fsFileRead(&fileConfig, 0, configFileData.data(), configFileSize, FsReadOption_None, &readSize);
                if (R_FAILED(rc) || readSize != static_cast<u64>(configFileSize))
                    return {};

                return parseIni(configFileData);
            }

            /**
             * @brief Replace Tesla settings file with new data
             * 
             * @param iniData new data
             */
            static void writeOverlaySettings(IniData const &iniData) {
                /* Open Sd card filesystem. */
                FsFileSystem fsSdmc;
                if (R_FAILED(fsOpenSdCardFileSystem(&fsSdmc)))
                    return;
                hlp::ScopeGuard fsGuard([&] { fsFsClose(&fsSdmc); });

                /* Open config file. */
                FsFile fileConfig;
                if (R_FAILED(fsFsOpenFile(&fsSdmc, CONFIG_FILE, FsOpenMode_Write, &fileConfig)))
                    return;
                hlp::ScopeGuard fileGuard([&] { fsFileClose(&fileConfig); });

                std::string iniString = unparseIni(iniData);

                fsFileWrite(&fileConfig, 0, iniString.c_str(), iniString.length(), FsWriteOption_Flush);
            }

            /**
             * @brief Merge and save changes into Tesla settings file
             * 
             * @param changes setting values to add or update
             */
            static void updateOverlaySettings(IniData const &changes) {
                hlp::ini::IniData iniData = hlp::ini::readOverlaySettings();
                for (auto &section : changes) {
                    for (auto &keyValue : section.second) {
                        iniData[section.first][keyValue.first] = keyValue.second;
                    }
                }
                writeOverlaySettings(iniData);
            }

        }

        /**
         * @brief Decodes a key string into it's key code
         * 
         * @param value Key string
         * @return Key code
         */
        static u64 stringToKeyCode(const std::string &value) {
            for (auto &keyInfo : impl::KEYS_INFO) {
                if (strcasecmp(value.c_str(), keyInfo.name) == 0)
                    return keyInfo.key;
            }
            return 0;
        }

        /**
         * @brief Decodes a combo string into key codes
         * 
         * @param value Combo string
         * @return Key codes
         */
        static u64 comboStringToKeys(const std::string &value) {
            u64 keyCombo = 0x00;
            for (std::string key : hlp::split(value, '+')) {
                keyCombo |= hlp::stringToKeyCode(key);
            }
            return keyCombo;
        }

        /**
         * @brief Encodes key codes into a combo string
         * 
         * @param keys Key codes
         * @return Combo string
         */
        static std::string keysToComboString(u64 keys) {
            std::string str;
            for (auto &keyInfo : impl::KEYS_INFO) {
                if (keys & keyInfo.key) {
                    if (!str.empty())
                        str.append("+");
                    str.append(keyInfo.name);
                }
            }
            return str;
        }

    }

    // Renderer

    namespace gfx {

        extern "C" u64 __nx_vi_layer_id;

        /**
         * @brief RGBA4444 Color structure
         */
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

        struct ScissoringConfig {
            u16 x, y, w, h;
        };

        /**
         * @brief Manages the Tesla layer and draws raw data to the screen
         */
        class Renderer final {
        public:
            Renderer& operator=(Renderer&) = delete;

            friend class tsl::Overlay;

            /**
             * @brief Handles opacity of drawn colors for fadeout. Pass all colors through this function in order to apply opacity properly
             * 
             * @param c Original color
             * @return Color with applied opacity
             */
            static Color a(const Color &c) {
                return (c.rgba & 0x0FFF) | (static_cast<u8>(c.a * Renderer::s_opacity) << 12);
            }

            /**
             * @brief Enables scissoring, discarding of any draw outside the given boundaries
             * 
             * @param x x pos
             * @param y y pos
             * @param w Width
             * @param h Height
             */
            inline void enableScissoring(u16 x, u16 y, u16 w, u16 h) {
                if (this->m_scissoring)
                    this->m_scissoringStack.push_back(this->m_currScissorConfig);
                else
                    this->m_scissoring = true;

                this->m_currScissorConfig = { x, y, w, h };
            }

            /**
             * @brief Disables scissoring
             */
            inline void disableScissoring() {
                if (this->m_scissoringStack.size() > 0) {
                    this->m_currScissorConfig = this->m_scissoringStack.back();
                    this->m_scissoringStack.pop_back();
                } 
                else {
                    this->m_scissoring = false;
                    this->m_currScissorConfig = { 0 };
                }
            }


            // Drawing functions

            /**
             * @brief Draw a single pixel onto the screen
             * 
             * @param x X pos 
             * @param y Y pos
             * @param color Color
             */
            inline void setPixel(s16 x, s16 y, Color color) {
                if (x < 0 || y < 0 || x >= cfg::FramebufferWidth || y >= cfg::FramebufferHeight)
                    return;

                static_cast<Color*>(this->getCurrentFramebuffer())[this->getPixelOffset(x, y)] = color;
            }

            /**
             * @brief Blends two colors
             * 
             * @param src Source color
             * @param dst Destination color
             * @param alpha Opacity
             * @return Blended color
             */
            inline u8 blendColor(u8 src, u8 dst, u8 alpha) {
                u8 oneMinusAlpha = 0x0F - alpha;

                return (dst * alpha + src * oneMinusAlpha) / float(0xF);
            }
            
            /**
             * @brief Draws a single source blended pixel onto the screen
             * 
             * @param x X pos 
             * @param y Y pos
             * @param color Color
             */
            inline void setPixelBlendSrc(s16 x, s16 y, Color color) {
                if (x < 0 || y < 0 || x >= cfg::FramebufferWidth || y >= cfg::FramebufferHeight)
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

            /**
             * @brief Draws a single destination blended pixel onto the screen
             * 
             * @param x X pos 
             * @param y Y pos
             * @param color Color
             */
            inline void setPixelBlendDst(s16 x, s16 y, Color color) {
                if (x < 0 || y < 0 || x >= cfg::FramebufferWidth || y >= cfg::FramebufferHeight)
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

            /**
             * @brief Draws a rectangle of given sizes
             * 
             * @param x X pos
             * @param y Y pos
             * @param w Width
             * @param h Height
             * @param color Color
             */
            inline void drawRect(s16 x, s16 y, s16 w, s16 h, Color color) {
                for (s16 x1 = x; x1 < (x + w); x1++)
                    for (s16 y1 = y; y1 < (y + h); y1++)
                        this->setPixelBlendDst(x1, y1, color);
            }

            void drawCircle(u16 centerX, u16 centerY, u16 radius, bool filled, Color color) {
                s16 x = radius;
                s16 y = 0;
                s16 radiusError = 0;
                s16 xChange = 1 - (radius << 1);
                s16 yChange = 0;
                
                while (x >= y) {
                    if(filled) {
                        for (s16 i = centerX - x; i <= centerX + x; i++) {
                            s16 y0 = centerY + y;
                            s16 y1 = centerY - y;
                            s16 x0 = i;
                            
                            this->setPixel(x0, y0, color);
                            this->setPixel(x0, y1, color);
                        }
                        
                        for (s16 i = centerX - y; i <= centerX + y; i++) {
                            s16 y0 = centerY + x;
                            s16 y1 = centerY - x;
                            s16 x0 = i;

                            this->setPixel(x0, y0, color);
                            this->setPixel(x0, y1, color);
                        }

                        y++;
                        radiusError += yChange;
                        yChange += 2;
                        if (((radiusError << 1) + xChange) > 0) {
                            x--;
                            radiusError += xChange;
                            xChange += 2;
                        }
                    } else {
                        this->setPixel(centerX + x, centerY + y, color);
                        this->setPixel(centerX + y, centerY + x, color);
                        this->setPixel(centerX - y, centerY + x, color);
                        this->setPixel(centerX - x, centerY + y, color);
                        this->setPixel(centerX - x, centerY - y, color);
                        this->setPixel(centerX - y, centerY - x, color);
                        this->setPixel(centerX + y, centerY - x, color);
                        this->setPixel(centerX + x, centerY - y, color);
                        
                        if(radiusError <= 0) {
                            y++;
                            radiusError += 2 * y + 1;
                        } else {
                            x--;
                            radiusError -= 2 * x + 1;
                        }
                    }
                }
            }

            /**
             * @brief Draws a RGBA8888 bitmap from memory
             * 
             * @param x X start position
             * @param y Y start position
             * @param w Bitmap width
             * @param h Bitmap height
             * @param bmp Pointer to bitmap data
             */
            void drawBitmap(s32 x, s32 y, s32 w, s32 h, const u8 *bmp) {
                for (s32 y1 = 0; y1 < h; y1++) {
                    for (s32 x1 = 0; x1 < w; x1++) {
                        const Color color = { static_cast<u8>(bmp[0] >> 4), static_cast<u8>(bmp[1] >> 4), static_cast<u8>(bmp[2] >> 4), static_cast<u8>(bmp[3] >> 4) };
                        setPixelBlendSrc(x + x1, y + y1, a(color));
                        bmp += 4;
                    }
                }
            }

            /**
             * @brief Fills the entire layer with a given color
             * 
             * @param color Color
             */
            inline void fillScreen(Color color) {
                std::fill_n(static_cast<Color*>(this->getCurrentFramebuffer()), this->getFramebufferSize() / sizeof(Color), color);
            }

            /**
             * @brief Clears the layer (With transparency)
             * 
             */
            inline void clearScreen() {
                this->fillScreen({ 0x00, 0x00, 0x00, 0x00 });
            }

            /**
             * @brief Draws a string
             * 
             * @param string String to draw
             * @param monospace Draw string in monospace font
             * @param x X pos
             * @param y Y pos
             * @param fontSize Height of the text drawn in pixels 
             * @param color Text color. Use transparent color to skip drawing and only get the string's dimensions
             * @return Dimensions of drawn string
             */
            std::pair<u32, u32> drawString(const char* string, bool monospace, u32 x, u32 y, float fontSize, Color color, size_t maxWidth = 0, size_t* written = nullptr) {
                const size_t stringLength = strlen(string);

                u32 maxX = x;
                u32 currX = x;
                u32 currY = y;
                u32 prevCharacter = 0;

                u32 i = 0;

                do {
                    if (maxWidth > 0 && maxWidth < (currX - x))
                        break;

                    if (written != nullptr)
                        *written += 1;

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
                    stbtt_GetCodepointHMetrics(currFont, monospace ? 'W' : currCharacter, &xAdvance, &yAdvance);

                    if (currCharacter == '\n') {
                        maxX = std::max(currX, maxX);

                        currX = x;
                        currY += fontSize;

                        continue;
                    }

                   if (!std::iswspace(currCharacter) && fontSize > 0 && color.a != 0x0)
                        this->drawGlyph(currCharacter, currX + bounds[0], currY + bounds[1], color, currFont, currFontSize);

                    currX += xAdvance * currFontSize;

                } while (i < stringLength);

                maxX = std::max(currX, maxX);

                return { maxX - x, currY - y };
            }
            
        private:
            Renderer() {}

            /**
             * @brief Gets the renderer instance
             * 
             * @return Renderer
             */
            static Renderer& get() {
                static Renderer renderer;

                return renderer;
            }

            /**
             * @brief Sets the opacity of the layer
             * 
             * @param opacity Opacity
             */
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
            ScissoringConfig m_currScissorConfig;
            std::vector<ScissoringConfig> m_scissoringStack;

            stbtt_fontinfo m_stdFont, m_extFont;

            static inline float s_opacity = 1.0F;

            /**
             * @brief Get the current framebuffer address
             * 
             * @return Framebuffer address
             */
            inline void* getCurrentFramebuffer() {
                return this->m_currentFramebuffer;
            }

            /**
             * @brief Get the next framebuffer address
             * 
             * @return Next framebuffer address
             */
            inline void* getNextFramebuffer() {
                return static_cast<u8*>(this->m_framebuffer.buf) + this->getNextFramebufferSlot() * this->getFramebufferSize();
            }

            /**
             * @brief Get the framebuffer size
             * 
             * @return Framebuffer size
             */
            inline size_t getFramebufferSize() {
                return this->m_framebuffer.fb_size;
            }

            /**
             * @brief Get the number of framebuffers in use
             * 
             * @return Number of framebuffers
             */
            inline size_t getFramebufferCount() {
                return this->m_framebuffer.num_fbs;
            }

            /**
             * @brief Get the currently used framebuffer's slot
             * 
             * @return Slot
             */
            inline u8 getCurrentFramebufferSlot() {
                return this->m_window.cur_slot;
            }

            /**
             * @brief Get the next framebuffer's slot
             * 
             * @return Next slot
             */
            inline u8 getNextFramebufferSlot() {
                return (this->getCurrentFramebufferSlot() + 1) % this->getFramebufferCount();
            }

            /**
             * @brief Waits for the vsync event
             * 
             */
            inline void waitForVSync() {
                eventWait(&this->m_vsyncEvent, UINT64_MAX);
            }

            /**
             * @brief Decodes a x and y coordinate into a offset into the swizzled framebuffer
             * 
             * @param x X pos
             * @param y Y Pos
             * @return Offset
             */
            const u32 getPixelOffset(u32 x, u32 y) {
                if (this->m_scissoring) {
                    if (x < this->m_currScissorConfig.x ||
                        y < this->m_currScissorConfig.y ||
                        x > this->m_currScissorConfig.x + this->m_currScissorConfig.w ||
                        y > this->m_currScissorConfig.y + this->m_currScissorConfig.h)
                            return cfg::FramebufferWidth * cfg::FramebufferHeight * 2 + 1;
                }

                u32 tmpPos = ((y & 127) / 16) + (x / 32 * 8) + ((y / 16 / 8) * (((cfg::FramebufferWidth / 2) / 16 * 8)));
                tmpPos *= 16 * 16 * 4;

                tmpPos += ((y % 16) / 8) * 512 + ((x % 32) / 16) * 256 + ((y % 8) / 2) * 64 + ((x % 16) / 8) * 32 + (y % 2) * 16 + (x % 8) * 2;
                
                return tmpPos / 2;
            }

            /**
             * @brief Initializes the renderer and layers
             * 
             */
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

            /**
             * @brief Exits the renderer and layer
             * 
             */
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

            /**
             * @brief Initializes Nintendo's shared fonts. Default and Extended
             * 
             * @return Result
             */
            Result initFonts() {
                static PlFontData stdFontData, extFontData;

                // Nintendo's default font
                R_TRY(plGetSharedFontByType(&stdFontData, PlSharedFontType_Standard));

                u8 *fontBuffer = reinterpret_cast<u8*>(stdFontData.address);
                stbtt_InitFont(&this->m_stdFont, fontBuffer, stbtt_GetFontOffsetForIndex(fontBuffer, 0));
                
                // Nintendo's extended font containing a bunch of icons
                R_TRY(plGetSharedFontByType(&extFontData, PlSharedFontType_NintendoExt));

                fontBuffer = reinterpret_cast<u8*>(extFontData.address);
                stbtt_InitFont(&this->m_extFont, fontBuffer, stbtt_GetFontOffsetForIndex(fontBuffer, 0));

                return 0;
            }
            
            /**
             * @brief Start a new frame
             * @warning Don't call this more than once before calling \ref endFrame
             */
            inline void startFrame() {
                this->m_currentFramebuffer = framebufferBegin(&this->m_framebuffer, nullptr);
            }

            /**
             * @brief End the current frame
             * @warning Don't call this before calling \ref startFrame once
             */
            inline void endFrame() {
                std::memcpy(this->getNextFramebuffer(), this->getCurrentFramebuffer(), this->getFramebufferSize());
                this->waitForVSync();
                framebufferEnd(&this->m_framebuffer);

                this->m_currentFramebuffer = nullptr;
            }

            /**
             * @brief Draws a single font glyph
             * 
             * @param codepoint Unicode codepoint to draw
             * @param x X pos
             * @param y Y pos
             * @param color Color
             * @param font STB Font to use
             * @param fontSize Font size
             */
            inline void drawGlyph(s32 codepoint, s32 x, s32 y, Color color, stbtt_fontinfo *font, float fontSize) {
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
        };

    }

    // Elements

    namespace elm {
        
        /**
         * @brief The top level Element of the libtesla UI library
         * @note When creating your own elements, extend from this or one of it's sub classes
         */
        class Element {
        public:
            Element() {}
            virtual ~Element() {}

            /**
             * @brief Handles focus requesting
             * @note This function should return the element to focus. 
             *       When this element should be focused, return `this`. 
             *       When one of it's child should be focused, return `this->child->requestFocus(oldFocus, direction)`
             *       When this element is not focusable, return `nullptr`
             * 
             * @param oldFocus Previously focused element
             * @param direction Direction in which focus moved. \ref FocusDirection::None is passed for the initial load
             * @return Element to focus
             */
            virtual Element* requestFocus(Element *oldFocus, FocusDirection direction) {
                return nullptr;
            }
            
            /**
             * @brief Function called when a joycon button got pressed
             * 
             * @param keys Keys pressed in the last frame
             * @return true when button press has been consumed
             * @return false when button press should be passed on to the parent
             */
            virtual bool onClick(u64 keys) {
                return m_clickListener(keys);
            }

            /**
             * @brief Called once per frame with the latest HID inputs
             * 
             * @param keysDown Buttons pressed in the last frame
             * @param keysHeld Buttons held down longer than one frame
             * @param touchInput Last touch position
             * @param leftJoyStick Left joystick position
             * @param rightJoyStick Right joystick position
             * @return Weather or not the input has been consumed
             */
            virtual bool handleInput(u64 keysDown, u64 keysHeld, touchPosition touchPos, JoystickPosition joyStickPosLeft, JoystickPosition joyStickPosRight) {
                return false;
            }

            /**
             * @brief Function called when the element got touched
             * @todo Not yet implemented
             * 
             * @param x X pos
             * @param y Y pos
             * @return true when touch input has been consumed
             * @return false when touch input should be passed on to the parent
             */
            virtual bool onTouch(u32 x, u32 y) {
                return false;
            }

            /**
             * @brief Called once per frame to draw the element
             * @warning Do not call this yourself. Use \ref Element::frame(gfx::Renderer *renderer)
             * 
             * @param renderer Renderer
             */
            virtual void draw(gfx::Renderer *renderer) = 0;

            /**
             * @brief Called when the underlying Gui gets created and after calling \ref Gui::invalidate() to calculate positions and boundaries of the element
             * @warning Do not call this yourself. Use \ref Element::invalidate()
             * 
             * @param parentX Parent X pos
             * @param parentY Parent Y pos
             * @param parentWidth Parent Width
             * @param parentHeight Parent Height
             */
            virtual void layout(u16 parentX, u16 parentY, u16 parentWidth, u16 parentHeight) = 0;

            /**
             * @brief Draws highlighting and the element itself
             * @note When drawing children of a element in \ref Element::draw(gfx::Renderer *renderer), use `this->child->frame(renderer)` instead of calling draw directly
             * 
             * @param renderer 
             */
            virtual void frame(gfx::Renderer *renderer) final {
                if (this->m_focused)
                    this->drawFocusBackground(renderer);

                this->draw(renderer);

                renderer->enableScissoring(0, 0, tsl::cfg::FramebufferWidth, tsl::cfg::FramebufferHeight);

                if (this->m_focused)
                    this->drawHighlight(renderer);

                renderer->disableScissoring();
            }

            /**
             * @brief Forces a layout recreation of a element
             * 
             */
            virtual void invalidate() final {
                const auto& parent = this->getParent();

                if (parent == nullptr)
                    this->layout(0, 0, cfg::FramebufferWidth, cfg::FramebufferHeight);
                else
                    this->layout(parent->getX(), parent->getY(), parent->getWidth(), parent->getHeight());
            }

            /**
             * @brief Shake the highlight in the given direction to signal that the focus cannot move there
             * 
             * @param direction Direction to shake highlight in
             */
            virtual void shakeHighlight(FocusDirection direction) final {
                this->m_highlightShaking = true;
                this->m_highlightShakingDirection = direction;
                this->m_highlightShakingStartTime = std::chrono::system_clock::now();
            }

            /**
             * @brief Triggers the blue click animation to signal a element has been clicked on
             * 
             */
            virtual void triggerClickAnimation() final {
                this->m_clickAnimationProgress = tsl::style::ListItemHighlightLength;
            }

            /**
             * @brief Draws the blue highlight animation when clicking on a button
             * @note Override this if you have a element that e.g requires a non-rectangular animation or a different color
             * 
             * @param renderer Renderer
             */
            virtual void drawClickAnimation(gfx::Renderer *renderer) {
                u8 saturation = tsl::style::ListItemHighlightSaturation * (float(this->m_clickAnimationProgress) / float(tsl::style::ListItemHighlightLength));

                renderer->drawRect(this->getX(), this->getY(), this->getWidth(), this->getHeight(), a({0x0, saturation, saturation, 0xf}));
            }
            
            /**
             * @brief Draws the back background when a element is highlighted
             * @note Override this if you have a element that e.g requires a non-rectangular focus
             * 
             * @param renderer Renderer
             */
            virtual void drawFocusBackground(gfx::Renderer *renderer) {
                renderer->drawRect(this->m_x, this->m_y, this->m_width, this->m_height, a(0xF000));

                if (this->m_clickAnimationProgress > 0) {
                    this->drawClickAnimation(renderer);
                    this->m_clickAnimationProgress--;
                }
            }

            /**
             * @brief Draws the blue boarder when a element is highlighted
             * @note Override this if you have a element that e.g requires a non-rectangular focus
             * 
             * @param renderer Renderer
             */
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

                renderer->drawRect(this->m_x + x - 4, this->m_y + y - 4, this->m_width + 8, 4, a(highlightColor));
                renderer->drawRect(this->m_x + x - 4, this->m_y + y + this->m_height, this->m_width + 8, 4, a(highlightColor));
                renderer->drawRect(this->m_x + x - 4, this->m_y + y, 4, this->m_height, a(highlightColor));
                renderer->drawRect(this->m_x + x + this->m_width, this->m_y + y, 4, this->m_height, a(highlightColor));
            }

            /**
             * @brief Sets the boundaries of this view
             * 
             * @param x Start X pos
             * @param y Start Y pos
             * @param width Width
             * @param height Height
             */
            virtual void setBoundaries(s32 x, s32 y, s32 width, s32 height) final {
                this->m_x = x;
                this->m_y = y;
                this->m_width = width;
                this->m_height = height;
            }

            /**
             * @brief Adds a click listener to the element
             * 
             * @param clickListener Click listener called with keys that were pressed last frame. Callback should return true if keys got consumed
             */
            virtual void setClickListener(std::function<bool(u64 keys)> clickListener) {
                this->m_clickListener = clickListener;
            }

            /**
             * @brief Gets the element's X position
             * 
             * @return X position
             */
            virtual inline s32 getX() final { return this->m_x; }
            /**
             * @brief Gets the element's Y position
             * 
             * @return Y position
             */
            virtual inline s32 getY() final { return this->m_y; }
            /**
             * @brief Gets the element's Width
             * 
             * @return Width
             */
            virtual inline s32 getWidth()  final { return this->m_width;  }
            /**
             * @brief Gets the element's Height
             * 
             * @return Height
             */
            virtual inline s32 getHeight() final { return this->m_height; }

            /**
             * @brief Sets the element's parent
             * @note This is required to handle focus and button downpassing properly
             * 
             * @param parent Parent
             */
            virtual inline void setParent(Element *parent) final { this->m_parent = parent; }

            /**
             * @brief Get the element's parent
             * 
             * @return Parent
             */
            virtual inline Element* getParent() final { return this->m_parent; }

            /**
             * @brief Marks this element as focused or unfocused to draw the highlight
             * 
             * @param focused Focused
             */
            virtual inline void setFocused(bool focused) { this->m_focused = focused; }

        protected:
            constexpr static inline auto a = &gfx::Renderer::a;
            bool m_focused = false;
            u8 m_clickAnimationProgress = 0;

            // Highlight shake animation
            bool m_highlightShaking = false;
            std::chrono::system_clock::time_point m_highlightShakingStartTime;
            FocusDirection m_highlightShakingDirection;

            /**
             * @brief Shake animation callculation based on a damped sine wave
             * 
             * @param t Passed time
             * @param a Amplitude
             * @return Damped sine wave output
             */
            int shakeAnimation(std::chrono::system_clock::duration t, float a) {
                float w = 0.2F;
                float tau = 0.05F;

                int t_ = t.count() / 1'000'000;

                return roundf(a * exp(-(tau * t_) * sin(w * t_)));
            }

        private:
            friend class Gui;

            s32 m_x = 0, m_y = 0, m_width = 0, m_height = 0;
            Element *m_parent = nullptr;

            std::function<bool(u64 keys)> m_clickListener = [](u64) { return false; };

        };

        /**
         * @brief A Element that exposes the renderer directly to draw custom views easily
         */
        class CustomDrawer : public Element {
        public:
            /**
             * @brief Constructor
             * @note This element should only be used to draw static things the user cannot interact with e.g info text, images, etc.
             * 
             * @param renderFunc Callback that will be called once every frame to draw this view
             */
            CustomDrawer(std::function<void(gfx::Renderer* r, u16 x, u16 y, u16 w, u16 h)> renderFunc) : Element(), m_renderFunc(renderFunc) {}
            virtual ~CustomDrawer() {}

            virtual void draw(gfx::Renderer* renderer) override {
                renderer->enableScissoring(this->getX(), this->getY(), this->getWidth(), this->getHeight());
                this->m_renderFunc(renderer, this->getX(), this->getY(), this->getWidth(), this->getHeight());
                renderer->disableScissoring();
            }

            virtual void layout(u16 parentX, u16 parentY, u16 parentWidth, u16 parentHeight) override {

            }

        private:
            std::function<void(gfx::Renderer*, u16 x, u16 y, u16 w, u16 h)> m_renderFunc;
        };


        /**
         * @brief The base frame which can contain another view
         * 
         */
        class OverlayFrame : public Element {
        public:
            /**
             * @brief Constructor
             * 
             * @param title Name of the Overlay drawn bolt at the top
             * @param subtitle Subtitle drawn bellow the title e.g version number
             */
            OverlayFrame(const std::string& title, const std::string& subtitle) : Element(), m_title(title), m_subtitle(subtitle) {}
            virtual ~OverlayFrame() {
                if (this->m_contentElement != nullptr)
                    delete this->m_contentElement;
            }

            virtual void draw(gfx::Renderer *renderer) override {
                renderer->fillScreen(a(tsl::style::color::ColorFrameBackground));

                renderer->drawString(this->m_title.c_str(), false, 20, 50, 30, a(tsl::style::color::ColorText));
                renderer->drawString(this->m_subtitle.c_str(), false, 20, 70, 15, a(tsl::style::color::ColorDescription));

                renderer->drawRect(15, tsl::cfg::FramebufferHeight - 73, tsl::cfg::FramebufferWidth - 30, 1, a(tsl::style::color::ColorText));

                renderer->drawString("\uE0E1  Back     \uE0E0  OK", false, 30, 693, 23, a(tsl::style::color::ColorText));

                if (this->m_contentElement != nullptr)
                    this->m_contentElement->frame(renderer);
            }

            virtual void layout(u16 parentX, u16 parentY, u16 parentWidth, u16 parentHeight) override {
                this->setBoundaries(parentX, parentY, parentWidth, parentHeight);

                if (this->m_contentElement != nullptr) {
                    this->m_contentElement->setBoundaries(parentX + 35, parentY + 125, parentWidth - 85, parentHeight - 73 - 125);
                    this->m_contentElement->invalidate();
                }
            }

            virtual Element* requestFocus(Element *oldFocus, FocusDirection direction) override {
                if (this->m_contentElement != nullptr)
                    return this->m_contentElement->requestFocus(oldFocus, direction);
                else
                    return nullptr;
            }

            /**
             * @brief Sets the content of the frame
             * 
             * @param content Element
             */
            virtual void setContent(Element *content) final {
                if (this->m_contentElement != nullptr)
                    delete this->m_contentElement;

                this->m_contentElement = content;

                if (content != nullptr) {
                    this->m_contentElement->setParent(this);
                    this->invalidate();
                }
            }

            /**
             * @brief Changes the title of the menu
             * 
             * @param title Title to change to
             */
            virtual void setTitle(const std::string &title) final {
                this->m_title = title;
            }

            /**
             * @brief Changes the subtitle of the menu
             * 
             * @param title Subtitle to change to
             */
            virtual void setSubtitle(const std::string &subtitle) final {
                this->m_subtitle = subtitle;
            }

        protected:
            Element *m_contentElement = nullptr;

            std::string m_title, m_subtitle;
        };

        /**
         * @brief The base frame which can contain another view with a customizable header
         * 
         */
        class HeaderOverlayFrame : public Element {
        public:
            HeaderOverlayFrame(u16 headerHeight = 175) : Element(), m_headerHeight(headerHeight) {}
            virtual ~HeaderOverlayFrame() {
                if (this->m_contentElement != nullptr)
                    delete this->m_contentElement;

                if (this->m_header != nullptr)
                    delete this->m_header;
            }

            virtual void draw(gfx::Renderer *renderer) override {
                renderer->fillScreen(a(tsl::style::color::ColorFrameBackground));

                renderer->drawRect(15, tsl::cfg::FramebufferHeight - 73, tsl::cfg::FramebufferWidth - 30, 1, a(tsl::style::color::ColorText));

                renderer->drawString("\uE0E1  Back     \uE0E0  OK", false, 30, 693, 23, a(tsl::style::color::ColorText));

                if (this->m_header != nullptr)
                    this->m_header->frame(renderer);

                if (this->m_contentElement != nullptr)
                    this->m_contentElement->frame(renderer);
            }

            virtual void layout(u16 parentX, u16 parentY, u16 parentWidth, u16 parentHeight) override {
                this->setBoundaries(parentX, parentY, parentWidth, parentHeight);

                if (this->m_contentElement != nullptr) {
                    this->m_contentElement->setBoundaries(parentX + 35, parentY + this->m_headerHeight, parentWidth - 85, parentHeight - 73 - this->m_headerHeight);
                    this->m_contentElement->invalidate();
                }

                if (this->m_header != nullptr) {
                    this->m_header->setBoundaries(parentX, parentY, parentWidth, this->m_headerHeight);
                    this->m_header->invalidate();
                }
            }

            virtual Element* requestFocus(Element *oldFocus, FocusDirection direction) override {
                if (this->m_contentElement != nullptr)
                    return this->m_contentElement->requestFocus(oldFocus, direction);
                else
                    return nullptr;
            }

            /**
             * @brief Sets the content of the frame
             * 
             * @param content Element
             */
            virtual void setContent(Element *content) final {
                if (this->m_contentElement != nullptr)
                    delete this->m_contentElement;

                this->m_contentElement = content;

                if (content != nullptr) {
                    this->m_contentElement->setParent(this);
                    this->invalidate();
                }
            }

            /**
             * @brief Sets the header of the frame
             * 
             * @param header Header custom drawer
             */
            virtual void setHeader(CustomDrawer *header) final {
                if (this->m_header != nullptr)
                    delete this->m_header;

                this->m_header = header;

                if (header != nullptr) {
                    this->m_header->setParent(this);
                    this->invalidate();
                }
            }

        protected:
            Element *m_contentElement = nullptr;
            CustomDrawer *m_header = nullptr;

            u16 m_headerHeight;
        };

        /**
         * @brief Single color rectangle element mainly used for debugging to visualize boundaries
         * 
         */
        class DebugRectangle : public Element {
        public:
            /**
             * @brief Constructor
             * 
             * @param color Color of the rectangle
             */
            DebugRectangle(gfx::Color color) : Element(), m_color(color) {}
            virtual ~DebugRectangle() {}

            virtual void draw(gfx::Renderer *renderer) override {
                renderer->drawRect(this->getX(), this->getY(), this->getWidth(), this->getHeight(), a(this->m_color));
            }

            virtual void layout(u16 parentX, u16 parentY, u16 parentWidth, u16 parentHeight) override {}

        private:
            gfx::Color m_color;
        };


        /**
         * @brief A List containing list items
         * 
         */
        class List : public Element {
        public:
            /**
             * @brief Constructor
             * 
             * @param entriesShown Amount of items displayed in the list at once before scrolling starts
             */
            List(u16 entriesShown = 5) : Element(), m_entriesShown(entriesShown) {}
            virtual ~List() {
                for (auto& item : this->m_items)
                    delete item;
            }

            virtual void draw(gfx::Renderer *renderer) override {
                renderer->enableScissoring(this->getX(), this->getY(), this->getWidth(), this->getHeight());

                for (auto &entry : this->m_items) {
                    if (entry->getY() + entry->getHeight() > this->getY() && entry->getY() < this->getY() + this->getHeight()) {
                        entry->frame(renderer);
                    }
                }

                renderer->disableScissoring();

                this->m_offset += (this->m_nextOffset - this->m_offset) * 0.1F;
                this->invalidate();
            }

            virtual void layout(u16 parentX, u16 parentY, u16 parentWidth, u16 parentHeight) override {
                s32 y = this->getY() - this->m_offset;

                this->m_listHeight = 0;
                for (auto &entry : this->m_items)
                    this->m_listHeight += entry->getHeight();

                for (auto &entry : this->m_items) {
                    entry->setBoundaries(this->getX(), y, this->getWidth(), entry->getHeight());
                    entry->invalidate();
                    y += entry->getHeight();
                }
            }

            /**
             * @brief Adds a new item to the list
             * 
             * @param element Element to add
             * @param height Height of the element. Don't set this parameter for libtesla to try and figure out the size based on the type 
             */
            virtual void addItem(Element *element, u16 height = 0) final {
                if (element != nullptr) {
                    if (height != 0)
                        element->setBoundaries(this->getX(), this->getY(), this->getWidth(), height);

                    element->setParent(this);
                    element->invalidate();
                    this->m_items.push_back(element);
                    this->invalidate();
                }

                if (this->m_items.size() == 1)
                    this->requestFocus(nullptr, FocusDirection::None);
            }   

            /**
             * @brief Removes all children from the list
             * @warning When clearing a list, make sure none of the its children are focused. Call \ref Gui::removeFocus before.
             */
            virtual void clear() final {     
                for (auto& item : this->m_items)
                    delete item;

                this->m_items.clear();
                this->m_offset = 0;
                this->m_focusedIndex = 0;
            }

            virtual Element* requestFocus(Element *oldFocus, FocusDirection direction) override {
                Element *newFocus = nullptr;

                if (direction == FocusDirection::None) {
                    for (u16 i = 0; i < this->m_items.size(); i++) {
                        newFocus = this->m_items[i]->requestFocus(oldFocus, direction);

                        if (newFocus != nullptr) {
                            m_focusedIndex = i;

                            this->updateScrollOffset();
                            return newFocus;
                        }
                    }
                } else {
                    if (direction == FocusDirection::Down) {

                        for (u16 i = this->m_focusedIndex + 1; i < this->m_items.size(); i++) {
                            newFocus = this->m_items[i]->requestFocus(oldFocus, direction);

                            if (newFocus != nullptr && newFocus != oldFocus) {
                                this->m_focusedIndex = i;

                                this->updateScrollOffset();
                                return newFocus;
                            }
                        }

                        return oldFocus;
                    } else if (direction == FocusDirection::Up) {
                        if (this->m_focusedIndex > 0) {

                            for (u16 i = this->m_focusedIndex - 1; i >= 0; i--) {
                                if (i > this->m_items.size() || this->m_items[i] == nullptr)
                                    return oldFocus;
                                else
                                    newFocus = this->m_items[i]->requestFocus(oldFocus, direction);
                                
                                if (newFocus != nullptr && newFocus != oldFocus) {
                                    this->m_focusedIndex = i;
                                    
                                    this->updateScrollOffset();
                                    return newFocus;
                                }
                            }
                        }

                        return oldFocus;
                    }
                }

                return oldFocus;
            }

            /**
             * @brief Gets the index in the list of the element passed in
             * 
             * @param element Element to check
             * @return Index in list. -1 for if the element isn't a member of the list
             */
            virtual s32 getIndexInList(Element *element) {
                auto it = std::find(this->m_items.begin(), this->m_items.end(), element);

                if (it == this->m_items.end())
                    return -1;

                return it - this->m_items.begin();
            }

        protected:
            std::vector<Element*> m_items;
            u16 m_focusedIndex = 0;

            float m_offset = 0, m_nextOffset = 0;
            u32 m_listHeight = 0;
            u16 m_entriesShown = 5;

        private:

            virtual void updateScrollOffset() {
                this->m_nextOffset = 0;
                for (u16 i = 0; i < this->m_focusedIndex; i++)
                    this->m_nextOffset += this->m_items[i]->getHeight();

                this->m_nextOffset -= this->getHeight() / 4;
                
                if (this->m_nextOffset < 0)
                    this->m_nextOffset = 0;

                if (this->m_nextOffset > (this->m_listHeight - this->getHeight()) + 50)
                    this->m_nextOffset = (this->m_listHeight - this->getHeight() + 50);
            }
        };

        /**
         * @brief A item that goes into a list
         * 
         */
        class ListItem : public Element {
        public:
            /**
             * @brief Constructor
             * 
             * @param text Initial description text
             */
            ListItem(const std::string& text)
                : Element(), m_text(text) {
            }
            virtual ~ListItem() {}

            virtual void draw(gfx::Renderer *renderer) override {
                if (this->m_maxWidth == 0) {
                    if (this->m_value.length() > 0) {
                        auto [valueWidth, valueHeight] = renderer->drawString(this->m_value.c_str(), false, 0, 0, 20, tsl::style::color::ColorTransparent);
                        this->m_maxWidth = this->getWidth() - valueWidth - 70;
                    } else {
                        this->m_maxWidth = this->getWidth() - 40;
                    }

                    size_t written = 0;
                    renderer->drawString(this->m_text.c_str(), false, 0, 0, 23, tsl::style::color::ColorTransparent, this->m_maxWidth, &written);
                    this->m_trunctuated = written < this->m_text.length();

                    if (this->m_trunctuated) {
                        this->m_maxScroll = this->m_text.length() + 8;
                        this->m_scrollText = this->m_text + "        " + this->m_text;
                        this->m_ellipsisText = hlp::limitStringLength(this->m_text, written);
                    }
                }

                renderer->drawRect(this->getX(), this->getY(), this->getWidth(), 1, a(tsl::style::color::ColorFrame));
                renderer->drawRect(this->getX(), this->getY() + this->getHeight(), this->getWidth(), 1, a(tsl::style::color::ColorFrame));

                const char *text = m_text.c_str();
                if (this->m_trunctuated) {
                    if (this->m_focused) {
                        if (this->m_scroll) {
                            if ((this->m_scrollAnimationCounter % 20) == 0) {
                                this->m_scrollOffset++;
                                if (this->m_scrollOffset >= this->m_maxScroll) {
                                    this->m_scrollOffset = 0;
                                    this->m_scroll = false;
                                    this->m_scrollAnimationCounter = 0;
                                }
                            }
                            text = this->m_scrollText.c_str() + this->m_scrollOffset;
                        } else {
                            if (this->m_scrollAnimationCounter > 60) {
                                this->m_scroll = true;
                                this->m_scrollAnimationCounter = 0;
                            }
                        }
                        this->m_scrollAnimationCounter++;
                    } else {
                        text = this->m_ellipsisText.c_str();
                    }
                }

                renderer->drawString(text, false, this->getX() + 20, this->getY() + 45, 23, a(tsl::style::color::ColorText), this->m_maxWidth);

                renderer->drawString(this->m_value.c_str(), false, this->getX() + this->m_maxWidth + 45, this->getY() + 45, 20, this->m_faint ? a(tsl::style::color::ColorDescription) : a(tsl::style::color::ColorHighlight));
            }

            virtual void layout(u16 parentX, u16 parentY, u16 parentWidth, u16 parentHeight) override {
                this->setBoundaries(this->getX(), this->getY(), this->getWidth(), tsl::style::ListItemDefaultHeight);
            }

            virtual bool onClick(u64 keys) {
                if (keys & KEY_A) {
                    this->triggerClickAnimation();
                    return true;
                }

                return false;
            }

            virtual void setFocused(bool state) override {
                this->m_scroll = false;
                this->m_scrollOffset = 0;
                this->m_scrollAnimationCounter = 0;
                this->m_focused = state;
            }

            virtual Element* requestFocus(Element *oldFocus, FocusDirection direction) override {
                return this;
            }

            /**
             * @brief Sets the left hand description text of the list item
             * 
             * @param text Text
             */
            virtual inline void setText(const std::string& text) {
                this->m_text = text;
                this->m_scrollText = "";
                this->m_ellipsisText = "";
                this->m_maxWidth = 0;
            }

            /**
             * @brief Sets the right hand value text of the list item
             * 
             * @param value Text
             * @param faint Should the text be drawn in a glowing green or a faint gray
             */
            virtual inline void setValue(const std::string& value, bool faint = false) {
                this->m_value = value;
                this->m_faint = faint;
                this->m_maxWidth = 0;
            }

        protected:
            std::string m_text;
            std::string m_value = "";
            std::string m_scrollText = "";
            std::string m_ellipsisText = "";

            bool m_scroll = false;
            bool m_trunctuated = false;
            bool m_faint = false;

            u16 m_maxScroll = 0;
            u16 m_scrollOffset = 0;
            u32 m_maxWidth = 0;
            u16 m_scrollAnimationCounter = 0;
        };

        /**
         * @brief A toggleable list item that changes the state from On to Off when the A button gets pressed
         * 
         */
        class ToggleListItem : public ListItem {
        public:
            /**
             * @brief Constructor
             * 
             * @param text Initial description text
             * @param initialState Is the toggle set to On or Off initially
             * @param onValue Value drawn if the toggle is on
             * @param offValue Value drawn if the toggle is off
             */
            ToggleListItem(const std::string& text, bool initialState, const std::string& onValue = "On", const std::string& offValue = "Off")
                : ListItem(text), m_state(initialState), m_onValue(onValue), m_offValue(offValue) {
                
                this->setState(this->m_state);
            }

            virtual ~ToggleListItem() {}

            virtual bool onClick(u64 keys) override {
                if (keys & KEY_A) {
                    this->m_state = !this->m_state;

                    this->setState(this->m_state);
                    this->m_stateChangedListener(this->m_state);

                    return ListItem::onClick(keys);
                }

                return false;
            }

            /**
             * @brief Gets the current state of the toggle
             * 
             * @return State
             */
            virtual inline bool getState() {
                return this->m_state;
            }

            /**
             * @brief Sets the current state of the toggle. Updates the Value
             * 
             * @param state State
             */
            virtual void setState(bool state) {
                this->m_state = state;

                if (state)
                    this->setValue(this->m_onValue, false);
                else
                    this->setValue(this->m_offValue, true);
            }

            /**
             * @brief Adds a listener that gets called whenever the state of the toggle changes
             * 
             * @param stateChangedListener Listener with the current state passed in as parameter
             */
            void setStateChangedListener(std::function<void(bool)> stateChangedListener) {
                this->m_stateChangedListener = stateChangedListener;
            } 

        protected:
            bool m_state = true;
            std::string m_onValue, m_offValue;

            std::function<void(bool)> m_stateChangedListener = [](bool){};
        };

        class CategoryHeader : public ListItem {
        public:
            CategoryHeader(const std::string &title) : ListItem(title) {}
            virtual ~CategoryHeader() {}

            virtual void draw(gfx::Renderer *renderer) override {
                renderer->drawRect(this->getX() - 2, this->getY() + this->getHeight() - 30, 5, 23, tsl::style::color::ColorHeaderBar);
                renderer->drawString(this->m_text.c_str(), false, this->getX() + 13, this->getY() + this->getHeight() - 12, 15, tsl::style::color::ColorText);
            }

            virtual void layout(u16 parentX, u16 parentY, u16 parentWidth, u16 parentHeight) override {
                // Check if the CategoryHeader is part of a list and if it's the first entry in it, half it's height
                if (List *list = dynamic_cast<List*>(this->getParent()); list != nullptr) {
                    if (list->getIndexInList(this) == 0) {
                        this->setBoundaries(this->getX(), this->getY(), this->getWidth(), tsl::style::ListItemDefaultHeight / 2);
                        return;
                    }
                }

                this->setBoundaries(this->getX(), this->getY(), this->getWidth(), tsl::style::ListItemDefaultHeight);
            }

            virtual bool onClick(u64 keys) {
                return false;
            }

            virtual Element* requestFocus(Element *oldFocus, FocusDirection direction) override {
                return nullptr;
            }
        };

        /**
         * @brief A customizable analog trackbar going from 0% to 100%
         * 
         */
        class TrackBar : public ListItem {
        public:
            /**
             * @brief Constructor
             * 
             * @param text Initial description text
             * @param initialState Is the toggle set to On or Off initially
             * @param onValue Value drawn if the toggle is on
             * @param offValue Value drawn if the toggle is off
             */
            TrackBar(const char icon[3]) : ListItem(icon), m_icon(icon) { }

            virtual ~TrackBar() {}

            virtual bool handleInput(u64 keysDown, u64 keysHeld, touchPosition touchInput, JoystickPosition leftJoyStick, JoystickPosition rightJoyStick) override {
                if (keysHeld & KEY_LEFT) {
                    if (this->m_value > 0) {
                        this->m_value--;
                        this->m_valueChangedListener(this->m_value);
                        return true;
                    }
                }

                if (keysHeld & KEY_RIGHT) {
                    if (this->m_value < 100) {
                        this->m_value++;
                        this->m_valueChangedListener(this->m_value);
                        return true;
                    }
                }

                return false;
            }

            virtual void draw(gfx::Renderer *renderer) override {
                renderer->drawRect(this->getX(), this->getY(), this->getWidth(), 1, a(tsl::style::color::ColorFrame));
                renderer->drawRect(this->getX(), this->getY() + this->getHeight(), this->getWidth(), 1, a(tsl::style::color::ColorFrame));

                renderer->drawString(this->m_icon, false, this->getX() + 15, this->getY() + 45, 23, a(tsl::style::color::ColorText), this->m_maxWidth);

                u16 handlePos = (this->getWidth() - 95) * static_cast<float>(this->m_value) / 100;
                renderer->drawCircle(this->getX() + 60, this->getY() + 37, 2, true, tsl::style::color::ColorHighlight);
                renderer->drawCircle(this->getX() + 60 + this->getWidth() - 95, this->getY() + 37, 2, true, tsl::style::color::ColorFrame);
                renderer->drawRect(this->getX() + 60 + handlePos, this->getY() + 35, this->getWidth() - 95 - handlePos, 5, tsl::style::color::ColorFrame);
                renderer->drawRect(this->getX() + 60, this->getY() + 35, handlePos, 5, tsl::style::color::ColorHighlight);

                renderer->drawCircle(this->getX() + 62 + handlePos, this->getY() + 37, 18, true, tsl::style::color::ColorHandle);
                renderer->drawCircle(this->getX() + 62 + handlePos, this->getY() + 37, 18, false, tsl::style::color::ColorFrame);
            }

            virtual void drawFocusBackground(gfx::Renderer *renderer) {
                // No background drawn here in HOS
            }

            virtual void drawHighlight(gfx::Renderer *renderer) override {
                static float counter = 0;
                const float progress = (std::sin(counter) + 1) / 2;
                gfx::Color highlightColor = {   static_cast<u8>((0x2 - 0x8) * progress + 0x8),
                                                static_cast<u8>((0x8 - 0xF) * progress + 0xF), 
                                                static_cast<u8>((0xC - 0xF) * progress + 0xF), 
                                                0xF };

                counter += 0.1F;

                u16 handlePos = (this->getWidth() - 95) * static_cast<float>(this->m_value) / 100;

                s32 x = 0;
                s32 y = 0;

                if (Element::m_highlightShaking) {
                    auto t = (std::chrono::system_clock::now() - Element::m_highlightShakingStartTime);
                    if (t >= 100ms)
                        Element::m_highlightShaking = false;
                    else {
                        s32 amplitude = std::rand() % 5 + 5;

                        switch (Element::m_highlightShakingDirection) {
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

                for (u8 i = 16; i <= 19; i++) {
                    renderer->drawCircle(this->getX() + 62 + x + handlePos, this->getY() + 37 + y, i, false, a(highlightColor));
                }
            }

            /**
             * @brief Gets the current value of the trackbar
             * 
             * @return State
             */
            virtual inline u8 getProgress() {
                return this->m_value;
            }

            /**
             * @brief Sets the current state of the toggle. Updates the Value
             * 
             * @param state State
             */
            virtual void setProgress(u8 value) {
                this->m_value = value;
            }

            /**
             * @brief Adds a listener that gets called whenever the state of the toggle changes
             * 
             * @param stateChangedListener Listener with the current state passed in as parameter
             */
            void setValueChangedListener(std::function<void(u8)> valueChangedListener) {
                this->m_valueChangedListener = valueChangedListener;
            } 

        protected:
            const char *m_icon = nullptr;
            u8 m_value = 0;

            std::function<void(u8)> m_valueChangedListener = [](u8){};

        private:
            virtual inline void setText(const std::string& text) {}
            virtual inline void setValue(const std::string& value, bool faint = false) {}
        };

        class StepTrackBar : public TrackBar {
        public:
            /**
             * @brief Constructor
             * 
             * @param text Initial description text
             * @param initialState Is the toggle set to On or Off initially
             * @param onValue Value drawn if the toggle is on
             * @param offValue Value drawn if the toggle is off
             */
            StepTrackBar(const char icon[3], std::initializer_list<std::string> stepDescriptions) 
                : TrackBar(icon), m_numSteps(stepDescriptions.size()), m_stepDescriptions(stepDescriptions.begin(), stepDescriptions.end()) { }

            virtual ~StepTrackBar() {}

            virtual bool handleInput(u64 keysDown, u64 keysHeld, touchPosition touchInput, JoystickPosition leftJoyStick, JoystickPosition rightJoyStick) override {
                return false;
            }

            virtual bool onClick(u64 key) {
                if (key & KEY_LEFT) {
                    if (this->m_value > 0) {
                        this->m_value = std::max(this->m_value - (100 / (this->m_numSteps - 1)), 0);
                        this->m_valueChangedListener(this->getProgress());
                        return true;
                    }
                }

                if (key & KEY_RIGHT) {
                    if (this->m_value < 100) {
                        this->m_value = std::min(this->m_value + (100 / (this->m_numSteps - 1)), 100);
                        this->m_valueChangedListener(this->getProgress());
                        return true;
                    }
                }

                return false;
            }

            virtual void draw(gfx::Renderer *renderer) override {

                u16 trackBarWidth = this->getWidth() - 95;
                u16 stepWidth = trackBarWidth / (this->m_numSteps - 1);

                for (u8 i = 0; i < this->m_numSteps; i++) {
                    renderer->drawRect(this->getX() + 60 + stepWidth * i, this->getY() + 45, 1, 10, tsl::style::color::ColorFrame);
                }

                u8 currentDescIndex = std::clamp(this->m_value / (100 / (this->m_numSteps - 1)), 0, this->m_numSteps - 1);

                auto [descWidth, descHeight] = renderer->drawString(this->m_stepDescriptions[currentDescIndex].c_str(), false, 0, 0, 15, tsl::style::color::ColorTransparent);
                renderer->drawString(this->m_stepDescriptions[currentDescIndex].c_str(), false, ((this->getX() + 60) + (this->getWidth() - 95) / 2) - (descWidth / 2), this->getY() + 20, 15, tsl::style::color::ColorDescription);

                TrackBar::draw(renderer);
            }

            /**
             * @brief Gets the current value of the trackbar
             * 
             * @return State
             */
            virtual inline u8 getProgress() override {
                return this->m_value / (100 / (this->m_numSteps - 1));
            }

            /**
             * @brief Sets the current state of the toggle. Updates the Value
             * 
             * @param state State
             */
            virtual void setProgress(u8 value) override {
                value = std::min(value, u8(this->m_numSteps - 1));
                this->m_value = value * (100 / (this->m_numSteps - 1));
            }

        protected:
            u8 m_numSteps = 1;
            std::vector<std::string> m_stepDescriptions;
        };

    }

    // GUI

    /**
     * @brief The top level Gui class
     * @note The main menu and every sub menu are a separate Gui. Create your own Gui class that extends from this one to create your own menus
     * 
     */
    class Gui {
    public:
        Gui() { }

        virtual ~Gui() {
            if (this->m_topElement != nullptr)
                delete this->m_topElement;
        }

        /**
         * @brief Creates all elements present in this Gui
         * @note Implement this function and let it return a heap allocated element used as the top level element. This is usually some kind of frame e.g \ref OverlayFrame
         * 
         * @return Top level element
         */
        virtual elm::Element* createUI() = 0;

        /**
         * @brief Called once per frame to update values
         * 
         */
        virtual void update() {}

        /**
         * @brief Called once per frame with the latest HID inputs
         * 
         * @param keysDown Buttons pressed in the last frame
         * @param keysHeld Buttons held down longer than one frame
         * @param touchInput Last touch position
         * @param leftJoyStick Left joystick position
         * @param rightJoyStick Right joystick position
         * @return Weather or not the input has been consumed
         */
        virtual bool handleInput(u64 keysDown, u64 keysHeld, touchPosition touchInput, JoystickPosition leftJoyStick, JoystickPosition rightJoyStick) {
            return false;
        }

        /**
         * @brief Gets the top level element
         * 
         * @return Top level element
         */
        virtual elm::Element* getTopElement() final {
            return this->m_topElement;
        }

        /**
         * @brief Get the currently focused element
         * 
         * @return Focused element
         */
        virtual elm::Element* getFocusedElement() final {
            return this->m_focusedElement;
        }
        
        /**
         * @brief Requests focus to a element
         * @note Use this function when focusing a element outside of a element's requestFocus function
         * 
         * @param element Element to focus
         * @param direction Focus direction
         */
        virtual void requestFocus(elm::Element *element, FocusDirection direction) final {
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

        /**
         * @brief Removes focus from a element
         * 
         * @param element Element to remove focus from. Pass nullptr to remove the focus unconditionally
         */
        virtual void removeFocus(elm::Element* element = nullptr) final {
            if (element == nullptr || element == this->m_focusedElement)
                this->m_focusedElement = nullptr;
        }

    protected:
        constexpr static inline auto a = &gfx::Renderer::a;

    private:
        elm::Element *m_focusedElement = nullptr;
        elm::Element *m_topElement = nullptr;

        friend class Overlay;
        friend class gfx::Renderer;

        /**
         * @brief Draws the Gui
         * 
         * @param renderer 
         */
        virtual void draw(gfx::Renderer *renderer) final {
            if (this->m_topElement != nullptr)
                this->m_topElement->draw(renderer);
        }
    };


    // Overlay

    /**
     * @brief The top level Overlay class
     * @note Every Tesla overlay should have exactly one Overlay class initializing services and loading the default Gui
     */
    class Overlay {
    protected:
        /**
         * @brief Constructor
         * @note Called once when the Overlay gets loaded
         */
        Overlay() {}
    public:
        /**
         * @brief Deconstructor
         * @note Called once when the Overlay exits
         * 
         */
        virtual ~Overlay() {}

        /**
         * @brief Initializes services
         * @note Called once at the start to initializes services. You have a sm session available during this call, no need to initialize sm yourself
         */
        virtual void initServices() {} 

        /**
         * @brief Exits services
         * @note Make sure to exit all services you initialized in \ref Overlay::initServices() here to prevent leaking handles
         */
        virtual void exitServices() {}

        /**
         * @brief Called before overlay changes from invisible to visible state
         * 
         */
        virtual void onShow() {}

        /**
         * @brief Called before overlay changes from visible to invisible state
         * 
         */
        virtual void onHide() {}

        /**
         * @brief Loads the default Gui
         * @note This function should return the initial Gui to load using the \ref Gui::initially<T>(Args.. args) function
         *       e.g `return initially<GuiMain>();`
         * 
         * @return Default Gui
         */
        virtual std::unique_ptr<tsl::Gui> loadInitialGui() = 0;

        /**
         * @brief Gets a reference to the current Gui on top of the Gui stack
         * 
         * @return Current Gui reference
         */
        virtual std::unique_ptr<tsl::Gui>& getCurrentGui() final {
            return this->m_guiStack.top();
        }

        /**
         * @brief Shows the Gui
         * 
         */
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

        /**
         * @brief Hides the Gui
         * 
         */
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

        /**
         * @brief Returns whether fade animation is playing
         *
         * @return whether fade animation is playing
         */
        virtual bool fadeAnimationPlaying() final {
            return this->m_fadeInAnimationPlaying || this->m_fadeOutAnimationPlaying;
        }

        /**
         * @brief Closes the Gui
         * @note This makes the Tesla overlay exit and return back to the Tesla-Menu
         * 
         */
        virtual void close() final {
            this->m_shouldClose = true;
        }

        /**
         * @brief Gets the Overlay instance
         * 
         * @return Overlay instance
         */
        static inline Overlay* const get() {
            return Overlay::s_overlayInstance;
        }

        /**
         * @brief Creates the initial Gui of an Overlay and moves the object to the Gui stack
         * 
         * @tparam T 
         * @tparam Args 
         * @param args 
         * @return constexpr std::unique_ptr<T> 
         */
        template<typename T, typename ... Args>
        constexpr inline std::unique_ptr<T> initially(Args&&... args) {
            return std::move(std::make_unique<T>(args...));
        }

    private:
        using GuiPtr = std::unique_ptr<tsl::Gui>;
        std::stack<GuiPtr, std::list<GuiPtr>> m_guiStack;
        static inline Overlay *s_overlayInstance = nullptr;
        
        bool m_fadeInAnimationPlaying = true, m_fadeOutAnimationPlaying = false;
        u8 m_animationCounter = 0;

        bool m_shouldHide = false;
        bool m_shouldClose = false;

        bool m_disableNextAnimation = false;

        bool m_closeOnExit;

        /**
         * @brief Initializes the Renderer
         * 
         */
        virtual void initScreen() final {
            gfx::Renderer::get().init();
        }

        /**
         * @brief Exits the Renderer
         * 
         */
        virtual void exitScreen() final {
            gfx::Renderer::get().exit();
        }

        /**
         * @brief Weather or not the Gui should get hidden
         * 
         * @return should hide 
         */
        virtual bool shouldHide() final {
            return this->m_shouldHide;
        }

        /**
         * @brief Weather or not hte Gui should get closed
         * 
         * @return should close
         */
        virtual bool shouldClose() final {
            return this->m_shouldClose;
        }

        /**
         * @brief Handles fade in and fade out animations of the Overlay
         * 
         */
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

        /**
         * @brief Main loop
         * 
         */
        virtual void loop() final {
            auto& renderer = gfx::Renderer::get();

            renderer.startFrame();


            this->animationLoop();
            this->getCurrentGui()->update();
            this->getCurrentGui()->draw(&renderer);

            renderer.endFrame();
        }

        /**
         * @brief Called once per frame with the latest HID inputs
         * 
         * @param keysDown Buttons pressed in the last frame
         * @param keysHeld Buttons held down longer than one frame
         * @param touchInput Last touch position
         * @param leftJoyStick Left joystick position
         * @param rightJoyStick Right joystick position
         * @return Weather or not the input has been consumed
         */
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

            parentElement = currentFocus;
            do {
                handled = parentElement->handleInput(keysDown, keysHeld, touchPos, joyStickPosLeft, joyStickPosRight);
                parentElement = parentElement->getParent();
            } while (!handled && parentElement != nullptr);

            if (currentGui != this->getCurrentGui())
                return;

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

        /**
         * @brief Clears the screen
         * 
         */
        virtual void clearScreen() final {
            auto& renderer = gfx::Renderer::get();

            renderer.startFrame();
            renderer.clearScreen();
            renderer.endFrame();
        }

        /**
         * @brief Reset hide and close flags that were previously set by \ref Overlay::close() or \ref Overlay::hide()
         * 
         */
        virtual void resetFlags() final {
            this->m_shouldHide = false;
            this->m_shouldClose = false;
        }

        /**
         * @brief Disables the next animation that would play
         * 
         */
        virtual void disableNextAnimation() final {
            this->m_disableNextAnimation = true;
        }

        /**
         * @brief Changes to a different Gui
         * 
         * @param gui Gui to change to
         * @return Reference to the Gui
         */
        std::unique_ptr<tsl::Gui>& changeTo(std::unique_ptr<tsl::Gui>&& gui) {
            gui->m_topElement = gui->createUI();
            gui->requestFocus(gui->m_topElement, FocusDirection::None);

            this->m_guiStack.push(std::move(gui));

            return this->m_guiStack.top();
        }

        /**
         * @brief Creates a new Gui and changes to it
         * 
         * @tparam G Gui to create
         * @tparam Args Arguments to pass to the Gui
         * @param args Arguments to pass to the Gui
         * @return Reference to the newly created Gui
         */
        template<typename G, typename ...Args>
        std::unique_ptr<tsl::Gui>& changeTo(Args&&... args) {
            return this->changeTo(std::make_unique<G>(std::forward<Args>(args)...));
        }

        /**
         * @brief Pops the top Gui from the stack and goes back to the last one
         * @note The Overlay gets closes once there are no more Guis on the stack
         */
        void goBack() {
            if (!this->m_closeOnExit && this->m_guiStack.size() == 1) {
                this->hide();
                return;
            }

            if (!this->m_guiStack.empty())
                this->m_guiStack.pop();

            if (this->m_guiStack.empty())
                this->close();
        }

        template<typename G, typename ...Args>
        friend std::unique_ptr<tsl::Gui>& changeTo(Args&&... args);

        friend void goBack();

        template<typename, tsl::impl::LaunchFlags>
        friend int loop(int argc, char** argv);

        friend class tsl::Gui;
    };

    
    namespace impl {
        
        /**
         * @brief Data shared between the different threads
         * 
         */
        struct SharedThreadData {
            bool running = false;

            Event comboEvent = { 0 }, homeButtonPressEvent = { 0 }, powerButtonPressEvent = { 0 };

            bool overlayOpen = false;

            std::mutex dataMutex;
            u64 keysDown = 0;
            u64 keysDownPending = 0;
            u64 keysHeld = 0;
            touchPosition touchPos = { 0 };
            JoystickPosition joyStickPosLeft = { 0 }, joyStickPosRight = { 0 };
        };


        /**
         * @brief Extract values from Tesla settings file
         * 
         * @param[out] keyCombo Overlay launch button combo
         */
        static void parseOverlaySettings(u64 &keyCombo) {
            hlp::ini::IniData parsedConfig = hlp::ini::readOverlaySettings();

            u64 decodedKeys = hlp::comboStringToKeys(parsedConfig["tesla"]["key_combo"]);
            if (decodedKeys)
                keyCombo = decodedKeys;
        }

        /**
         * @brief Update and save launch combo keys
         * 
         * @param keys the new combo keys
         */
        [[maybe_unused]] static void updateCombo(u64 keys) {
            tsl::cfg::launchCombo = keys;
            hlp::ini::updateOverlaySettings({
                { "tesla", {
                    { "key_combo", tsl::hlp::keysToComboString(keys) }
                }}
            });
        }

        /**
         * @brief Input polling loop thread
         * 
         * @tparam launchFlags Launch flags
         * @param args Used to pass in a pointer to a \ref SharedThreadData struct
         */
        template<impl::LaunchFlags launchFlags>
        static void hidInputPoller(void *args) {
            SharedThreadData *shData = static_cast<SharedThreadData*>(args);
            
            // Parse Tesla settings
            impl::parseOverlaySettings(tsl::cfg::launchCombo);

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
                    shData->joyStickPosLeft  = { 0 };
                    shData->joyStickPosRight = { 0 };

                    // Combine input from all controllers
                    for (u8 controller = 0; controller < 8; controller++) {
                        if (hidIsControllerConnected(static_cast<HidControllerID>(controller))) {
                            shData->keysDown |= hidKeysDown(static_cast<HidControllerID>(controller));
                            shData->keysHeld |= hidKeysHeld(static_cast<HidControllerID>(controller));

                            JoystickPosition joyStickPosLeft, joyStickPosRight;
                            hidJoystickRead(&joyStickPosLeft, static_cast<HidControllerID>(controller), HidControllerJoystick::JOYSTICK_LEFT);
                            hidJoystickRead(&joyStickPosRight, static_cast<HidControllerID>(controller), HidControllerJoystick::JOYSTICK_RIGHT);

                            if (joyStickPosLeft.dx > 0 && joyStickPosLeft.dx > shData->joyStickPosLeft.dx)
                                shData->joyStickPosLeft.dx = joyStickPosLeft.dx;
                            if (joyStickPosLeft.dx < 0 && joyStickPosLeft.dx < shData->joyStickPosLeft.dx)
                                shData->joyStickPosLeft.dx = joyStickPosLeft.dx;
                            if (joyStickPosLeft.dy > 0 && joyStickPosLeft.dy > shData->joyStickPosLeft.dy)
                                shData->joyStickPosLeft.dy = joyStickPosLeft.dy;
                            if (joyStickPosLeft.dy < 0 && joyStickPosLeft.dy < shData->joyStickPosLeft.dy)
                                shData->joyStickPosLeft.dy = joyStickPosLeft.dy;
                        }
                    }

                    if (hidIsControllerConnected(CONTROLLER_HANDHELD)) {
                        shData->keysDown |= hidKeysDown(CONTROLLER_HANDHELD);
                        shData->keysHeld |= hidKeysHeld(CONTROLLER_HANDHELD);

                        hidJoystickRead(&shData->joyStickPosLeft, CONTROLLER_HANDHELD, HidControllerJoystick::JOYSTICK_LEFT);
                        hidJoystickRead(&shData->joyStickPosRight, CONTROLLER_HANDHELD, HidControllerJoystick::JOYSTICK_RIGHT);
                    }

                    // Read in touch positions
                    if (hidTouchCount() > 0)
                        hidTouchRead(&shData->touchPos, 0);
                    else 
                        shData->touchPos = { 0 };

                    if (((shData->keysHeld & tsl::cfg::launchCombo) == tsl::cfg::launchCombo) && shData->keysDown & tsl::cfg::launchCombo) {
                        if (shData->overlayOpen) {
                            tsl::Overlay::get()->hide();
                            shData->overlayOpen = false;
                        }
                        else
                            eventFire(&shData->comboEvent);
                    }

                    if (shData->touchPos.px >= cfg::FramebufferWidth && shData->overlayOpen) {
                        if (shData->overlayOpen) {
                            tsl::Overlay::get()->hide();
                            shData->overlayOpen = false;
                        }
                    }

                    shData->keysDownPending |= shData->keysDown;
                }

                //20 ms
                svcSleepThread(20E6);
            }
        }

        /**
         * @brief Home button detection loop thread
         * @note This makes sure that focus cannot glitch out when pressing the home button
         * 
         * @param args Used to pass in a pointer to a \ref SharedThreadData struct
         */
        static void homeButtonDetector(void *args) {
            SharedThreadData *shData = static_cast<SharedThreadData*>(args);

            // To prevent focus glitchout, close the overlay immediately when the home button gets pressed
            hidsysAcquireHomeButtonEventHandle(&shData->homeButtonPressEvent);
            eventClear(&shData->homeButtonPressEvent);

            while (shData->running) {
                if (R_SUCCEEDED(eventWait(&shData->homeButtonPressEvent, 100'000'000))) {
                    eventClear(&shData->homeButtonPressEvent);

                    if (shData->overlayOpen) {
                        tsl::Overlay::get()->hide();
                        shData->overlayOpen = false;
                    }
                }
            }

        }

        /**
         * @brief Power button detection loop thread
         * @note This makes sure that focus cannot glitch out when pressing the power button
         * 
         * @param args Used to pass in a pointer to a \ref SharedThreadData struct
         */
        static void powerButtonDetector(void *args) {
            SharedThreadData *shData = static_cast<SharedThreadData*>(args);

            // To prevent focus glitchout, close the overlay immediately when the power button gets pressed
            hidsysAcquireSleepButtonEventHandle(&shData->powerButtonPressEvent);
            eventClear(&shData->powerButtonPressEvent);

            while (shData->running) {
                if (R_SUCCEEDED(eventWait(&shData->powerButtonPressEvent, 100'000'000))) {
                    eventClear(&shData->powerButtonPressEvent);

                    if (shData->overlayOpen) {
                        tsl::Overlay::get()->hide();
                        shData->overlayOpen = false;
                    }
                }
            }

        }

    }

    /**
     * @brief Creates a new Gui and changes to it
     * 
     * @tparam G Gui to create 
     * @tparam Args Arguments to pass to the Gui
     * @param args Arguments to pass to the Gui
     * @return Reference to the newly created Gui
     */
    template<typename G, typename ...Args>
    std::unique_ptr<tsl::Gui>& changeTo(Args&&... args) {
        return Overlay::get()->changeTo<G, Args...>(std::forward<Args>(args)...);
    }

    /**
     * @brief Pops the top Gui from the stack and goes back to the last one
     * @note The Overlay gets closes once there are no more Guis on the stack
     */
    static void goBack() {
        Overlay::get()->goBack();
    }

    static void setNextOverlay(const std::string& ovlPath, std::string args) {

        args += " --skipCombo";

        envSetNextLoad(ovlPath.c_str(), args.c_str());
    }



    /**
     * @brief libtesla's main function
     * @note Call it directly from main passing in argc and argv and returning it e.g `return tsl::loop<OverlayTest>(argc, argv);`
     * 
     * @tparam TOverlay Your overlay class
     * @tparam launchFlags \ref LaunchFlags
     * @param argc argc
     * @param argv argv
     * @return int result
     */
    template<typename TOverlay, impl::LaunchFlags launchFlags>   
    static inline int loop(int argc, char** argv) {
        static_assert(std::is_base_of_v<tsl::Overlay, TOverlay>, "tsl::loop expects a type derived from tsl::Overlay");

        impl::SharedThreadData shData;

        shData.running = true;

        Thread hidPollerThread, homeButtonDetectorThread, powerButtonDetectorThread;
        threadCreate(&hidPollerThread, impl::hidInputPoller<launchFlags>, &shData, nullptr, 0x1000, 0x10, -2);
        threadCreate(&homeButtonDetectorThread, impl::homeButtonDetector, &shData, nullptr, 0x1000, 0x2C, -2);
        threadCreate(&powerButtonDetectorThread, impl::powerButtonDetector, &shData, nullptr, 0x1000, 0x2C, -2);
        threadStart(&hidPollerThread);
        threadStart(&homeButtonDetectorThread);
        threadStart(&powerButtonDetectorThread);

        eventCreate(&shData.comboEvent, false);



        auto& overlay = tsl::Overlay::s_overlayInstance;
        overlay = new TOverlay();
        overlay->m_closeOnExit = (u8(launchFlags) & u8(impl::LaunchFlags::CloseOnExit)) == u8(impl::LaunchFlags::CloseOnExit);

        tsl::hlp::doWithSmSession([&overlay]{ overlay->initServices(); });
        overlay->initScreen();
        overlay->changeTo(overlay->loadInitialGui());

        
        // Argument parsing
        for (u8 arg = 0; arg < argc; arg++) {
            if (strcasecmp(argv[arg], "--skipCombo") == 0) {
                eventFire(&shData.comboEvent);
                overlay->disableNextAnimation();
            }
        }


        while (shData.running) {
            
            eventWait(&shData.comboEvent, UINT64_MAX);
            eventClear(&shData.comboEvent);
            shData.overlayOpen = true;
            

            hlp::requestForeground(true);

            overlay->show();
            overlay->clearScreen();

            while (shData.running) {
                overlay->loop();

                {
                    std::scoped_lock lock(shData.dataMutex);
                    if (!overlay->fadeAnimationPlaying()) {
                        overlay->handleInput(shData.keysDownPending, shData.keysHeld, shData.touchPos, shData.joyStickPosLeft, shData.joyStickPosRight);
                    }
                    shData.keysDownPending = 0;
                }

                if (overlay->shouldHide())
                    break;
                
                if (overlay->shouldClose())
                    shData.running = false;
            }

            overlay->clearScreen();
            overlay->resetFlags();

            hlp::requestForeground(false);

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

        overlay->exitScreen();
        overlay->exitServices();

        delete overlay;
        
        return 0;
    }

}


#ifdef TESLA_INIT_IMPL

namespace tsl::cfg {

    u16 LayerWidth  = 0;
    u16 LayerHeight = 0;
    u16 LayerPosX   = 0;
    u16 LayerPosY   = 0;
    u16 FramebufferWidth  = 0;
    u16 FramebufferHeight = 0;
    u64 launchCombo = KEY_L | KEY_DDOWN | KEY_RSTICK;

}

extern "C" {

    u32 __nx_applet_type = AppletType_None;
    u32  __nx_nv_transfermem_size = 0x40000;
    ViLayerFlags __nx_vi_stray_layer_flags = (ViLayerFlags)0;

    /**
     * @brief libtesla service initializing function to override libnx's
     * 
     */
    void __appInit(void) {
        tsl::hlp::doWithSmSession([]{
            ASSERT_FATAL(fsInitialize());
            ASSERT_FATAL(hidInitialize());      // Controller inputs and Touch
            ASSERT_FATAL(plInitialize());       // Font data
            ASSERT_FATAL(pmdmntInitialize());   // PID querying
            ASSERT_FATAL(hidsysInitialize());   // Focus control
            ASSERT_FATAL(setsysInitialize());   // Settings querying
        });
    }

    /**
     * @brief libtesla service exiting function to override libnx's
     * 
     */
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
