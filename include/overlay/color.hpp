/**
 * Copyright (C) 2019 averne
 * Copyright (C) 2019 WerWolv
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

#include <cstdint>
#include <utility>
#include <type_traits>

namespace tsl {

    struct rgb565_t {
        union {
            struct {
                u16 r: 5, g: 6, b: 5;
            } PACKED;
            u16 rgb;
        };
        inline rgb565_t(u16 raw): rgb(raw) { }
        inline rgb565_t(u8 r, u8 g, u8 b): r(r), g(g), b(b) { }
    };

    struct rgba4444_t {
        union {
            struct {
                u16 r: 4, g: 4, b: 4, a: 4;
            } PACKED;
            u16 rgba;
        };
        inline rgba4444_t(u16 raw): rgba(raw) { }
        inline rgba4444_t(u8 r, u8 g, u8 b, u8 a): r(r), g(g), b(b), a(a) { }
    };

    template<typename T, typename=void>
    struct hasAlpha : std::false_type { };

    template<typename T>
    struct hasAlpha<T, decltype(std::declval<T>().a, void())> : std::true_type { };

    template<typename T>
    inline constexpr bool hasAlpha_v = hasAlpha<T>::value;

    template<typename T>
    struct col_underlying_type { using type = decltype(T::r); };

    template<typename T>
    using col_underlying_type_t = typename col_underlying_type<T>::type;

    template<typename T>
    inline T makeColor(col_underlying_type_t<T> raw) {
        return T(raw);
    }

    template<typename T>
    typename std::enable_if_t<!hasAlpha_v<T>, T>
    inline makeColor(decltype(T::r) r, decltype(T::g) g, decltype(T::b) b) {
        return T(r, g, b);
    }

    template<typename T>
    typename std::enable_if_t<hasAlpha_v<T>, T>
    inline makeColor(decltype(T::r) r, decltype(T::g) g, decltype(T::b) b, decltype(T::a) a=-1) {
        return T(r, g, b, a);
    }

    #define BLEND_CHANNEL(x, y, a) (((a) * (x) + ((0xFF - (a)) * (y))) / 0xFF)

    template<typename T>
    typename std::enable_if_t<!hasAlpha_v<T>, T>
    inline blend(T x, T y, u8 alpha) {
        return makeColor<T>(
            BLEND_CHANNEL(x.r, y.r, alpha),
            BLEND_CHANNEL(x.g, y.g, alpha),
            BLEND_CHANNEL(x.b, y.b, alpha)
        );
    }

    template<typename T>
    typename std::enable_if_t<hasAlpha_v<T>, T>
    inline blend(T x, T y, u8 alpha) {
        return makeColor<T>(
            BLEND_CHANNEL(x.r, y.r, alpha),
            BLEND_CHANNEL(x.g, y.g, alpha),
            BLEND_CHANNEL(x.b, y.b, alpha),
            BLEND_CHANNEL(1,   y.a, alpha)
        );
    }
    
}

