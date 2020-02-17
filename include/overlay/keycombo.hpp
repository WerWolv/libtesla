/**
 * Copyright (C) 2020 diwo
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

#include <string>
#include <list>

namespace tsl {

    struct keyinfo_t {
        u64 key;
        const char* name;
        const char* glyph;
        inline keyinfo_t(u64 key, const char* name, const char* glyph)
        : key(key), name(name), glyph(glyph) {}
    };

    class KeyCombo {
    public:
        static const u64 DEFAULT_KEYS;
        static const std::list<keyinfo_t> KEYS_INFO;

    public:
        static bool isComboActive(u64 keysHeld, u64 keysDown);
        static u64 loadConfig();
        static bool update(u64 comboKeys);

    protected:
        static bool validate(u64 comboKeys);
        static std::string stringify(u64 comboKeys);
        static u64 parse(std::string comboString);

    private:
        KeyCombo() {}

    private:
        static u64 s_comboKeys;
    };

}

