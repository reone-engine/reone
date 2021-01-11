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

#include "binfile.h"

namespace reone {

namespace resource {

/**
 * Encapsulates the LTR file format, used to generate random names.
 */
class LtrFile : public BinaryFile {
public:
    LtrFile();

    std::string getRandomName(int maxLength) const;

private:
    struct LetterSet {
        std::vector<float> start;
        std::vector<float> mid;
        std::vector<float> end;
    };

    int _letterCount { 0 };
    LetterSet _singleLetters;
    std::vector<LetterSet> _doubleLetters;
    std::vector<std::vector<LetterSet>> _trippleLetters;

    void doLoad() override;

    void readLetterSet(LetterSet &set);
};

} // namespace resource

} // namespace reone
