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
#include "../../src/common/stringbuilder.h"
#include "../../src/resource/format/gffwriter.h"
#include "../../src/resource/gffstruct.h"

#include "../checkutil.h"

using namespace std;

using namespace reone;
using namespace reone::resource;

BOOST_AUTO_TEST_SUITE(gff_writer)

BOOST_AUTO_TEST_CASE(should_write_gff) {
    // given

    auto expectedOutput = StringBuilder()
                              // header
                              .append("RES V3.2")
                              .append("\x38\x00\x00\x00", 4) // offset to structs
                              .append("\x04\x00\x00\x00", 4) // number of structs
                              .append("\x68\x00\x00\x00", 4) // offset to fields
                              .append("\x13\x00\x00\x00", 4) // number of fields
                              .append("\x4c\x01\x00\x00", 4) // offset to labels
                              .append("\x13\x00\x00\x00", 4) // number of labels
                              .append("\x7c\x02\x00\x00", 4) // offset to field data
                              .append("\x67\x00\x00\x00", 4) // size of field data
                              .append("\xe3\x02\x00\x00", 4) // offset to field indices
                              .append("\x40\x00\x00\x00", 4) // size of field indices
                              .append("\x23\x03\x00\x00", 4) // offset to list indices
                              .append("\x0c\x00\x00\x00", 4) // size of list indices
                              // structs
                              .append("\xff\xff\xff\xff", 4) // 0: type
                              .append("\x00\x00\x00\x00", 4) // 0: data offset
                              .append("\x10\x00\x00\x00", 4) // 0: field count
                              .append("\x01\x00\x00\x00", 4) // 1: type
                              .append("\x10\x00\x00\x00", 4) // 1: data offset
                              .append("\x01\x00\x00\x00", 4) // 1: field count
                              .append("\x02\x00\x00\x00", 4) // 2: type
                              .append("\x11\x00\x00\x00", 4) // 2: data offset
                              .append("\x01\x00\x00\x00", 4) // 2: field count
                              .append("\x03\x00\x00\x00", 4) // 3: type
                              .append("\x12\x00\x00\x00", 4) // 3: data offset
                              .append("\x01\x00\x00\x00", 4) // 3: field count
                              // fields
                              .append("\x00\x00\x00\x00", 4) // 0: type
                              .append("\x00\x00\x00\x00", 4) // 0: label index
                              .append("\x00\x00\x00\x00", 4) // 0: data
                              .append("\x05\x00\x00\x00", 4) // 1: type
                              .append("\x01\x00\x00\x00", 4) // 1: label index
                              .append("\x01\x00\x00\x00", 4) // 1: data
                              .append("\x04\x00\x00\x00", 4) // 2: type
                              .append("\x02\x00\x00\x00", 4) // 2: label index
                              .append("\x02\x00\x00\x00", 4) // 2: data
                              .append("\x07\x00\x00\x00", 4) // 3: type
                              .append("\x03\x00\x00\x00", 4) // 3: label index
                              .append("\x00\x00\x00\x00", 4) // 3: data
                              .append("\x06\x00\x00\x00", 4) // 4: type
                              .append("\x04\x00\x00\x00", 4) // 4: label index
                              .append("\x08\x00\x00\x00", 4) // 4: data
                              .append("\x08\x00\x00\x00", 4) // 5: type
                              .append("\x05\x00\x00\x00", 4) // 5: label index
                              .append("\x00\x00\x80\x3f", 4) // 5: data
                              .append("\x09\x00\x00\x00", 4) // 6: type
                              .append("\x06\x00\x00\x00", 4) // 6: label index
                              .append("\x10\x00\x00\x00", 4) // 6: data
                              .append("\x0a\x00\x00\x00", 4) // 7: type
                              .append("\x07\x00\x00\x00", 4) // 7: label index
                              .append("\x18\x00\x00\x00", 4) // 7: data
                              .append("\x0b\x00\x00\x00", 4) // 8: type
                              .append("\x08\x00\x00\x00", 4) // 8: label index
                              .append("\x20\x00\x00\x00", 4) // 8: data
                              .append("\x0c\x00\x00\x00", 4) // 9: type
                              .append("\x09\x00\x00\x00", 4) // 9: label index
                              .append("\x25\x00\x00\x00", 4) // 9: data
                              .append("\x0d\x00\x00\x00", 4) // 10: type
                              .append("\x0a\x00\x00\x00", 4) // 10: label index
                              .append("\x3d\x00\x00\x00", 4) // 10: data
                              .append("\x10\x00\x00\x00", 4) // 11: type
                              .append("\x0b\x00\x00\x00", 4) // 11: label index
                              .append("\x43\x00\x00\x00", 4) // 11: data
                              .append("\x11\x00\x00\x00", 4) // 12: type
                              .append("\x0c\x00\x00\x00", 4) // 12: label index
                              .append("\x53\x00\x00\x00", 4) // 12: data
                              .append("\x12\x00\x00\x00", 4) // 13: type
                              .append("\x0d\x00\x00\x00", 4) // 13: label index
                              .append("\x5f\x00\x00\x00", 4) // 13: data
                              .append("\x0e\x00\x00\x00", 4) // 14: type
                              .append("\x0e\x00\x00\x00", 4) // 14: label index
                              .append("\x01\x00\x00\x00", 4) // 14: data
                              .append("\x0f\x00\x00\x00", 4) // 15: type
                              .append("\x0f\x00\x00\x00", 4) // 15: label index
                              .append("\x00\x00\x00\x00", 4) // 15: data
                              .append("\x01\x00\x00\x00", 4) // 16: type
                              .append("\x10\x00\x00\x00", 4) // 16: label index
                              .append("\x01\x00\x00\x00", 4) // 16: data
                              .append("\x02\x00\x00\x00", 4) // 17: type
                              .append("\x11\x00\x00\x00", 4) // 17: label index
                              .append("\x02\x00\x00\x00", 4) // 17: data
                              .append("\x03\x00\x00\x00", 4) // 18: type
                              .append("\x12\x00\x00\x00", 4) // 18: label index
                              .append("\x03\x00\x00\x00", 4) // 18: data
                              // labels
                              .append("Byte\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 16)
                              .append("Int\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 16)
                              .append("Uint\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 16)
                              .append("Int64\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 16)
                              .append("Uint64\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 16)
                              .append("Float\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 16)
                              .append("Double\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 16)
                              .append("CExoString\x00\x00\x00\x00\x00\x00", 16)
                              .append("ResRef\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 16)
                              .append("CExoLocString\x00\x00\x00", 16)
                              .append("Void\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 16)
                              .append("Orientation\x00\x00\x00\x00\x00", 16)
                              .append("Vector\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 16)
                              .append("StrRef\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 16)
                              .append("Struct\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 16)
                              .append("List\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 16)
                              .append("Struct1Char\x00\x00\x00\x00\x00", 16)
                              .append("Struct2Word\x00\x00\x00\x00\x00", 16)
                              .append("Struct3Short\x00\x00\x00\x00", 16)
                              // field data
                              .append("\x03\x00\x00\x00\x00\x00\x00\x00", 8)
                              .append("\x04\x00\x00\x00\x00\x00\x00\x00", 8)
                              .append("\x00\x00\x00\x00\x00\x00\xf0\x3f", 8)
                              .append("\x04\x00\x00\x00John", 8)
                              .append("\x04Jane", 5)
                              .append("\x14\x00\x00\x00\xff\xff\xff\xff\x01\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00Jill", 24)
                              .append("\x02\x00\x00\x00\xff\xff", 6)
                              .append("\x00\x00\x80\x3f\x00\x00\x80\x3f\x00\x00\x80\x3f\x00\x00\x80\x3f", 16)
                              .append("\x00\x00\x80\x3f\x00\x00\x80\x3f\x00\x00\x80\x3f", 12)
                              .append("\x04\x00\x00\x00\x01\x00\x00\x00", 8)
                              // field indices
                              .append("\x00\x00\x00\x00", 4)
                              .append("\x01\x00\x00\x00", 4)
                              .append("\x02\x00\x00\x00", 4)
                              .append("\x03\x00\x00\x00", 4)
                              .append("\x04\x00\x00\x00", 4)
                              .append("\x05\x00\x00\x00", 4)
                              .append("\x06\x00\x00\x00", 4)
                              .append("\x07\x00\x00\x00", 4)
                              .append("\x08\x00\x00\x00", 4)
                              .append("\x09\x00\x00\x00", 4)
                              .append("\x0a\x00\x00\x00", 4)
                              .append("\x0b\x00\x00\x00", 4)
                              .append("\x0c\x00\x00\x00", 4)
                              .append("\x0d\x00\x00\x00", 4)
                              .append("\x0e\x00\x00\x00", 4)
                              .append("\x0f\x00\x00\x00", 4)
                              // list indices
                              .append("\x02\x00\x00\x00", 4)
                              .append("\x02\x00\x00\x00", 4)
                              .append("\x03\x00\x00\x00", 4)
                              .build();

    auto root = make_shared<GffStruct>(
        0xffffffff,
        vector<GffField> {
            GffField::newByte("Byte", 0),
            GffField::newInt("Int", 1),
            GffField::newDword("Uint", 2),
            GffField::newInt64("Int64", 3),
            GffField::newDword64("Uint64", 4),
            GffField::newFloat("Float", 1.0f),
            GffField::newDouble("Double", 1.0),
            GffField::newCExoString("CExoString", "John"),
            GffField::newResRef("ResRef", "Jane"),
            GffField::newCExoLocString("CExoLocString", -1, "Jill"),
            GffField::newVoid("Void", ByteArray {static_cast<char>(0xff), static_cast<char>(0xff)}),
            GffField::newOrientation("Orientation", glm::quat(1.0f, 1.0f, 1.0f, 1.0f)),
            GffField::newVector("Vector", glm::vec3(1.0f, 1.0f, 1.0f)),
            GffField::newStrRef("StrRef", 1),
            GffField::newStruct(
                "Struct",
                make_shared<GffStruct>(1, vector<GffField> {GffField::newChar("Struct1Char", 1)})),
            GffField::newList(
                "List",
                vector<shared_ptr<GffStruct>> {
                    make_shared<GffStruct>(2, vector<GffField> {GffField::newWord("Struct2Word", 2)}),
                    make_shared<GffStruct>(3, vector<GffField> {GffField::newShort("Struct3Short", 3)})})});

    auto writer = GffWriter(ResourceType::Res, root);
    auto stream = make_shared<ostringstream>();

    // when

    writer.save(stream);

    // then

    auto actualOutput = stream->str();
    BOOST_TEST((expectedOutput == actualOutput), notEqualMessage(expectedOutput, actualOutput));
}

BOOST_AUTO_TEST_SUITE_END()
