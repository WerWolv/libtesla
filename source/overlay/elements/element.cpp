/**
 * Copyright (C) 2020 natinusala
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

#include <algorithm>

#include "overlay/elements/element.hpp"
#include "overlay/gui/gui.hpp"

namespace tsl::ovl::element {
    
    void Element::frame(ovl::Screen *screen) {
        if (gui::Gui::isFocused(this))
            this->drawFocus(screen);

        this->draw(screen, this->m_x, this->m_y);
    }
    
    int shakeAnimation(std::chrono::system_clock::duration t, float a) {
        float w = 0.2F;
        float tau = 0.05F;

        int t_ = t.count() / 1'000'000;

        return roundf(a * exp(-(tau * t_) * sin(w * t_)));
    }

    void Element::drawFocus(ovl::Screen *screen) {
        static float counter = 0;
        const float progress = (std::sin(counter) + 1) / 2;
        rgba4444_t highlightColor = {   static_cast<u8>((0x2 - 0x8) * progress + 0x8),
                                        static_cast<u8>((0x8 - 0xF) * progress + 0xF), 
                                        static_cast<u8>((0xC - 0xF) * progress + 0xF), 
                                        0xF };

        counter += 0.2F;

        s32 x = 0, y = 0;

        if (this->m_highlightShaking) {
            auto t = (std::chrono::system_clock::now() - this->m_highlightShakingStartTime);
            if (t >= 100ms)
                this->m_highlightShaking = false;
            else {
                s32 amplitude = std::rand() % 5 + 5;

                switch (this->m_highlightShakingDirection) {
                    case FocusDirection::UP:
                        y -= shakeAnimation(t, amplitude);
                        break;
                    case FocusDirection::DOWN:
                        y += shakeAnimation(t, amplitude);
                        break;
                    case FocusDirection::LEFT:
                        x -= shakeAnimation(t, amplitude);
                        break;
                    case FocusDirection::RIGHT:
                        x += shakeAnimation(t, amplitude);
                        break;
                    default:
                        break;
                }

                x = std::clamp(x, -amplitude, amplitude);
                y = std::clamp(y, -amplitude, amplitude);
            }
        }

        screen->drawRect(this->m_x, this->m_y, this->m_width, this->m_height, a(0xF000));

        screen->drawRect(this->m_x + x - 4, this->m_y + y - 4, this->m_width + 8, 4, a(highlightColor));
        screen->drawRect(this->m_x + x - 4, this->m_y + y + this->m_height, this->m_width + 8, 4, a(highlightColor));
        screen->drawRect(this->m_x + x - 4, this->m_y + y, 4, this->m_height, a(highlightColor));
        screen->drawRect(this->m_x + x + this->m_width, this->m_y + y, 4, this->m_height, a(highlightColor));

    }

    void Element::shakeFocus(FocusDirection direction) {
        this->m_highlightShaking = true;
        this->m_highlightShakingDirection = direction;
        this->m_highlightShakingStartTime = std::chrono::system_clock::now();
    }

}