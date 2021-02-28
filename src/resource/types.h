/*
 * Copyright (c) 2020-2021 The reone project contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace reone {

namespace resource {

enum class GameID {
    KotOR,
    TSL
};

/**
 * Used together with a ResRef to locate game resources.
 */
enum class ResourceType : uint16_t {
    Res = 0,
    Bmp = 1,
    Tga = 3,
    Wav = 4,
    Plt = 6,
    Ini = 7,
    Txt = 10,
    Mdl = 2002,
    Nss = 2009,
    Ncs = 2010,
    Are = 2012,
    Set = 2013,
    Ifo = 2014,
    Bic = 2015,
    Wok = 2016,
    TwoDa = 2017,
    Txi = 2022,
    Git = 2023,
    Bti = 2024,
    Uti = 2025,
    Btc = 2026,
    Utc = 2027,
    Dlg = 2029,
    Itp = 2030,
    Utt = 2032,
    Dds = 2033,
    Uts = 2035,
    Ltr = 2036,
    Gff = 2037,
    Fac = 2038,
    Ute = 2040,
    Utd = 2042,
    Utp = 2044,
    Dft = 2045,
    Gic = 2046,
    Gui = 2047,
    Utm = 2051,
    Dwk = 2052,
    Pwk = 2053,
    Jrl = 2056,
    Mod = 2057,
    Utw = 2058,
    Ssf = 2060,
    Ndb = 2064,
    Ptm = 2065,
    Ptt = 2066,
    Lyt = 3000,
    Vis = 3001,
    Pth = 3003,
    Lip = 3004,
    Tpc = 3007,
    Mdx = 3008,

    Mp3 = 0x1000,
    Gr2 = 0x1001, // SWTOR model
    Mat = 0x1002, // SWTOR material
    Jba = 0x1003, // SWTOR animation

    Invalid = 0xffff
};

typedef std::multimap<std::string, std::string> Visibility;

} // namespace resource

} // namespace reone
