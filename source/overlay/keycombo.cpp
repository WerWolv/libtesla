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

#include <sstream>

#include "helpers/config.hpp"

#include "overlay/keycombo.hpp"

namespace tsl {

    const u64 KeyCombo::DEFAULT_KEYS = KEY_L | KEY_DDOWN | KEY_RSTICK;

    // Will be displayed in this order
    const std::list<keyinfo_t> KeyCombo::KEYS_INFO = {
        { KEY_L, "L", "\uE0A4" }, { KEY_R, "R", "\uE0A5" }, { KEY_ZL, "ZL", "\uE0A6" }, { KEY_ZR, "ZR", "\uE0A7" },
        { KEY_DLEFT, "DLEFT", "\uE07B" }, { KEY_DUP, "DUP", "\uE079" }, { KEY_DRIGHT, "DRIGHT", "\uE07C" }, { KEY_DDOWN, "DDOWN", "\uE07A" },
        { KEY_A, "A", "\uE0A0" }, { KEY_B, "B", "\uE0A1" }, { KEY_X, "X", "\uE0A2" }, { KEY_Y, "Y", "\uE0A3" },
        { KEY_LSTICK, "LSTICK", "\uE08A" }, { KEY_RSTICK, "RSTICK", "\uE08B" },
        { KEY_MINUS, "MINUS", "\uE0B6" }, { KEY_PLUS, "PLUS", "\uE0B5" }
    };

    u64 KeyCombo::s_comboKeys = loadConfig();

    bool KeyCombo::isComboActive(u64 keysHeld, u64 keysDown) {
        // Check for new keys to prevent repeated activation
        auto allKeysPressed = (keysHeld & s_comboKeys) == s_comboKeys;
        auto newKeyPressed = keysDown & s_comboKeys;
        return allKeysPressed && newKeyPressed;
    }

    u64 KeyCombo::loadConfig() {
        auto j = Config::read();
        auto it = j.find("comboKeys");
        if (it != j.end()) {
            u64 keys = parse(*it);
            if (validate(keys)) {
                s_comboKeys = keys;
                return keys;
            }
        }
        update(DEFAULT_KEYS);
        return DEFAULT_KEYS;
    }

    bool KeyCombo::update(u64 comboKeys) {
        bool isValid = validate(comboKeys);
        if (isValid) {
            Config::update([comboKeys](json &j) {
                j["comboKeys"] = stringify(comboKeys);
            });
            s_comboKeys = comboKeys;
        }
        return isValid;
    }

    bool KeyCombo::validate(u64 comboKeys) {
        u64 allowedKeys = 0;
        for (auto i : KEYS_INFO) {
            allowedKeys |= i.key;
        }
        // keys not empty and no disallowed keys used
        return comboKeys && (comboKeys & ~allowedKeys) == 0;
    }

    std::string KeyCombo::stringify(u64 comboKeys) {
        std::string str;
        for (auto i : KEYS_INFO) {
            if (comboKeys & i.key) {
                if (!str.empty())
                    str.append("+");
                str.append(i.name);
            }
        }
        return str;
    }

    u64 KeyCombo::parse(std::string comboString) {
        u64 keyCombo = 0;
        std::istringstream is(comboString);
        std::string token;
        while (std::getline(is, token, '+')) {
            auto it = std::find_if(KEYS_INFO.begin(), KEYS_INFO.end(), [token](keyinfo_t i) {
                return i.name == token;
            });
            if (it == KEYS_INFO.end()) return 0;
            keyCombo |= (*it).key;
        }
        return keyCombo;
    }

}
