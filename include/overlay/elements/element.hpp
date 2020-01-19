/**
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

#include "overlay/screen.hpp"
#include <memory>
#include <utility>
#include <cmath>
#include <chrono>

enum class FocusDirection { NONE, UP, DOWN, LEFT, RIGHT };

namespace tsl::element {
    
    using namespace std::literals::chrono_literals;

    class Element {
    public:
        Element() { }
        virtual ~Element() { }

        virtual Element* requestFocus(Element *oldFocus, FocusDirection direction) = 0;
        
        virtual bool onClick(s64 key) {
            return false;
        }

        virtual bool onTouch(u32 x, u32 y) {
            return false;
        }

        void shakeFocus(FocusDirection direction);

        virtual void draw(Screen *screen, u16 x, u16 y) = 0;
        virtual void layout() = 0;

        void frame(Screen *screen);

        virtual void drawFocus(Screen *screen);

        void setParent(Element *parent) { this->m_parent = parent; }
        Element* getParent() { return this->m_parent; }

        constexpr void setPosition(u16 x, u16 y) { this->m_x = x; this->m_y = y; }
        constexpr std::pair<u16, u16> getPosition() { return { this->m_x, this->m_y }; }

        constexpr void setSize(u16 width, u16 height) { this->m_width = width; this->m_height = height; }
        constexpr std::pair<u16, u16> getSize() { return { this->m_width, this->m_height }; }

        void setOpacity(float opacity) { this->m_opacity = opacity; applyOpacity(opacity); }


    protected:
        virtual void applyOpacity(float opacity) { }

        rgba4444_t a(const rgba4444_t &c) {
            return Screen::a((c.rgba & 0x0FFF) | (static_cast<u8>(c.a * Element::m_opacity) << 12));
        }

    private:
        Element *m_parent = nullptr;
        float m_opacity = 1.0F;

        u16 m_x = 0, m_y = 0, m_width = 0, m_height = 0;

        // Highlight shake animation
        bool m_highlightShaking = false;
        std::chrono::system_clock::time_point m_highlightShakingStartTime;
        FocusDirection m_highlightShakingDirection;

    };

}