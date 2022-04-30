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

#include <boost/test/unit_test.hpp>

#include "../../src/common/stringbuilder.h"
#include "../../src/resource/format/erfwriter.h"

#include "../checkutil.h"

using namespace std;

using namespace reone;
using namespace reone::resource;

BOOST_AUTO_TEST_SUITE(erf_writer)

BOOST_AUTO_TEST_CASE(should_write_erf) {
    // given

    auto expectedOutput = StringBuilder()
                              // header
                              .append("ERF V1.0")
                              .append("\x00\x00\x00\x00", 4) // number of languages
                              .append("\x00\x00\x00\x00", 4) // size of localized strings
                              .append("\x01\x00\x00\x00", 4) // number of entries
                              .append("\xa0\x00\x00\x00", 4) // offset to localized strings
                              .append("\xa0\x00\x00\x00", 4) // offset to key list
                              .append("\xb8\x00\x00\x00", 4) // offset to resource list
                              .append("\x00\x00\x00\x00", 4) // build year
                              .append("\x00\x00\x00\x00", 4) // build day
                              .append("\xff\xff\xff\xff", 4) // description strref
                              .repeat('\x00', 116)           // reserved
                              // key list
                              .append("Aa\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 16) // resref
                              .append("\x00\x00\x00\x00", 4)                                            // resid
                              .append("\xe6\x07", 2)                                                    // restype
                              .append("\x00\x00", 2)                                                    // unused
                              // resource list
                              .append("\xc0\x00\x00\x00", 4) // offset to resource
                              .append("\x02\x00\x00\x00", 4) // resource size
                              // resource data
                              .append("Bb")
                              .build();

    auto writer = ErfWriter();
    writer.add(ErfWriter::Resource {"Aa", ResourceType::Txi, ByteArray {'B', 'b'}});

    auto erf = make_shared<ostringstream>();

    // when

    writer.save(ErfWriter::FileType::ERF, erf);

    // then

    auto actualOutput = erf->str();
    BOOST_TEST((expectedOutput == actualOutput), notEqualMessage(expectedOutput, actualOutput));
}

BOOST_AUTO_TEST_SUITE_END()
