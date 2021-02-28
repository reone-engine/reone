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

#include <memory>

#include <boost/filesystem/path.hpp>

#include  "../../common/streamwriter.h"

#include "../gffstruct.h"
#include "../types.h"

namespace reone {

namespace resource {

class GffWriter {
public:
    GffWriter(ResourceType resType, const std::shared_ptr<GffStruct> &root);

    void save(const boost::filesystem::path &path);
    void save(const std::shared_ptr<std::ostream> &out);

private:
    struct WriteStruct {
        uint32_t type { 0 };
        uint32_t dataOrDataOffset { 0 };
        uint32_t fieldCount { 0 };
    };

    struct WriteField {
        uint32_t type { 0 };
        uint32_t labelIndex { 0 };
        uint32_t dataOrDataOffset { 0 };
    };

    struct WriteContext {
        std::vector<WriteStruct> structs;
        std::vector<WriteField> fields;
        std::vector<std::string> labels;
        ByteArray fieldData;
        std::vector<uint32_t> fieldIndices;
        std::vector<uint32_t> listIndices;
    };

    ResourceType _resType;
    std::shared_ptr<GffStruct> _root;
    WriteContext _context;
    std::unique_ptr<StreamWriter> _writer;

    void processTree();

    void writeHeader();
    void writeStructArray();
    void writeFieldArray();
    void writeLabelArray();
    void writeFieldData();
    void writeFieldIndices();
    void writeListIndices();
};

} // namespace resource

} // namespace reone
