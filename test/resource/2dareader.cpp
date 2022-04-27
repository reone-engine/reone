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

#include "../../src/common/streamwriter.h"
#include "../../src/resource/2da.h"
#include "../../src/resource/format/2dareader.h"

using namespace std;

using namespace reone;
using namespace reone::resource;

BOOST_AUTO_TEST_SUITE(two_da_reader)

BOOST_AUTO_TEST_CASE(should_read_two_da) {
    // given

    auto ss = ostringstream();
    ss << "2DA V2.b";
    ss << string("\x0a", 1);
    ss << string("key\x09", 4);
    ss << string("value\x09", 6);
    ss << string("\x00", 1);
    ss << string("\x02\x00\x00\x00", 4);
    ss << string("\x30\x09\x31\x09", 4);
    ss << string("\x00\x00", 2);
    ss << string("\x07\x00", 2);
    ss << string("\x07\x00", 2);
    ss << string("\x07\x00", 2);
    ss << string("\x0c\x00", 2);
    ss << string("unique\x00", 7);
    ss << string("same\x00", 5);

    auto stream = make_shared<istringstream>(ss.str());
    auto reader = TwoDaReader();

    // when

    reader.load(stream);

    // then

    auto twoDa = reader.twoDa();
    BOOST_CHECK_EQUAL(twoDa->getColumnCount(), 2);
    BOOST_CHECK_EQUAL(twoDa->getRowCount(), 2);
    BOOST_CHECK_EQUAL(string("unique"), twoDa->getString(0, "key"));
    BOOST_CHECK_EQUAL(string("same"), twoDa->getString(0, "value"));
    BOOST_CHECK_EQUAL(string("same"), twoDa->getString(1, "key"));
    BOOST_CHECK_EQUAL(string("same"), twoDa->getString(1, "value"));
}

BOOST_AUTO_TEST_SUITE_END()
