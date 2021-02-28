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

#include <boost/filesystem/path.hpp>

#include "../src/resource/format/2dafile.h"
#include "../src/resource/format/biffile.h"
#include "../src/resource/format/erffile.h"
#include "../src/resource/format/keyfile.h"
#include "../src/resource/format/rimfile.h"

#include "types.h"

namespace reone {

namespace tools {

class ITool {
public:
    virtual void invoke(
        Operation operation,
        const boost::filesystem::path &target,
        const boost::filesystem::path &gamePath,
        const boost::filesystem::path &destPath) = 0;

    virtual bool supports(Operation operation, const boost::filesystem::path &target) const = 0;
};

class KeyBifTool : public ITool {
public:
    void invoke(
        Operation operation,
        const boost::filesystem::path &target,
        const boost::filesystem::path &gamePath,
        const boost::filesystem::path &destPath) override;

    bool supports(Operation operation, const boost::filesystem::path &target) const override;

private:
    void listKEY(const resource::KeyFile &key);
    void listBIF(const resource::KeyFile &key, const resource::BifFile &bif, int bifIdx);
    void extractBIF(const resource::KeyFile &key, resource::BifFile &bif, int bifIdx, const boost::filesystem::path &destPath);
};

class ErfTool : public ITool {
public:
    void invoke(
        Operation operation,
        const boost::filesystem::path &target,
        const boost::filesystem::path &gamePath,
        const boost::filesystem::path &destPath) override;

    bool supports(Operation operation, const boost::filesystem::path &target) const override;

private:
    void list(const resource::ErfFile &erf);
    void extract(resource::ErfFile &erf, const boost::filesystem::path &destPath);
};

class RimTool : public ITool {
public:
    void invoke(
        Operation operation,
        const boost::filesystem::path &target,
        const boost::filesystem::path &gamePath,
        const boost::filesystem::path &destPath) override;

    bool supports(Operation operation, const boost::filesystem::path &target) const override;

private:
    void list(const resource::RimFile &rim);
    void extract(resource::RimFile &rim, const boost::filesystem::path &destPath);
};

class TwoDaTool : public ITool {
public:
    void invoke(
        Operation operation,
        const boost::filesystem::path &target,
        const boost::filesystem::path &gamePath,
        const boost::filesystem::path &destPath) override;

    bool supports(Operation operation, const boost::filesystem::path &target) const override;

private:
    void toJSON(const boost::filesystem::path &path, const boost::filesystem::path &destPath);
    void to2DA(const boost::filesystem::path &path, const boost::filesystem::path &destPath);
};

class GffTool : public ITool {
public:
    void invoke(
        Operation operation,
        const boost::filesystem::path &target,
        const boost::filesystem::path &gamePath,
        const boost::filesystem::path &destPath) override;

    bool supports(Operation operation, const boost::filesystem::path &target) const override;

private:
    void toJSON(const boost::filesystem::path &path, const boost::filesystem::path &destPath);
    void toGFF(const boost::filesystem::path &path, const boost::filesystem::path &destPath);
};

class TlkTool : public ITool {
public:
    void invoke(
        Operation operation,
        const boost::filesystem::path &target,
        const boost::filesystem::path &gamePath,
        const boost::filesystem::path &destPath) override;

    bool supports(Operation operation, const boost::filesystem::path &target) const override;

private:
    void toJSON(const boost::filesystem::path &path, const boost::filesystem::path &destPath);
};

class TpcTool : public ITool {
public:
    void invoke(
        Operation operation,
        const boost::filesystem::path &target,
        const boost::filesystem::path &gamePath,
        const boost::filesystem::path &destPath) override;

    bool supports(Operation operation, const boost::filesystem::path &target) const override;

private:
    void toTGA(const boost::filesystem::path &path, const boost::filesystem::path &destPath);
};

} // namespace tools

} // namespace reone
