/*
 * Copyright (c) 2020-2023 The reone project contributors
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

#include "explorer.h"

#include <wx/stopwatch.h>

#include "reone/audio/format/mp3reader.h"
#include "reone/audio/format/wavreader.h"
#include "reone/graphics/animation.h"
#include "reone/graphics/format/lipreader.h"
#include "reone/graphics/format/mdlmdxreader.h"
#include "reone/graphics/lipanimation.h"
#include "reone/resource/exception/notfound.h"
#include "reone/resource/format/2dareader.h"
#include "reone/resource/format/gffreader.h"
#include "reone/resource/format/keyreader.h"
#include "reone/resource/format/ssfreader.h"
#include "reone/resource/format/tlkreader.h"
#include "reone/resource/provider/textures.h"
#include "reone/resource/talktable.h"
#include "reone/system/fileutil.h"
#include "reone/system/stream/fileinput.h"
#include "reone/system/stream/fileoutput.h"
#include "reone/system/stream/memoryinput.h"
#include "reone/system/stream/memoryoutput.h"
#include "reone/tools/legacy/2da.h"
#include "reone/tools/legacy/audio.h"
#include "reone/tools/legacy/erf.h"
#include "reone/tools/legacy/gff.h"
#include "reone/tools/legacy/keybif.h"
#include "reone/tools/legacy/lip.h"
#include "reone/tools/legacy/ncs.h"
#include "reone/tools/legacy/rim.h"
#include "reone/tools/legacy/ssf.h"
#include "reone/tools/legacy/tlk.h"
#include "reone/tools/legacy/tpc.h"
#include "reone/tools/lip/shapeutil.h"

#include "gff.h"
#include "ncs.h"
#include "nss.h"
#include "table.h"
#include "text.h"

using namespace reone::audio;
using namespace reone::game;
using namespace reone::graphics;
using namespace reone::resource;
using namespace reone::scene;
using namespace reone::script;

namespace reone {

static const std::set<std::string> kFilesSubdirectoryWhitelist {
    "data", "lips", "localvault", "modules", "movies", "override", "rims", "saves", "texturepacks", //
    "streammusic", "streamsounds", "streamwaves", "streamvoice"};

static const std::set<std::string> kFilesArchiveExtensions {".bif", ".erf", ".sav", ".rim", ".mod"};

static const std::set<std::string> kFilesExtensionBlacklist {
    ".key",                                         //
    ".lnk", ".bat", ".exe", ".dll", ".ini", ".ico", //
    ".zip", ".pdf",                                 //
    ".hashdb", ".info", ".script", ".dat", ".msg", ".sdb", ".ds_store"};

static const std::set<ResType> kFilesPlaintextResTypes {
    ResType::Txt,
    ResType::Txi,
    ResType::Lyt,
    ResType::Vis};

class wxClock : public IClock, boost::noncopyable {
public:
    wxClock() {
        _stopWatch.Start();
    }

    uint64_t ticks() const override {
        return _stopWatch.Time();
    }

private:
    wxStopWatch _stopWatch;
};

void ResourceExplorerViewModel::openFile(const ResourcesItem &item) {
    withResourceStream(item, [this, &item](auto &res) {
        try {
            openResource(*item.resId, res);
        } catch (const std::exception &e) {
            error(str(boost::format("Error opening resource '%s': %s") % item.resId->string() % std::string(e.what())));
        }
    });
}

void ResourceExplorerViewModel::openResource(const ResourceId &id, IInputStream &data) {
    info(str(boost::format("Opening resource '%s'") % id.string()));

    PageType pageType;
    try {
        pageType = getPageType(id.type);
    } catch (const std::invalid_argument &e) {
        return;
    }
    for (size_t i = 0; i < _pages->size(); ++i) {
        const auto &page = _pages.at(i);
        if (page->resourceId == id && page->type == pageType) {
            _selectedPage = i;
            return;
        }
    }

    if (kFilesPlaintextResTypes.count(id.type) > 0) {
        data.seek(0, SeekOrigin::End);
        auto length = data.position();
        data.seek(0, SeekOrigin::Begin);
        auto text = std::string(length, '\0');
        data.read(&text[0], length);

        auto page = std::make_shared<Page>(PageType::Text, id.string(), id);
        page->viewModel = std::make_shared<TextResourceViewModel>(std::move(text));
        _pages.add(std::move(page));

    } else if (id.type == ResType::TwoDa) {
        auto reader = TwoDaReader(data);
        reader.load();
        auto twoDa = reader.twoDa();

        auto columns = std::vector<std::string>();
        for (auto &column : twoDa->columns()) {
            columns.push_back(column);
        }
        auto rows = std::vector<std::vector<std::string>>();
        for (int i = 0; i < twoDa->getRowCount(); ++i) {
            auto &row = twoDa->rows()[i];
            auto values = std::vector<std::string>();
            for (auto &value : row.values) {
                values.push_back(value);
            }
            rows.push_back(std::move(values));
        }

        auto page = std::make_shared<Page>(PageType::Table, id.string(), id);
        page->viewModel = std::make_shared<TableResourceViewModel>(
            id.type,
            std::make_shared<TableContent>(std::move(columns), std::move(rows), true));
        _pages.add(std::move(page));

    } else if (isGFFCompatibleResType(id.type)) {
        auto reader = GffReader(data);
        reader.load();

        auto page = std::make_shared<Page>(PageType::GFF, id.string(), id);
        page->viewModel = std::make_shared<GFFResourceViewModel>(reader.root());
        _pages.add(std::move(page));

    } else if (id.type == ResType::Tlk) {
        auto reader = TlkReader(data);
        reader.load();
        auto tlk = reader.table();

        auto columns = std::vector<std::string>();
        columns.push_back("Text");
        columns.push_back("Sound");

        auto rows = std::vector<std::vector<std::string>>();
        for (int i = 0; i < tlk->getStringCount(); ++i) {
            auto &str = tlk->getString(i);
            auto cleanedText = boost::replace_all_copy(str.text, "\n", "\\n");
            auto values = std::vector<std::string>();
            values.push_back(cleanedText);
            values.push_back(str.soundResRef);
            rows.push_back(std::move(values));
        }

        auto page = std::make_shared<Page>(PageType::Table, id.string(), id);
        page->viewModel = std::make_shared<TableResourceViewModel>(
            id.type,
            std::make_shared<TableContent>(std::move(columns), std::move(rows), true));
        _pages.add(std::move(page));

    } else if (id.type == ResType::Ncs) {
        auto pcodeBytes = ByteBuffer();
        auto pcode = MemoryOutputStream(pcodeBytes);
        NcsTool(_gameId).toPCODE(data, pcode, *_routines);

        auto page = std::make_shared<Page>(PageType::NCS, id.string(), id);
        page->viewModel = std::make_shared<NCSResourceViewModel>(std::string(pcodeBytes.begin(), pcodeBytes.end()));
        _pages.add(std::move(page));

    } else if (id.type == ResType::Nss) {
        data.seek(0, SeekOrigin::End);
        auto length = data.position();
        data.seek(0, SeekOrigin::Begin);
        auto text = std::string(length, '\0');
        data.read(&text[0], length);

        auto page = std::make_shared<Page>(PageType::NSS, id.string(), id);
        page->viewModel = std::make_shared<NSSResourceViewModel>(std::move(text));
        _pages.add(std::move(page));

    } else if (id.type == ResType::Lip) {
        auto reader = LipReader(data, "");
        reader.load();
        auto animation = reader.animation();

        auto columns = std::vector<std::string>();
        columns.push_back("Time");
        columns.push_back("Shape");
        auto rows = std::vector<std::vector<std::string>>();
        for (auto &kf : animation->keyframes()) {
            auto values = std::vector<std::string>();
            values.push_back(std::to_string(kf.time));
            values.push_back(std::to_string(kf.shape));
            rows.push_back(std::move(values));
        }

        auto page = std::make_shared<Page>(PageType::Table, id.string(), id);
        page->viewModel = std::make_shared<TableResourceViewModel>(
            id.type,
            std::make_shared<TableContent>(std::move(columns), std::move(rows), false));
        _pages.add(std::move(page));

    } else if (id.type == ResType::Ssf) {
        auto reader = SsfReader(data);
        reader.load();
        auto &soundSet = reader.soundSet();

        auto columns = std::vector<std::string>();
        columns.push_back("StrRef");
        auto rows = std::vector<std::vector<std::string>>();
        for (size_t i = 0; i < soundSet.size(); ++i) {
            auto values = std::vector<std::string>();
            auto strRef = soundSet.at(i);
            values.push_back(std::to_string(strRef));
            rows.push_back(std::move(values));
        }

        auto page = std::make_shared<Page>(PageType::Table, id.string(), id);
        page->viewModel = std::make_shared<TableResourceViewModel>(
            id.type,
            std::make_shared<TableContent>(std::move(columns), std::move(rows), true));
        _pages.add(std::move(page));

    } else if (id.type == ResType::Tpc || id.type == ResType::Tga) {
        _imageResViewModel->openImage(id, data);

        _pages.removeIf([](auto &page) { return page->type == PageType::Image; });
        auto page = std::make_shared<Page>(PageType::Image, id.string(), id);
        _pages.add(std::move(page));

    } else if (id.type == ResType::Mdl) {
        loadEngine();

        _renderEnabled = false;
        _modelResViewModel->openModel(id, data);

        _pages.removeIf([](auto &page) { return page->type == PageType::Model; });
        auto page = std::make_shared<Page>(PageType::Model, id.string(), id);
        _pages.add(std::move(page));

        _renderEnabled = true;

    } else if (id.type == ResType::Wav) {
        loadEngine();
        _audioResViewModel->openAudio(id, data);

        _pages.removeIf([](auto &page) { return page->type == PageType::Audio; });
        auto page = std::make_shared<Page>(PageType::Audio, id.string(), id);
        _pages.add(std::move(page));

    } else {
        return;
    }
}

PageType ResourceExplorerViewModel::getPageType(ResType type) const {
    if (kFilesPlaintextResTypes.count(type) > 0) {
        return PageType::Text;
    }
    if (isGFFCompatibleResType(type)) {
        return PageType::GFF;
    }
    switch (type) {
    case ResType::TwoDa:
    case ResType::Tlk:
    case ResType::Lip:
    case ResType::Ssf:
        return PageType::Table;
    case ResType::Ncs:
        return PageType::NCS;
    case ResType::Nss:
        return PageType::NSS;
    case ResType::Tga:
    case ResType::Tpc:
        return PageType::Image;
    case ResType::Mdl:
        return PageType::Model;
    case ResType::Wav:
        return PageType::Audio;
    default:
        throw std::invalid_argument("Invalid resource type: " + std::to_string(static_cast<int>(type)));
    }
}

void ResourceExplorerViewModel::loadResources() {
    auto keyPath = findFileIgnoreCase(_resourcesPath, "chitin.key");
    if (keyPath) {
        auto key = FileInputStream(*keyPath);
        auto keyReader = KeyReader(key);
        keyReader.load();
        _keyKeys = keyReader.keys();
        _keyFiles = keyReader.files();
    }
    auto tlkPath = findFileIgnoreCase(_resourcesPath, "dialog.tlk");
    if (tlkPath) {
        auto tlk = FileInputStream(*tlkPath);
        auto tlkReader = TlkReader(tlk);
        tlkReader.load();
        _talkTable = tlkReader.table();
    }
    _routines = std::make_unique<Routines>(_gameId, nullptr, nullptr);
    _routines->init();

    for (auto &file : std::filesystem::directory_iterator(_resourcesPath)) {
        auto filename = boost::to_lower_copy(file.path().filename().string());
        auto extension = boost::to_lower_copy(file.path().extension().string());
        bool container;
        if ((file.is_directory() && kFilesSubdirectoryWhitelist.count(filename) > 0) ||
            (file.is_regular_file() && kFilesArchiveExtensions.count(extension) > 0)) {
            container = true;
        } else if (file.is_regular_file() && (kFilesExtensionBlacklist.count(extension) == 0 && extension != ".txt")) {
            container = false;
        } else {
            continue;
        }
        std::shared_ptr<ResourceId> resId;
        if (!container) {
            auto dotFirstIdx = filename.find_first_of('.');
            if (dotFirstIdx != -1) {
                auto resRef = filename.substr(0, dotFirstIdx);
                auto type = getResTypeByExt(filename.substr(dotFirstIdx + 1));
                resId = std::make_shared<ResourceId>(std::move(resRef), type);
            }
        }
        auto item = std::make_shared<ResourcesItem>();
        item->displayName = filename;
        item->path = file.path();
        item->container = container;
        item->resId = std::move(resId);
        _resItems.push_back(std::move(item));
    }

    _graphicsOpt.grass = false;
    _graphicsOpt.pbr = false;
    _graphicsOpt.ssao = false;
    _graphicsOpt.ssr = false;
    _graphicsOpt.fxaa = false;
    _graphicsOpt.sharpen = false;

    _clock = std::make_unique<wxClock>();
    _systemModule = std::make_unique<SystemModule>(*_clock);
    _graphicsModule = std::make_unique<GraphicsModule>(_graphicsOpt);
    _audioModule = std::make_unique<AudioModule>(_audioOpt);
    _scriptModule = std::make_unique<ScriptModule>();
    _resourceModule = std::make_unique<ResourceModule>(_gameId, _resourcesPath, _graphicsOpt, _audioOpt, *_graphicsModule, *_audioModule, *_scriptModule);
    _sceneModule = std::make_unique<SceneModule>(_graphicsOpt, *_resourceModule, *_graphicsModule, *_audioModule);

    _modelResViewModel = std::make_unique<ModelResourceViewModel>(*_systemModule, *_graphicsModule, *_resourceModule, *_sceneModule);
}

void ResourceExplorerViewModel::loadTools() {
    _tools.clear();
    _tools.push_back(std::make_shared<KeyBifTool>());
    _tools.push_back(std::make_shared<ErfTool>());
    _tools.push_back(std::make_shared<RimTool>());
    _tools.push_back(std::make_shared<TwoDaTool>());
    _tools.push_back(std::make_shared<TlkTool>());
    _tools.push_back(std::make_shared<LipTool>());
    _tools.push_back(std::make_shared<SsfTool>());
    _tools.push_back(std::make_shared<GffTool>());
    _tools.push_back(std::make_shared<TpcTool>());
    _tools.push_back(std::make_shared<AudioTool>());
    _tools.push_back(std::make_shared<NcsTool>(_gameId));
}

void ResourceExplorerViewModel::loadEngine() {
    if (_engineLoaded || _resourcesPath.empty()) {
        return;
    }
    info("Loading engine");
    _engineLoadRequested = true;

    _systemModule->init();
    _graphicsModule->init();
    _audioModule->init();
    _resourceModule->init();
    _sceneModule->init();

    auto keyPath = findFileIgnoreCase(_resourcesPath, "chitin.key");
    if (!keyPath) {
        _resourceModule->resources().addFolder(_resourcesPath);
    }

    _modelResViewModel->initScene();

    _engineLoaded = true;
}

void ResourceExplorerViewModel::decompile(ResourcesItemId itemId, bool optimize) {
    auto &item = *_idToResItem.at(itemId);

    withResourceStream(item, [this, &item, &optimize](auto &res) {
        auto nssBytes = ByteBuffer();
        auto nss = MemoryOutputStream(nssBytes);
        NcsTool(_gameId).toNSS(res, nss, *_routines, optimize);

        auto page = std::make_shared<Page>(PageType::NSS, str(boost::format("%s.nss") % item.resId->resRef.value()), *item.resId);
        page->viewModel = std::make_shared<NSSResourceViewModel>(std::string(nssBytes.begin(), nssBytes.end()));
        _pages.add(std::move(page));
    });
}

void ResourceExplorerViewModel::extractArchive(const std::filesystem::path &srcPath, const std::filesystem::path &destPath) {
    auto extension = boost::to_lower_copy(srcPath.extension().string());
    if (extension == ".bif") {
        auto keyPath = getFileIgnoreCase(_resourcesPath, "chitin.key");
        auto key = FileInputStream(keyPath);
        auto keyReader = KeyReader(key);
        keyReader.load();
        auto filename = boost::to_lower_copy(srcPath.filename().string());
        auto maybeBif = std::find_if(keyReader.files().begin(), keyReader.files().end(), [&filename](auto &file) {
            return boost::contains(boost::to_lower_copy(file.filename), filename);
        });
        if (maybeBif == keyReader.files().end()) {
            return;
        }
        auto bifIdx = std::distance(keyReader.files().begin(), maybeBif);
        KeyBifTool().extractBIF(keyReader, bifIdx, srcPath, destPath);
    } else if (extension == ".erf" || extension == ".sav" || extension == ".mod") {
        auto erf = FileInputStream(srcPath);
        auto erfReader = ErfReader(erf);
        erfReader.load();
        ErfTool().extract(erfReader, srcPath, destPath);
    } else if (extension == ".rim") {
        auto rim = FileInputStream(srcPath);
        auto rimReader = RimReader(rim);
        rimReader.load();
        RimTool().extract(rimReader, srcPath, destPath);
    }
}

void ResourceExplorerViewModel::exportFile(ResourcesItemId itemId, const std::filesystem::path &destPath) {
    auto &item = *_idToResItem.at(itemId);
    withResourceStream(item, [&destPath, &item](auto &res) {
        auto exportedPath = destPath;
        exportedPath.append(item.resId->string());
        auto exported = FileOutputStream(exportedPath);
        auto buffer = ByteBuffer();
        buffer.resize(8192);
        bool eof = false;
        while (!eof) {
            int bytesRead = res.read(&buffer[0], buffer.size());
            if (bytesRead < buffer.size()) {
                eof = true;
            }
            exported.write(&buffer[0], bytesRead);
        }
    });
}

void ResourceExplorerViewModel::extractAllBifs(const std::filesystem::path &destPath) {
    auto tool = KeyBifTool();

    auto keyPath = getFileIgnoreCase(_resourcesPath, "chitin.key");
    auto key = FileInputStream(keyPath);
    auto keyReader = KeyReader(key);
    keyReader.load();

    auto progress = Progress();
    progress.visible = true;
    progress.title = "Extract all BIF archives";
    _progress = progress;

    int bifIdx = 0;
    for (auto &file : _keyFiles) {
        auto cleanedFilename = boost::replace_all_copy(file.filename, "\\", "/");
        auto bifPath = findFileIgnoreCase(_resourcesPath, cleanedFilename);
        if (!bifPath) {
            continue;
        }
        progress.value = 100 * bifIdx / static_cast<int>(_keyFiles.size());
        tool.extractBIF(keyReader, bifIdx++, *bifPath, destPath);
        _progress = progress;
    }

    progress.visible = false;
    _progress = progress;
}

void ResourceExplorerViewModel::batchConvertTpcToTga(const std::filesystem::path &srcPath, const std::filesystem::path &destPath) {
    std::vector<std::filesystem::path> tpcFiles;
    for (auto &file : std::filesystem::directory_iterator(srcPath)) {
        if (!file.is_regular_file()) {
            continue;
        }
        auto extension = boost::to_lower_copy(file.path().extension().string());
        if (extension == ".tpc") {
            tpcFiles.push_back(file.path());
        }
    }

    auto progress = Progress();
    progress.visible = true;
    progress.title = "Batch convert TPC to TGA/TXI";
    _progress = progress;

    auto tool = TpcTool();
    for (size_t i = 0; i < tpcFiles.size(); ++i) {
        progress.value = 100 * static_cast<int>(i / tpcFiles.size());
        _progress = progress;

        auto &tpcPath = tpcFiles[i];
        tool.toTGA(tpcPath, destPath);
    }

    progress.visible = false;
    _progress = progress;
}

bool ResourceExplorerViewModel::invokeTool(Operation operation,
                                           const std::filesystem::path &srcPath,
                                           const std::filesystem::path &destPath) {
    for (auto &tool : _tools) {
        if (!tool->supports(operation, srcPath)) {
            continue;
        }
        tool->invoke(operation, srcPath, destPath, _resourcesPath);
        return true;
    }
    return false;
}

void ResourceExplorerViewModel::withResourceStream(const ResourcesItem &item, std::function<void(IInputStream &)> block) {
    if (!item.resId) {
        return;
    }
    if (item.archived) {
        auto extension = boost::to_lower_copy(item.path.extension().string());
        if (extension == ".bif") {
            auto maybeKey = std::find_if(_keyKeys.begin(), _keyKeys.end(), [&item](auto &key) {
                return key.resId == *item.resId;
            });
            if (maybeKey == _keyKeys.end()) {
                return;
            }
            auto resIdx = maybeKey->resIdx;
            auto bif = FileInputStream(item.path);
            auto bifReader = BifReader(bif);
            bifReader.load();
            if (bifReader.resources().size() <= resIdx) {
                return;
            }
            auto &bifEntry = bifReader.resources().at(resIdx);
            auto resBytes = ByteBuffer();
            resBytes.resize(bifEntry.fileSize);
            bif.seek(bifEntry.offset, SeekOrigin::Begin);
            bif.read(&resBytes[0], bifEntry.fileSize);
            auto res = MemoryInputStream(resBytes);
            block(res);
        } else if (extension == ".erf" || extension == ".sav" || extension == ".mod") {
            auto erf = FileInputStream(item.path);
            auto erfReader = ErfReader(erf);
            erfReader.load();
            auto maybeKey = std::find_if(erfReader.keys().begin(), erfReader.keys().end(), [&item](auto &key) {
                return key.resId == *item.resId;
            });
            if (maybeKey == erfReader.keys().end()) {
                return;
            }
            auto resIdx = std::distance(erfReader.keys().begin(), maybeKey);
            auto &erfEntry = erfReader.resources().at(resIdx);
            auto resBytes = ByteBuffer();
            resBytes.resize(erfEntry.size);
            erf.seek(erfEntry.offset, SeekOrigin::Begin);
            erf.read(&resBytes[0], erfEntry.size);
            auto res = MemoryInputStream(resBytes);
            block(res);
        } else if (extension == ".rim") {
            auto rim = FileInputStream(item.path);
            auto rimReader = RimReader(rim);
            rimReader.load();
            auto maybeRes = std::find_if(rimReader.resources().begin(), rimReader.resources().end(), [&item](auto &res) {
                return res.resId == *item.resId;
            });
            if (maybeRes == rimReader.resources().end()) {
                return;
            }
            auto &rimRes = *maybeRes;
            auto resBytes = ByteBuffer();
            resBytes.resize(rimRes.size);
            rim.seek(rimRes.offset, SeekOrigin::Begin);
            rim.read(&resBytes[0], rimRes.size);
            auto res = MemoryInputStream(resBytes);
            block(res);
        }
    } else {
        auto res = FileInputStream(item.path);
        block(res);
    }
}

void ResourceExplorerViewModel::onViewCreated() {
    loadTools();
}

void ResourceExplorerViewModel::onViewDestroyed() {
    _audioResViewModel->audioStream() = nullptr;
}

void ResourceExplorerViewModel::onNotebookPageClose(int page) {
    auto resId = _pages.at(page)->resourceId;
    _pages.removeAt(page);
    if (resId.type == ResType::Mdl) {
        _renderEnabled = false;
    }
    if (resId.type == ResType::Wav) {
        _audioResViewModel->audioStream() = nullptr;
    }
}

void ResourceExplorerViewModel::onResourcesDirectoryChanged(GameID gameId, std::filesystem::path path) {
    _gameId = gameId;
    _resourcesPath = path;
    _resItems.clear();
    _idToResItem.clear();

    loadResources();
    loadTools();
}

void ResourceExplorerViewModel::onResourcesItemIdentified(int index, ResourcesItemId id) {
    auto &item = _resItems[index];
    item->id = id;
    _idToResItem.insert(std::make_pair(id, item.get()));
}

void ResourceExplorerViewModel::onResourcesItemExpanding(ResourcesItemId id) {
    if (_idToResItem.count(id) == 0) {
        return;
    }
    auto &expandingItem = *_idToResItem.at(id);
    if (std::filesystem::is_directory(expandingItem.path)) {
        for (auto &file : std::filesystem::directory_iterator(expandingItem.path)) {
            auto filename = boost::to_lower_copy(file.path().filename().string());
            auto extension = boost::to_lower_copy(file.path().extension().string());
            bool container;
            if (file.is_directory() || kFilesArchiveExtensions.count(extension) > 0) {
                container = true;
            } else if (file.is_regular_file() && kFilesExtensionBlacklist.count(extension) == 0) {
                container = false;
            } else {
                continue;
            }
            auto item = std::make_shared<ResourcesItem>();
            item->parentId = expandingItem.id;
            item->displayName = filename;
            item->path = file.path();
            if (!extension.empty()) {
                auto resType = getResTypeByExt(extension.substr(1), false);
                if (resType != ResType::Invalid) {
                    auto resRef = filename.substr(0, filename.size() - 4);
                    item->resId = std::make_shared<ResourceId>(resRef, resType);
                }
            }
            item->container = container;
            _resItems.push_back(std::move(item));
        }
    } else {
        auto extension = boost::to_lower_copy(expandingItem.path.extension().string());
        if (boost::ends_with(extension, ".bif")) {
            auto filename = str(boost::format("data/%s") % boost::to_lower_copy(expandingItem.path.filename().string()));
            auto maybeFile = std::find_if(_keyFiles.begin(), _keyFiles.end(), [&filename](auto &file) {
                return boost::to_lower_copy(file.filename) == filename;
            });
            if (maybeFile != _keyFiles.end()) {
                auto bifIdx = std::distance(_keyFiles.begin(), maybeFile);
                for (auto &key : _keyKeys) {
                    if (key.bifIdx != bifIdx) {
                        continue;
                    }
                    auto item = std::make_shared<ResourcesItem>();
                    item->parentId = expandingItem.id;
                    item->displayName = str(boost::format("%s.%s") % key.resId.resRef.value() % getExtByResType(key.resId.type));
                    item->path = expandingItem.path;
                    item->resId = std::make_shared<ResourceId>(key.resId);
                    item->archived = true;
                    _resItems.push_back(std::move(item));
                }
            }
        } else if (boost::ends_with(extension, ".erf") || boost::ends_with(extension, ".sav") || boost::ends_with(extension, ".mod")) {
            auto erf = FileInputStream(expandingItem.path);
            auto erfReader = ErfReader(erf);
            erfReader.load();
            auto &keys = erfReader.keys();
            for (auto &key : keys) {
                auto item = std::make_shared<ResourcesItem>();
                item->parentId = expandingItem.id;
                item->displayName = str(boost::format("%s.%s") % key.resId.resRef.value() % getExtByResType(key.resId.type));
                item->path = expandingItem.path;
                item->resId = std::make_shared<ResourceId>(key.resId);
                item->archived = true;
                _resItems.push_back(std::move(item));
            }
        } else if (boost::ends_with(extension, ".rim")) {
            auto rim = FileInputStream(expandingItem.path);
            auto rimReader = RimReader(rim);
            rimReader.load();
            auto &resources = rimReader.resources();
            for (auto &resource : resources) {
                auto item = std::make_shared<ResourcesItem>();
                item->parentId = expandingItem.id;
                item->displayName = str(boost::format("%s.%s") % resource.resId.resRef.value() % getExtByResType(resource.resId.type));
                item->path = expandingItem.path;
                item->resId = std::make_shared<ResourceId>(resource.resId);
                item->archived = true;
                _resItems.push_back(std::move(item));
            }
        }
    }
    expandingItem.loaded = true;
}

void ResourceExplorerViewModel::onResourcesItemActivated(ResourcesItemId id) {
    auto &item = *_idToResItem.at(id);
    openFile(item);
}

} // namespace reone