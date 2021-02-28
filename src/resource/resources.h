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
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <boost/filesystem/path.hpp>
#include <boost/noncopyable.hpp>

#include "2da.h"
#include "format/keyfile.h"
#include "format/pefile.h"
#include "format/tlkfile.h"
#include "gffstruct.h"
#include "keybifprovider.h"
#include "resourceprovider.h"
#include "stringprocessor.h"
#include "types.h"

namespace reone {

namespace resource {

/**
 * Encapsulates game resource management. Contains a prioritized list of
 * resource providers, that it queries for resources by ResRef and ResType.
 * Caches found resources.
 */
class Resources : boost::noncopyable {
public:
    static Resources &instance();

    void init(GameID gameId, const boost::filesystem::path &gamePath);
    void deinit();

    void loadModule(const std::string &name);

    std::shared_ptr<TwoDA> get2DA(const std::string &resRef);
    std::shared_ptr<GffStruct> getGFF(const std::string &resRef, ResourceType type);
    std::shared_ptr<ByteArray> getFromExe(uint32_t name, PEResourceType type);
    std::shared_ptr<TalkTable> getTalkTable(const std::string &resRef);

    /**
     * Searches for the raw resource data by ResRef and ResType.
     */
    std::shared_ptr<ByteArray> get(const std::string &resRef, ResourceType type, bool logNotFound = true);

    /**
     * Searches for a string in the global talktable by StrRef.
     *
     * @return string from the global talktable if found, empty string otherwise
     */
    std::string getString(int strRef) const;

    /**
     * Searches for a sound in the global talktable by StrRef.
     *
     * @return ResRef of a sound from the global talktable if found, empty string otherwise
     */
    std::string getSoundByStrRef(int strRef) const;

    /**
     * @return list of available module names
     */
    const std::vector<std::string> &moduleNames() const { return _moduleNames; }

private:
    GameID _gameId { GameID::KotOR };
    boost::filesystem::path _gamePath;
    std::vector<std::string> _moduleNames;
    StringProcessor _stringProcessor;

    // Resource providers

    TlkFile _tlkFile;
    PEFile _exeFile;
    std::vector<std::unique_ptr<IResourceProvider>> _providers;
    std::vector<std::unique_ptr<IResourceProvider>> _transientProviders; /**< transient providers are replaced when switching between modules */

    // END Resource providers

    // Resource caches

    std::unordered_map<std::string, std::shared_ptr<TwoDA>> _2daCache;
    std::unordered_map<std::string, std::shared_ptr<GffStruct>> _gffCache;
    std::unordered_map<std::string, std::shared_ptr<ByteArray>> _resCache;
    std::unordered_map<std::string, std::shared_ptr<TalkTable>> _talkTableCache;

    // END Resource caches

    ~Resources();

    void indexKeyBifFiles();
    void indexTexturePacks();
    void indexTalkTable();
    void indexAudioFiles();
    void indexLipModFiles();
    void indexExeFile();
    void indexOverrideDirectory();

    void indexErfFile(const boost::filesystem::path &path);
    void indexTransientErfFile(const boost::filesystem::path &path);
    void indexRimFile(const boost::filesystem::path &path);
    void indexTransientRimFile(const boost::filesystem::path &path);
    void indexDirectory(const boost::filesystem::path &path);

    void invalidateCache();
    void loadModuleNames();

    std::shared_ptr<ByteArray> get(const std::vector<std::unique_ptr<IResourceProvider>> &providers, const std::string &resRef, ResourceType type);
    std::string getCacheKey(const std::string &resRef, ResourceType type) const;
};

} // namespace resource

} // namespace reone
