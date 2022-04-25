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

#include "../src/common/streamwriter.h"
#include "../src/resource/2da.h"
#include "../src/resource/format/2dawriter.h"

#include "checkutil.h"

using namespace std;

using namespace reone;
using namespace reone::resource;

BOOST_AUTO_TEST_SUITE(two_da_writer)

BOOST_AUTO_TEST_CASE(should_write_two_da) {
    // given

    auto twoDa = make_shared<TwoDA>();
    twoDa->addColumn("key");
    twoDa->addColumn("value");
    twoDa->add(TwoDA::Row {vector<string> {"@", "A"}});

    auto writer = TwoDaWriter(twoDa);
    auto stream = make_shared<ostringstream>();

    auto ss = ostringstream();
    ss << "2DA V2.b";
    ss << string("\x0a", 1);
    ss << string("key\x09", 4);
    ss << string("value\x09", 6);
    ss << string("\x00", 1);
    ss << string("\x01\x00\x00\x00", 4);
    ss << string("\x30\x09", 2);
    ss << string("\x00\x00", 2);
    ss << string("\x02\x00", 2);
    ss << string("\x04\x00", 2);
    ss << string("\x40\x00", 2);
    ss << string("\x41\x00", 2);
    auto expectedOutput = ss.str();

    // when

    writer.save(stream);

    // then

    auto actualOutput = stream->str();
    BOOST_TEST(expectedOutput == actualOutput, notEqualMessage(expectedOutput, actualOutput));
}

BOOST_AUTO_TEST_SUITE_END()
