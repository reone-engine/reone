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

/** @file
 *  Tests for GffStruct class.
 */

#include <boost/test/unit_test.hpp>

#include "../../src/engine/common/streamwriter.h"
#include "../../src/engine/resource/format/gffreader.h"
#include "../../src/engine/resource/format/gffwriter.h"

using namespace std;

using namespace reone::resource;

namespace endian = boost::endian;
namespace fs = boost::filesystem;

BOOST_AUTO_TEST_CASE(WhenSaveAndLoad) {
    auto struct1 = make_shared<GffStruct>(0);
    GffField struct1Field(GffFieldType::Byte, "MyByte"); struct1Field.uintValue = 1;
    struct1->add(move(struct1Field));

    auto struct2 = make_shared<GffStruct>(0);
    GffField struct2Field(GffFieldType::Byte, "MyByte"); struct2Field.uintValue = 2;
    struct2->add(move(struct2Field));

    auto struct3 = make_shared<GffStruct>(0);
    GffField struct3Field(GffFieldType::Byte, "MyByte"); struct3Field.uintValue = 3;
    struct3->add(move(struct3Field));

    vector<GffField> rootFields {
        GffField(GffFieldType::Struct, "MyStruct"),
        GffField(GffFieldType::List, "MyList")
    };
    rootFields[0].children.push_back(move(struct1));
    rootFields[1].children.push_back(move(struct2));
    rootFields[1].children.push_back(move(struct3));
    auto root = make_shared<GffStruct>(0xffffffff);
    for (auto &field : rootFields) {
        root->add(move(field));
    }

    auto out = make_shared<ostringstream>();
    GffWriter writer(ResourceType::Utp, root);
    writer.save(out);

    auto in = make_shared<istringstream>(out->str());
    GffReader gff;
    gff.load(in);
    auto readRoot = gff.root();

    BOOST_TEST((readRoot->fields().size() == 2ll));
    BOOST_TEST((readRoot->fields()[0].children.size() == 1ll));
    BOOST_TEST((readRoot->fields()[1].children.size() == 2ll));
    BOOST_TEST((readRoot->fields()[0].children[0]->fields()[0].uintValue == 1));
    BOOST_TEST((readRoot->fields()[1].children[0]->fields()[0].uintValue == 2));
    BOOST_TEST((readRoot->fields()[1].children[1]->fields()[0].uintValue == 3));
}
