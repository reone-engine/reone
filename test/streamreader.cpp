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

#include "../src/common/streamreader.h"

using namespace std;

using namespace reone;

BOOST_AUTO_TEST_SUITE(stream_reader)

BOOST_AUTO_TEST_CASE(should_seek_ignore_and_tell_in_little_endian_stream) {
    // given
    auto input = make_shared<stringbuf>(string("Hello, world!\x00", 14));
    auto stream = make_shared<istream>(input.get());
    auto reader = StreamReader(stream, boost::endian::order::little);
    auto expectedPos = 7ll;

    // when
    reader.seek(5);
    reader.ignore(2);
    auto actualPos = reader.tell();

    // then
    BOOST_CHECK_EQUAL(expectedPos, actualPos);
}

BOOST_AUTO_TEST_CASE(should_get_unsigned_ints_from_little_endian_stream) {
    // given
    auto input = make_shared<stringbuf>(string("\xff\x01\xff\x02\xff\xff\xff\x03\xff\xff\xff\xff\xff\xff\xff", 15));
    auto stream = make_shared<istream>(input.get());
    auto reader = StreamReader(stream, boost::endian::order::little);
    auto expectedByte = 255u;
    auto expectedUint16 = 65281u;
    auto expectedUint32 = 4294967042u;
    auto expectedUint64 = 18446744073709551363u;

    // when
    auto actualByte = reader.getByte();
    auto actualUint16 = reader.getUint16();
    auto actualUint32 = reader.getUint32();
    auto actualUint64 = reader.getUint64();

    // then
    BOOST_CHECK_EQUAL(expectedByte, actualByte);
    BOOST_CHECK_EQUAL(expectedUint16, actualUint16);
    BOOST_CHECK_EQUAL(expectedUint32, actualUint32);
    BOOST_CHECK_EQUAL(expectedUint64, actualUint64);
}

BOOST_AUTO_TEST_CASE(should_get_signed_ints_from_little_endian_stream) {
    // given
    auto input = make_shared<stringbuf>(string("\x01\xff\x02\xff\xff\xff\x03\xff\xff\xff\xff\xff\xff\xff", 14));
    auto stream = make_shared<istream>(input.get());
    auto reader = StreamReader(stream, boost::endian::order::little);
    auto expectedInt16 = -255;
    auto expectedInt32 = -254;
    auto expectedInt64 = -253;

    // when
    auto actualInt16 = reader.getInt16();
    auto actualInt32 = reader.getInt32();
    auto actualInt64 = reader.getInt64();

    // then
    BOOST_CHECK_EQUAL(expectedInt16, actualInt16);
    BOOST_CHECK_EQUAL(expectedInt32, actualInt32);
    BOOST_CHECK_EQUAL(expectedInt64, actualInt64);
}

BOOST_AUTO_TEST_CASE(should_get_float_and_double_from_little_endian_stream) {
    // given
    auto input = make_shared<stringbuf>(string("\x00\x00\x80\x3f\x00\x00\x00\x00\x00\x00\xf0\x3f", 12));
    auto stream = make_shared<istream>(input.get());
    auto reader = StreamReader(stream, boost::endian::order::little);
    auto expectedFloat = 1.0f;
    auto expectedDouble = 1.0;

    // when
    auto actualFloat = reader.getFloat();
    auto actualDouble = reader.getDouble();

    // then
    BOOST_CHECK_EQUAL(expectedFloat, actualFloat);
    BOOST_CHECK_EQUAL(expectedDouble, actualDouble);
}

BOOST_AUTO_TEST_CASE(should_get_string_from_little_endian_stream) {
    // given
    auto input = make_shared<stringbuf>(string("Hello, world!", 13));
    auto stream = make_shared<istream>(input.get());
    auto reader = StreamReader(stream, boost::endian::order::little);
    auto expectedStr = string("Hello, world!");

    // when
    auto actualStr = reader.getString(13);

    // then
    BOOST_CHECK_EQUAL(expectedStr, actualStr);
}

BOOST_AUTO_TEST_CASE(should_get_null_terminated_string_from_little_endian_stream) {
    // given
    auto input = make_shared<stringbuf>(string("Hello, world!\x00", 14));
    auto stream = make_shared<istream>(input.get());
    auto reader = StreamReader(stream, boost::endian::order::little);
    auto expectedStr = string("Hello, world!");

    // when
    auto actualStr = reader.getNullTerminatedString();

    // then
    BOOST_CHECK_EQUAL(expectedStr, actualStr);
}

BOOST_AUTO_TEST_CASE(should_get_null_terminated_utf16_string_from_little_endian_stream) {
    // given
    auto input = make_shared<stringbuf>(string("\x48\x00\x65\x00\x6c\x00\x6c\x00\x6f\x00\x2c\x00\x20\x00\x77\x00\x6f\x00\x72\x00\x6c\x00\x64\x00\x21\x00\x00\x00", 28));
    auto stream = make_shared<istream>(input.get());
    auto reader = StreamReader(stream, boost::endian::order::little);
    auto expectedStr = u16string(u"Hello, world!");

    // when
    auto actualStr = reader.getNullTerminatedStringUTF16();

    // then
    BOOST_CHECK((expectedStr == actualStr));
}

BOOST_AUTO_TEST_CASE(should_get_bytes_from_little_endian_stream) {
    // given
    auto input = make_shared<stringbuf>(string("\x01\x02\x03\x04", 4));
    auto stream = make_shared<istream>(input.get());
    auto reader = StreamReader(stream, boost::endian::order::little);
    auto expectedBytes = ByteArray({ 0x01, 0x02, 0x03, 0x04 });

    // when
    auto actualBytes = reader.getBytes(4);

    // then
    BOOST_CHECK((expectedBytes == actualBytes));
}

BOOST_AUTO_TEST_CASE(should_get_unsigned_ints_from_big_endian_stream) {
    // given
    auto input = make_shared<stringbuf>(string("\xff\x01\xff\xff\xff\x02\xff\xff\xff\xff\xff\xff\xff\x03", 14));
    auto stream = make_shared<istream>(input.get());
    auto reader = StreamReader(stream, boost::endian::order::big);
    auto expectedUint16 = 65281u;
    auto expectedUint32 = 4294967042u;
    auto expectedUint64 = 18446744073709551363u;

    // when
    auto actualUint16 = reader.getUint16();
    auto actualUint32 = reader.getUint32();
    auto actualUint64 = reader.getUint64();

    // then
    BOOST_CHECK_EQUAL(expectedUint16, actualUint16);
    BOOST_CHECK_EQUAL(expectedUint32, actualUint32);
    BOOST_CHECK_EQUAL(expectedUint64, actualUint64);
}

BOOST_AUTO_TEST_CASE(should_get_signed_ints_from_big_endian_stream) {
    // given
    auto input = make_shared<stringbuf>(string("\xff\x01\xff\xff\xff\x02\xff\xff\xff\xff\xff\xff\xff\x03", 14));
    auto stream = make_shared<istream>(input.get());
    auto reader = StreamReader(stream, boost::endian::order::big);
    auto expectedInt16 = -255;
    auto expectedInt32 = -254;
    auto expectedInt64 = -253;

    // when
    auto actualInt16 = reader.getInt16();
    auto actualInt32 = reader.getInt32();
    auto actualInt64 = reader.getInt64();

    // then
    BOOST_CHECK_EQUAL(expectedInt16, actualInt16);
    BOOST_CHECK_EQUAL(expectedInt32, actualInt32);
    BOOST_CHECK_EQUAL(expectedInt64, actualInt64);
}

BOOST_AUTO_TEST_CASE(should_get_float_and_double_from_big_endian_stream) {
    // given
    auto input = make_shared<stringbuf>(string("\x3f\x80\x00\x00\x3f\xf0\x00\x00\x00\x00\x00\x00", 12));
    auto stream = make_shared<istream>(input.get());
    auto reader = StreamReader(stream, boost::endian::order::big);
    auto expectedFloat = 1.0f;
    auto expectedDouble = 1.0;

    // when
    auto actualFloat = reader.getFloat();
    auto actualDouble = reader.getDouble();

    // then
    BOOST_CHECK_EQUAL(expectedFloat, actualFloat);
    BOOST_CHECK_EQUAL(expectedDouble, actualDouble);
}

BOOST_AUTO_TEST_SUITE_END()
