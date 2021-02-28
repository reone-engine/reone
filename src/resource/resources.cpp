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

#include "resources.h"

#include <boost/algorithm/string.hpp>

#include "../common/log.h"
#include "../common/pathutil.h"
#include "../common/streamutil.h"

#include "format/2dafile.h"
#include "format/biffile.h"
#include "format/erffile.h"
#include "format/gfffile.h"
#include "format/rimfile.h"
#include "folder.h"
#include "typeutil.h"

using namespace std;

namespace fs = boost::filesystem;

namespace reone {

namespace resource {

static const char kPatchFileName[] = "patch.erf";
static const char kTalkTableFileName[] = "dialog.tlk";
static const char kExeFileNameKotor[] = "swkotor.exe";
static const char kExeFileNameTsl[] = "swkotor2.exe";

static const char kModulesDirectoryName[] = "modules";
static const char kOverrideDirectoryName[] = "override";
static const char kMusicDirectoryName[] = "streammusic";
static const char kSoundsDirectoryName[] = "streamsounds";
static const char kVoiceDirectoryName[] = "streamvoice";
static const char kWavesDirectoryName[] = "streamwaves";
static const char kTexturePackDirectoryName[] = "texturepacks";
static const char kLipsDirectoryName[] = "lips";

static const char kGUITexturePackFilename[] = "swpc_tex_gui.erf";
static const char kTexturePackFilename[] = "swpc_tex_tpa.erf";

Resources &Resources::instance() {
    static Resources instance;
    return instance;
}

void Resources::init(GameID gameId, const fs::path &gamePath) {
    _gameId = gameId;
    _gamePath = gamePath;

    indexKeyBifFiles();
    indexTexturePacks();
    indexTalkTable();
    indexAudioFiles();
    indexLipModFiles();
    indexExeFile();
    indexOverrideDirectory();

    loadModuleNames();
}

void Resources::indexKeyBifFiles() {
    auto keyBif = make_unique<KeyBifResourceProvider>();
    keyBif->init(_gamePath);

    _providers.push_back(move(keyBif));
}

void Resources::indexTexturePacks() {
    if (_gameId == GameID::KotOR) {
        fs::path patchPath(getPathIgnoreCase(_gamePath, kPatchFileName));
        indexErfFile(patchPath);
    }
    fs::path texPacksPath(getPathIgnoreCase(_gamePath, kTexturePackDirectoryName));
    fs::path guiTexPackPath(getPathIgnoreCase(texPacksPath, kGUITexturePackFilename));
    fs::path texPackPath(getPathIgnoreCase(texPacksPath, kTexturePackFilename));

    indexErfFile(guiTexPackPath);
    indexErfFile(texPackPath);
}

void Resources::indexErfFile(const fs::path &path) {
    auto erf = make_unique<ErfFile>();
    erf->load(path);

    _providers.push_back(move(erf));

    debug(boost::format("Resources: indexed: %s") % path);
}

void Resources::indexAudioFiles() {
    fs::path musicPath(getPathIgnoreCase(_gamePath, kMusicDirectoryName));
    fs::path soundsPath(getPathIgnoreCase(_gamePath, kSoundsDirectoryName));

    indexDirectory(musicPath);
    indexDirectory(soundsPath);

    switch (_gameId) {
        case GameID::TSL: {
            fs::path voicePath(getPathIgnoreCase(_gamePath, kVoiceDirectoryName));
            indexDirectory(voicePath);
            break;
        }
        default: {
            fs::path wavesPath(getPathIgnoreCase(_gamePath, kWavesDirectoryName));
            indexDirectory(wavesPath);
            break;
        }
    }
}

void Resources::indexLipModFiles() {
    static vector<string> kotorMods { "global", "localization" };
    static vector<string> tslMods { "localization" };

    const vector<string> &mods = _gameId == GameID::KotOR ? kotorMods : tslMods;
    fs::path lipsPath(getPathIgnoreCase(_gamePath, "lips"));

    for (auto &mod : mods) {
        fs::path modPath(getPathIgnoreCase(lipsPath, mod + ".mod"));
        indexErfFile(modPath);
    }
}

void Resources::indexRimFile(const fs::path &path) {
    auto rim = make_unique<RimFile>();
    rim->load(path);

    _providers.push_back(move(rim));

    debug(boost::format("Resources: indexed: %s") % path);
}

void Resources::indexDirectory(const fs::path &path) {
    auto folder = make_unique<Folder>();
    folder->load(path);

    _providers.push_back(move(folder));

    debug(boost::format("Resources: indexed: %s") % path);
}

void Resources::indexOverrideDirectory() {
    fs::path path(getPathIgnoreCase(_gamePath, kOverrideDirectoryName));
    indexDirectory(path);
}

void Resources::indexTalkTable() {
    fs::path path(getPathIgnoreCase(_gamePath, kTalkTableFileName));
    _tlkFile.load(path);

    debug(boost::format("Resources: indexed: %s") % path);
}

void Resources::indexExeFile() {
    string filename(_gameId == GameID::TSL ? kExeFileNameTsl : kExeFileNameKotor);
    fs::path path(getPathIgnoreCase(_gamePath, filename));

    _exeFile.load(path);

    debug(boost::format("Resources: indexed: %s") % path);
}

void Resources::loadModuleNames() {
    fs::path modules(getPathIgnoreCase(_gamePath, kModulesDirectoryName));

    for (auto &entry : fs::directory_iterator(modules)) {
        string filename(entry.path().filename().string());
        boost::to_lower(filename);

        if (!boost::ends_with(filename, ".rim") || boost::ends_with(filename, "_s.rim")) continue;

        string moduleName(filename.substr(0, filename.size() - 4));
        boost::to_lower(moduleName);

        _moduleNames.push_back(moduleName);
    }

    sort(_moduleNames.begin(), _moduleNames.end());
}

Resources::~Resources() {
    deinit();
}

void Resources::deinit() {
    invalidateCache();

    _transientProviders.clear();
    _providers.clear();
}

void Resources::invalidateCache() {
    _2daCache.clear();
    _gffCache.clear();
    _resCache.clear();
    _talkTableCache.clear();
}

void Resources::loadModule(const string &name) {
    invalidateCache();
    _transientProviders.clear();

    fs::path modulesPath(getPathIgnoreCase(_gamePath, kModulesDirectoryName));
    fs::path moduleRimPath(getPathIgnoreCase(modulesPath, name + ".rim"));
    fs::path moduleRimSPath(getPathIgnoreCase(modulesPath, name + "_s.rim"));

    fs::path lipsPath(getPathIgnoreCase(_gamePath, kLipsDirectoryName));
    fs::path lipModPath(getPathIgnoreCase(lipsPath, name + "_loc.mod"));

    indexTransientRimFile(moduleRimPath);
    indexTransientRimFile(moduleRimSPath);
    indexTransientErfFile(lipModPath);

    if (_gameId == GameID::TSL) {
        fs::path dlgPath(getPathIgnoreCase(modulesPath, name + "_dlg.erf"));
        indexTransientErfFile(dlgPath);
    }
}

void Resources::indexTransientRimFile(const fs::path &path) {
    auto rim = make_unique<RimFile>();
    rim->load(path);

    _transientProviders.push_back(move(rim));

    debug(boost::format("Resources: indexed: %s") % path);
}

void Resources::indexTransientErfFile(const fs::path &path) {
    auto erf = make_unique<ErfFile>();
    erf->load(path);

    _transientProviders.push_back(move(erf));

    debug(boost::format("Resources: indexed: %s") % path);
}

template <class T>
static shared_ptr<T> findResource(const string &key, unordered_map<string, shared_ptr<T>> &cache, const function<shared_ptr<T>()> &getter) {
    auto maybeResource = cache.find(key);
    if (maybeResource != cache.end()) {
        return maybeResource->second;
    };
    auto inserted = cache.insert(make_pair(key, getter()));

    return inserted.first->second;
}

shared_ptr<TwoDA> Resources::get2DA(const string &resRef) {
    return findResource<TwoDA>(resRef, _2daCache, [this, &resRef]() {
        shared_ptr<ByteArray> data(get(resRef, ResourceType::TwoDa));
        shared_ptr<TwoDA> twoDa;

        if (data) {
            TwoDaFile file;
            file.load(wrap(data));
            twoDa = file.twoDa();
        }

        return move(twoDa);
    });
}

shared_ptr<ByteArray> Resources::get(const string &resRef, ResourceType type, bool logNotFound) {
    string cacheKey(getCacheKey(resRef, type));
    auto res = _resCache.find(cacheKey);
    if (res != _resCache.end()) {
        return res->second;
    }
    debug("Resources: load " + cacheKey, 2);

    shared_ptr<ByteArray> data = get(_transientProviders, resRef, type);
    if (!data) {
        data = get(_providers, resRef, type);
    }
    if (!data && logNotFound) {
        warn("Resources: not found: " + cacheKey);
    }
    auto pair = _resCache.insert(make_pair(cacheKey, move(data)));

    return pair.first->second;
}

string Resources::getCacheKey(const string &resRef, resource::ResourceType type) const {
    return str(boost::format("%s.%s") % resRef % getExtByResType(type));
}

shared_ptr<ByteArray> Resources::get(const vector<unique_ptr<IResourceProvider>> &providers, const string &resRef, ResourceType type) {
    for (auto provider = providers.rbegin(); provider != providers.rend(); ++provider) {
        if (!(*provider)->supports(type)) continue;

        shared_ptr<ByteArray> data((*provider)->find(resRef, type));
        if (data) {
            return data;
        }
    }

    return nullptr;
}

shared_ptr<GffStruct> Resources::getGFF(const string &resRef, ResourceType type) {
    string cacheKey(getCacheKey(resRef, type));

    return findResource<GffStruct>(cacheKey, _gffCache, [this, &resRef, &type]() {
        shared_ptr<ByteArray> data(get(resRef, type));
        shared_ptr<GffStruct> gffs;

        if (data) {
            GffFile gff;
            gff.load(wrap(data));
            gffs = gff.root();
        }

        return move(gffs);
    });
}

shared_ptr<TalkTable> Resources::getTalkTable(const string &resRef) {
    return findResource<TalkTable>(resRef, _talkTableCache, [this, &resRef]() {
        shared_ptr<ByteArray> data(get(resRef, ResourceType::Dlg));
        shared_ptr<TalkTable> table;

        if (data) {
            TlkFile tlk;
            tlk.load(wrap(data));
            table = tlk.table();
        }

        return move(table);
    });
}

shared_ptr<ByteArray> Resources::getFromExe(uint32_t name, PEResourceType type) {
    return _exeFile.find(name, type);
}

string Resources::getString(int strRef) const {
    static string empty;

    shared_ptr<TalkTable> table(_tlkFile.table());
    if (strRef == -1 || strRef >= table->getStringCount()) {
        return empty;
    }

    string text(table->getString(strRef).text);
    _stringProcessor.process(text, _gameId);

    return move(text);
}

string Resources::getSoundByStrRef(int strRef) const {
    shared_ptr<TalkTable> table(_tlkFile.table());
    if (strRef == -1 || strRef >= table->getStringCount()) return "";

    return table->getString(strRef).soundResRef;
}

} // namespace resource

} // namespace reone
