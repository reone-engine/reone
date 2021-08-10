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

#include "program.h"

#include <boost/program_options.hpp>

#include "../engine/common/pathutil.h"
#include "../engine/resource/types.h"

#include "tools.h"
#include "types.h"

using namespace std;

using namespace reone::resource;
using namespace reone::tools;

namespace fs = boost::filesystem;
namespace po = boost::program_options;

namespace reone {

static const unordered_map<string, Operation> g_operations {
    { "list", Operation::List },
    { "extract", Operation::Extract },
    { "unwrap", Operation::Unwrap },
    { "to-json", Operation::ToJSON },
    { "to-tga", Operation::ToTGA },
    { "to-2da", Operation::To2DA },
    { "to-gff", Operation::ToGFF },
    { "to-rim", Operation::ToRIM },
    { "to-erf", Operation::ToERF },
    { "to-mod", Operation::ToMOD },
    { "to-pth", Operation::ToPTH },
    { "to-ascii", Operation::ToASCII },
    { "to-tlk", Operation::ToTLK },
    { "to-lip", Operation::ToLIP }
};

static fs::path getDestination(const po::variables_map &vars) {
    fs::path result;
    if (vars.count("dest") > 0) {
        result = vars["dest"].as<string>();
    } else if (vars.count("target") > 0) {
        result = fs::path(vars["target"].as<string>()).parent_path();
    } else {
        result = fs::current_path();
    }
    return move(result);
}

class Program : boost::noncopyable {
public:
    Program(int argc, char **argv) : _argc(argc), _argv(argv) {
    }

    int run() {
        initOptions();
        loadOptions();
        loadTools();

        switch (_operation) {
            case Operation::None:
                cout << _optsCmdLine << endl;
                break;
            default: {
                auto tool = getTool();
                if (tool) {
                    tool->invoke(_operation, _target, _gamePath, _destPath);
                } else {
                    cout << "Unable to choose a tool for the specified operation" << endl;
                }
                break;
            }
        }

        return 0;
    }

private:
    int _argc;
    char **_argv;

    boost::program_options::options_description _optsCmdLine { "Usage" };

    boost::filesystem::path _gamePath;
    boost::filesystem::path _destPath;
    std::string _target;
    Operation _operation { Operation::None };
    std::vector<std::shared_ptr<ITool>> _tools;

    void initOptions() {
        _optsCmdLine.add_options()
            ("game", po::value<string>(), "path to game directory")
            ("dest", po::value<string>(), "path to destination directory")
            ("list", "list file contents")
            ("extract", "extract file contents")
            ("unwrap", "unwrap an audio file")
            ("to-json", "convert 2DA, GFF or TLK file to JSON")
            ("to-tga", "convert TPC image to TGA")
            ("to-2da", "convert JSON to 2DA")
            ("to-gff", "convert JSON to GFF")
            ("to-rim", "create RIM archive from directory")
            ("to-erf", "create ERF archive from directory")
            ("to-mod", "create MOD archive from directory")
            ("to-pth", "convert ASCII PTH to binary PTH")
            ("to-ascii", "convert binary PTH to ASCII")
            ("to-tlk", "convert JSON to TLK")
            ("to-lip", "convert JSON to LIP")
            ("target", po::value<string>(), "target name or path to input file");
    }

    void loadOptions() {
        po::positional_options_description positional;
        positional.add("target", 1);

        po::parsed_options parsedCmdLineOpts = po::command_line_parser(_argc, _argv)
            .options(_optsCmdLine)
            .positional(positional)
            .run();

        po::variables_map vars;
        po::store(parsedCmdLineOpts, vars);
        po::notify(vars);

        _gamePath = vars.count("game") > 0 ? vars["game"].as<string>() : fs::current_path();
        _destPath = getDestination(vars);
        _target = vars.count("target") > 0 ? vars["target"].as<string>() : "";

        // Determine operation from program options
        for (auto &operation : g_operations) {
            if (vars.count(operation.first)) {
                _operation = operation.second;
                break;
            }
        }
    }

    void loadTools() {
        // Tools are queried in the order of addition, whether they support a
        // particular operation on a particular file, or not. The first tool
        // to return true gets chosen.

        _tools.push_back(make_shared<KeyBifTool>());
        _tools.push_back(make_shared<ErfTool>());
        _tools.push_back(make_shared<RimTool>());
        _tools.push_back(make_shared<TwoDaTool>());
        _tools.push_back(make_shared<TlkTool>());
        _tools.push_back(make_shared<LipTool>());
        _tools.push_back(make_shared<GffTool>());
        _tools.push_back(make_shared<TpcTool>());
        _tools.push_back(make_shared<PthTool>());
        _tools.push_back(make_shared<AudioTool>());
    }

    std::shared_ptr<ITool> getTool() const {
        for (auto &tool : _tools) {
            if (tool->supports(_operation, _target)) return tool;
        }
        return nullptr;
    }
};

int runProgram(int argc, char **argv) {
    return Program(argc, argv).run();
}

} // namespace reone
