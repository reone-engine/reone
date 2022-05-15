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

#include "ncs.h"

#include <boost/regex.hpp>

#include "../../common/collectionutil.h"
#include "../../common/exception/argument.h"
#include "../../common/exception/notimplemented.h"
#include "../../common/exception/validation.h"
#include "../../common/stream/fileinput.h"
#include "../../common/stream/fileoutput.h"
#include "../../common/textwriter.h"
#include "../../game/script/routines.h"
#include "../../script/format/ncsreader.h"
#include "../../script/format/ncswriter.h"
#include "../../script/instrutil.h"
#include "../../script/program.h"
#include "../../script/routine.h"
#include "../../script/variable.h"

#include "nwscript/program.h"

using namespace std;

using namespace reone::game;
using namespace reone::resource;
using namespace reone::script;

namespace fs = boost::filesystem;

namespace reone {

static void initRoutines(GameID gameId, Routines &routines) {
    if (gameId == GameID::TSL) {
        routines.initForTSL();
    } else {
        routines.initForKotOR();
    }
}

class PcodeReader {
public:
    PcodeReader(fs::path path, Routines &routines) :
        _path(move(path)),
        _routines(routines) {
    }

    void load() {
        vector<string> insLines;
        map<int, string> labelByLineIdx;
        map<int, uint32_t> addrByLineIdx;

        fs::ifstream pcode(_path);
        string line;
        boost::smatch what;
        boost::regex re("^([_\\d\\w]+):$");
        uint32_t addr = 13;
        while (getline(pcode, line)) {
            boost::trim(line);
            if (line.empty()) {
                continue;
            }
            int lineIdx = static_cast<int>(insLines.size());
            if (boost::regex_match(line, what, re)) {
                labelByLineIdx[lineIdx] = what[1].str();
                continue;
            }
            addrByLineIdx[lineIdx] = addr;
            addr += getInstructionSize(line);
            insLines.push_back(line);
        }

        fs::path filename(_path.filename());
        filename.replace_extension(); // drop .pcode
        filename.replace_extension(); // drop .ncs

        _addrByLabel.clear();
        for (auto &pair : labelByLineIdx) {
            _addrByLabel[pair.second] = addrByLineIdx[pair.first];
        }

        _program = make_shared<ScriptProgram>(filename.string());
        for (size_t i = 0; i < insLines.size(); ++i) {
            uint32_t insAddr = addrByLineIdx.find(static_cast<int>(i))->second;
            _program->add(parseInstruction(insLines[i], insAddr));
        }
    }

    shared_ptr<ScriptProgram> program() { return _program; }

private:
    fs::path _path;
    Routines &_routines;

    shared_ptr<ScriptProgram> _program;
    map<string, uint32_t> _addrByLabel;

    int getInstructionSize(const string &line) {
        int result = 2;

        size_t spaceIdx = line.find(" ");
        string typeDesc(spaceIdx != string::npos ? line.substr(0, spaceIdx) : line);
        string argsLine(line.substr(typeDesc.length()));
        InstructionType type = parseInstructionType(typeDesc);

        switch (type) {
        case InstructionType::CPDOWNSP:
        case InstructionType::CPTOPSP:
        case InstructionType::CPDOWNBP:
        case InstructionType::CPTOPBP:
        case InstructionType::DESTRUCT:
            result += 6;
            break;
        case InstructionType::CONSTI:
        case InstructionType::CONSTF:
        case InstructionType::CONSTO:
        case InstructionType::MOVSP:
        case InstructionType::JMP:
        case InstructionType::JSR:
        case InstructionType::JZ:
        case InstructionType::JNZ:
        case InstructionType::DECISP:
        case InstructionType::INCISP:
        case InstructionType::DECIBP:
        case InstructionType::INCIBP:
            result += 4;
            break;
        case InstructionType::CONSTS:
            result += 2;
            applyArguments(argsLine, "^ \"(.*)\"$", 1, [&result](auto &args) {
                result += args[0].length();
            });
            break;
        case InstructionType::ACTION:
            result += 3;
            break;
        case InstructionType::STORE_STATE:
            result += 8;
            break;
        case InstructionType::EQUALTT:
        case InstructionType::NEQUALTT:
            result += 2;
            break;
        default:
            break;
        };

        return result;
    }

    Instruction parseInstruction(const string &line, uint32_t addr) const {
        size_t spaceIdx = line.find(" ");
        string typeDesc(spaceIdx != string::npos ? line.substr(0, spaceIdx) : line);
        string argsLine(line.substr(typeDesc.length()));
        InstructionType type = parseInstructionType(typeDesc);

        Instruction ins;
        ins.offset = addr;
        ins.type = type;

        switch (type) {
        case InstructionType::CPDOWNSP:
        case InstructionType::CPTOPSP:
        case InstructionType::CPDOWNBP:
        case InstructionType::CPTOPBP:
            applyArguments(argsLine, "^ ([-\\d]+), (\\d+)$", 2, [&ins](auto &args) {
                ins.stackOffset = atoi(args[0].c_str());
                ins.size = atoi(args[1].c_str());
            });
            break;
        case InstructionType::CONSTI:
            applyArguments(argsLine, "^ ([-\\d]+)$", 1, [&ins](auto &args) {
                ins.intValue = atoi(args[0].c_str());
            });
            break;
        case InstructionType::CONSTF:
            applyArguments(argsLine, "^ ([-\\.\\d]+)$", 1, [&ins](auto &args) {
                ins.floatValue = atof(args[0].c_str());
            });
            break;
        case InstructionType::CONSTS:
            applyArguments(argsLine, "^ \"(.*)\"$", 1, [&ins](auto &args) {
                ins.strValue = args[0];
            });
            break;
        case InstructionType::CONSTO:
            applyArguments(argsLine, "^ (\\d+)$", 1, [&ins](auto &args) {
                ins.objectId = atoi(args[0].c_str());
            });
            break;
        case InstructionType::ACTION:
            applyArguments(argsLine, "^ (\\w+), (\\d+)$", 2, [this, &ins](auto &args) {
                ins.routine = _routines.getIndexByName(args[0]);
                ins.argCount = atoi(args[1].c_str());
            });
            break;
        case InstructionType::MOVSP:
            applyArguments(argsLine, "^ ([-\\d]+)$", 1, [&ins](auto &args) {
                ins.stackOffset = atoi(args[0].c_str());
            });
            break;
        case InstructionType::JMP:
        case InstructionType::JSR:
        case InstructionType::JZ:
        case InstructionType::JNZ:
            applyArguments(argsLine, "^ ([_\\d\\w]+)$", 1, [this, &ins](auto &args) {
                const string &label = args[0];
                auto maybeAddr = _addrByLabel.find(label);
                if (maybeAddr == _addrByLabel.end()) {
                    throw ValidationException("Instruction address not found by label '" + label + "'");
                }
                ins.jumpOffset = maybeAddr->second - ins.offset;
            });
            break;
        case InstructionType::DESTRUCT:
            applyArguments(argsLine, "^ (\\d+), ([-\\d]+), (\\d+)$", 3, [&ins](auto &args) {
                ins.size = atoi(args[0].c_str());
                ins.stackOffset = atoi(args[1].c_str());
                ins.sizeNoDestroy = atoi(args[2].c_str());
            });
            break;
        case InstructionType::DECISP:
        case InstructionType::INCISP:
        case InstructionType::DECIBP:
        case InstructionType::INCIBP:
            applyArguments(argsLine, "^ ([-\\d]+)$", 1, [&ins](auto &args) {
                ins.stackOffset = atoi(args[0].c_str());
            });
            break;
        case InstructionType::STORE_STATE:
            applyArguments(argsLine, "^ (\\d+), (\\d+)$", 2, [&ins](auto &args) {
                ins.size = atoi(args[0].c_str());
                ins.sizeLocals = atoi(args[1].c_str());
            });
            break;
        case InstructionType::EQUALTT:
        case InstructionType::NEQUALTT:
            applyArguments(argsLine, "^ (\\d+)$", 1, [&ins](auto &args) {
                ins.size = atoi(args[0].c_str());
            });
            break;
        default:
            break;
        };

        return move(ins);
    }

    void applyArguments(const string &line, const string &restr, int numArgs, const function<void(const vector<string> &)> &fn) const {
        boost::smatch what;
        boost::regex re(restr);
        if (!boost::regex_match(line, what, re)) {
            throw invalid_argument(str(boost::format("Arguments line '%s' must match regular expression '%s'") % line % restr));
        }
        vector<string> args;
        for (int i = 0; i < numArgs; ++i) {
            args.push_back(what[1 + i].str());
        }
        fn(move(args));
    }
};

class PcodeWriter {
public:
    PcodeWriter(ScriptProgram &program, Routines &routines) :
        _program(program),
        _routines(routines) {
    }

    void save(const fs::path &path) {
        fs::ofstream pcode(path);
        try {
            set<uint32_t> jumpOffsets;
            for (auto &instr : _program.instructions()) {
                switch (instr.type) {
                case InstructionType::JMP:
                case InstructionType::JSR:
                case InstructionType::JZ:
                case InstructionType::JNZ:
                    jumpOffsets.insert(instr.offset + instr.jumpOffset);
                    break;
                default:
                    break;
                }
            }
            for (auto &instr : _program.instructions()) {
                writeInstruction(instr, pcode, jumpOffsets);
            }
        } catch (const exception &e) {
            fs::remove(path);
            throw runtime_error(e.what());
        }
    }

private:
    ScriptProgram &_program;
    Routines &_routines;

    void writeInstruction(const Instruction &ins, fs::ofstream &pcode, const set<uint32_t> &jumpOffsets) {
        if (jumpOffsets.count(ins.offset) > 0) {
            string label(str(boost::format("loc_%08x:") % ins.offset));
            pcode << label << endl;
        }

        string desc(describeInstructionType(ins.type));

        switch (ins.type) {
        case InstructionType::CPDOWNSP:
        case InstructionType::CPTOPSP:
        case InstructionType::CPDOWNBP:
        case InstructionType::CPTOPBP:
            desc += str(boost::format(" %d, %d") % ins.stackOffset % ins.size);
            break;
        case InstructionType::CONSTI:
            desc += " " + to_string(ins.intValue);
            break;
        case InstructionType::CONSTF:
            desc += " " + to_string(ins.floatValue);
            break;
        case InstructionType::CONSTS:
            desc += " \"" + ins.strValue + "\"";
            break;
        case InstructionType::CONSTO:
            desc += " " + to_string(ins.objectId);
            break;
        case InstructionType::ACTION:
            desc += str(boost::format(" %s, %d") % _routines.get(ins.routine).name() % ins.argCount);
            break;
        case InstructionType::EQUALTT:
        case InstructionType::NEQUALTT:
            desc += " " + to_string(ins.size);
            break;
        case InstructionType::MOVSP:
            desc += " " + to_string(ins.stackOffset);
            break;
        case InstructionType::JMP:
        case InstructionType::JSR:
        case InstructionType::JZ:
        case InstructionType::JNZ: {
            uint32_t jumpAddr = ins.offset + ins.jumpOffset;
            desc += str(boost::format(" loc_%08x") % jumpAddr);
            break;
        }
        case InstructionType::DESTRUCT:
            desc += str(boost::format(" %d, %d, %d") % ins.size % ins.stackOffset % ins.sizeNoDestroy);
            break;
        case InstructionType::DECISP:
        case InstructionType::INCISP:
        case InstructionType::DECIBP:
        case InstructionType::INCIBP:
            desc += " " + to_string(ins.stackOffset);
            break;
        case InstructionType::STORE_STATE:
            desc += str(boost::format(" %d, %d") % ins.size % ins.sizeLocals);
            break;
        default:
            break;
        }

        pcode << desc << endl;
    }
};

class NssWriter {
public:
    NssWriter(
        NwscriptProgram &program,
        Routines &routines) :
        _program(program),
        _routines(routines) {
    }

    void save(IOutputStream &stream) {
        auto writer = TextWriter(stream);
        auto writtenOffsets = set<uint32_t>();
        for (auto &function : _program.functions()) {
            if (writtenOffsets.count(function->offset) > 0) {
                continue;
            }
            writeFunction(*function, writer);
            writtenOffsets.insert(function->offset);
        }
    }

private:
    NwscriptProgram &_program;
    Routines &_routines;

    void writeFunction(const NwscriptProgram::Function &function, TextWriter &writer) {
        auto returnType = describeVariableType(function.returnType);
        auto name = describeFunction(function);
        auto params = vector<string>();
        int paramIdx;
        paramIdx = 0;
        for (auto &paramType : function.inArgumentTypes) {
            auto type = describeVariableType(paramType);
            params.push_back(str(boost::format("%s in_%d") % type % (paramIdx++)));
        }
        paramIdx = 0;
        for (auto &paramType : function.outArgumentTypes) {
            auto type = describeVariableType(paramType);
            params.push_back(str(boost::format("%s &out_%d") % type % (paramIdx++)));
        }
        writer.putLine(str(boost::format("%s %s(%s)") % returnType % name % boost::join(params, ", ")));

        writeBlock(0, *function.block, writer);

        writer.put("\n\n");
    }

    void writeBlock(int level, const NwscriptProgram::BlockExpression &block, TextWriter &writer) {
        auto innerLevel = 1 + level;
        auto indent = indentAtLevel(level);
        auto innerIndent = indentAtLevel(innerLevel);

        writer.putLine(indent + string("{"));
        for (auto &innerExpr : block.expressions) {
            writer.put(innerIndent);
            writeExpression(innerLevel, true, *innerExpr, writer);
            if (innerExpr->type != NwscriptProgram::ExpressionType::Conditional) {
                writer.putLine(";");
            }
        }
        writer.put(indent + string("}"));
    }

    void writeExpression(int blockLevel, bool leftmost, const NwscriptProgram::Expression &expression, TextWriter &writer) {
        auto indent = indentAtLevel(blockLevel);

        if (expression.type == NwscriptProgram::ExpressionType::Return) {
            auto &returnExpr = static_cast<const NwscriptProgram::ReturnExpression &>(expression);
            writer.put("return");
            if (returnExpr.value) {
                writer.put(" ");
                writeExpression(blockLevel, false, *returnExpr.value, writer);
            }

        } else if (expression.type == NwscriptProgram::ExpressionType::Constant) {
            auto &constExpr = static_cast<const NwscriptProgram::ConstantExpression &>(expression);
            auto value = describeConstant(constExpr);
            writer.put(value);

        } else if (expression.type == NwscriptProgram::ExpressionType::Parameter) {
            auto &paramExpr = static_cast<const NwscriptProgram::ParameterExpression &>(expression);
            auto name = describeParameter(paramExpr);
            if (leftmost) {
                auto type = describeVariableType(paramExpr.variableType);
                writer.put(str(boost::format("%s %s") % type % name));
            } else {
                writer.put(name);
            }

        } else if (expression.type == NwscriptProgram::ExpressionType::Call) {
            auto &callExpr = static_cast<const NwscriptProgram::CallExpression &>(expression);
            auto name = describeFunction(*callExpr.function);
            auto params = vector<string>();
            for (auto &param : callExpr.arguments) {
                auto name = describeParameter(*param);
                params.push_back(name);
            }
            writer.put(str(boost::format("%s(%s)") % name % boost::join(params, ", ")));

        } else if (expression.type == NwscriptProgram::ExpressionType::Action) {
            auto &actionExpr = static_cast<const NwscriptProgram::ActionExpression &>(expression);
            auto name = describeAction(actionExpr);
            writer.put(name + "(");
            for (size_t i = 0; i < actionExpr.arguments.size(); ++i) {
                if (i > 0) {
                    writer.put(", ");
                }
                auto argExpr = actionExpr.arguments[i];
                if (argExpr->type == NwscriptProgram::ExpressionType::Parameter) {
                    auto paramExpr = static_cast<NwscriptProgram::ParameterExpression *>(argExpr);
                    auto name = describeParameter(*paramExpr);
                    writer.put(name);
                } else if (argExpr->type == NwscriptProgram::ExpressionType::Block) {
                    auto blockExpr = static_cast<NwscriptProgram::BlockExpression *>(argExpr);
                    writer.putLine("[&]()");
                    writeBlock(blockLevel, *blockExpr, writer);
                } else {
                    throw ValidationException("Action argument is neither parameter nor block expression");
                }
            }
            writer.put(")");

        } else if (expression.type == NwscriptProgram::ExpressionType::Assign ||
                   expression.type == NwscriptProgram::ExpressionType::Equal ||
                   expression.type == NwscriptProgram::ExpressionType::NotEqual) {
            auto &binaryExpr = static_cast<const NwscriptProgram::BinaryExpression &>(expression);
            string operation;
            if (expression.type == NwscriptProgram::ExpressionType::Assign) {
                operation = "=";
            } else if (expression.type == NwscriptProgram::ExpressionType::Equal) {
                operation = "==";
            } else if (expression.type == NwscriptProgram::ExpressionType::NotEqual) {
                operation = "!=";
            }
            writeExpression(blockLevel, false, *binaryExpr.left, writer);
            writer.put(str(boost::format(" %s ") % operation));
            writeExpression(blockLevel, false, *binaryExpr.right, writer);

        } else if (expression.type == NwscriptProgram::ExpressionType::Conditional) {
            auto &condExpr = static_cast<const NwscriptProgram::ConditionalExpression &>(expression);
            writer.put("if(");
            writeExpression(blockLevel, false, *condExpr.test, writer);
            writer.putLine(")");
            writeBlock(blockLevel, *condExpr.ifTrue, writer);
            writer.putLine("");
            if (condExpr.ifFalse) {
                writer.putLine(indent + "else");
                writeBlock(blockLevel, *condExpr.ifFalse, writer);
                writer.putLine("");
            }

        } else {
            throw NotImplementedException("Cannot write expression of type: " + to_string(static_cast<int>(expression.type)));
        }
    }

    std::string indentAtLevel(int level) {
        return string(4 * level, ' ');
    }

    std::string describeFunction(const NwscriptProgram::Function &function) {
        return !function.name.empty() ? function.name : str(boost::format("fun_%08x") % function.offset);
    }

    std::string describeConstant(const NwscriptProgram::ConstantExpression &constExpr) {
        if (constExpr.value.type == VariableType::Int) {
            return to_string(constExpr.value.intValue);
        } else if (constExpr.value.type == VariableType::Float) {
            return str(boost::format("%ff") % constExpr.value.floatValue);
        } else if (constExpr.value.type == VariableType::String) {
            return str(boost::format("\"%s\"") % constExpr.value.strValue);
        } else if (constExpr.value.type == VariableType::Object) {
            return to_string(constExpr.value.objectId);
        } else {
            throw ArgumentException("Cannot describe constant expression of type: " + to_string(static_cast<int>(constExpr.value.type)));
        }
    }

    std::string describeParameter(const NwscriptProgram::ParameterExpression &paramExpr) {
        if (paramExpr.locality == NwscriptProgram::ParameterLocality::Local) {
            if (paramExpr.index > 0) {
                return str(boost::format("var_%08x_%d") % paramExpr.offset % paramExpr.index);
            } else {
                return str(boost::format("var_%08x") % paramExpr.offset);
            }
        } else if (paramExpr.locality == NwscriptProgram::ParameterLocality::Input) {
            return str(boost::format("in_%d") % paramExpr.index);
        } else if (paramExpr.locality == NwscriptProgram::ParameterLocality::Output) {
            return str(boost::format("out_%d") % paramExpr.index);
        } else {
            throw ArgumentException("Unsupported parameter locality: " + to_string(static_cast<int>(paramExpr.locality)));
        }
    }

    std::string describeAction(const NwscriptProgram::ActionExpression &actionExpr) {
        auto numRoutines = _routines.getNumRoutines();
        if (actionExpr.action >= numRoutines) {
            throw ArgumentException(str(boost::format("Action number out of bounds: %d/%d") % actionExpr.action % numRoutines));
        }
        return _routines.get(actionExpr.action).name();
    }

    std::string describeVariableType(VariableType type) {
        switch (type) {
        case VariableType::Void:
            return "void";
        case VariableType::Int:
            return "int";
        case VariableType::Float:
            return "float";
        case VariableType::String:
            return "string";
        case VariableType::Vector:
            return "vector";
        case VariableType::Object:
            return "object";
        case VariableType::Effect:
            return "effect";
        case VariableType::Event:
            return "event";
        case VariableType::Location:
            return "location";
        case VariableType::Talent:
            return "talent";
        case VariableType::Action:
            return "action";
        default:
            throw ArgumentException("Cannot describe variable type: " + to_string(static_cast<int>(type)));
        }
    }
};

void NcsTool::invoke(Operation operation, const fs::path &target, const fs::path &gamePath, const fs::path &destPath) {
    if (operation == Operation::ToPCODE) {
        toPCODE(target, destPath);
    } else if (operation == Operation::ToNCS) {
        toNCS(target, destPath);
    } else if (operation == Operation::ToNSS) {
        toNSS(target, destPath);
    }
}

void NcsTool::toPCODE(const fs::path &path, const fs::path &destPath) {
    Routines routines;
    initRoutines(_gameId, routines);

    auto stream = FileInputStream(path, OpenMode::Binary);

    NcsReader ncs("");
    ncs.load(stream);

    fs::path pcodePath(destPath);
    pcodePath.append(path.filename().string() + ".pcode");

    PcodeWriter pcode(*ncs.program(), routines);
    pcode.save(pcodePath);
}

void NcsTool::toNCS(const fs::path &path, const fs::path &destPath) {
    Routines routines;
    initRoutines(_gameId, routines);

    PcodeReader pcode(path, routines);
    pcode.load();
    auto program = pcode.program();

    fs::path ncsPath(destPath);
    ncsPath.append(path.filename().string());
    ncsPath.replace_extension(); // drop .pcode

    NcsWriter writer(*program);
    writer.save(ncsPath);
}

void NcsTool::toNSS(const fs::path &path, const fs::path &destPath) {
    auto routines = Routines();
    initRoutines(_gameId, routines);

    auto ncs = FileInputStream(path, OpenMode::Binary);
    auto reader = NcsReader("");
    reader.load(ncs);
    auto compiledProgram = reader.program();
    auto program = NwscriptProgram::fromCompiled(*compiledProgram, routines);

    auto nssPath = destPath;
    nssPath.append(path.filename().string() + ".nss");
    auto nss = FileOutputStream(nssPath);
    auto writer = NssWriter(program, routines);
    writer.save(nss);
}

bool NcsTool::supports(Operation operation, const fs::path &target) const {
    return !fs::is_directory(target) &&
           ((target.extension() == ".ncs" && (operation == Operation::ToPCODE || operation == Operation::ToNSS)) ||
            (target.extension() == ".pcode" && operation == Operation::ToNCS));
}

} // namespace reone
