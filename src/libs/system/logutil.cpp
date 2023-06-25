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

#include "reone/system/logutil.h"

#include "reone/system/collectionutil.h"

namespace reone {

static std::thread::id g_mainThreadId;

static int g_channels = LogChannels::general;
static LogLevel g_level = LogLevel::Info;

static bool g_logToFile = false;
static std::unique_ptr<boost::filesystem::ofstream> g_logFile;
static std::string g_logFilename = "reone.log";

static const std::unordered_map<LogLevel, std::string> g_nameByLogLevel {
    {LogLevel::Error, "ERR"},
    {LogLevel::Warn, "WRN"},
    {LogLevel::Info, "INF"},
    {LogLevel::Debug, "DBG"}};

static std::string describeLogLevel(LogLevel level) {
    return getFromLookupOrElse(g_nameByLogLevel, level, [&level]() {
        return std::to_string(static_cast<int>(level));
    });
}

static void log(std::ostream &out, LogLevel level, const std::string &s, int channel) {
    auto msg = boost::format("%s %s") % describeLogLevel(level) % s;
    out << msg << std::endl;
}

static void log(LogLevel level, const std::string &s, int channel) {
    if (!isLogLevelEnabled(level)) {
        return;
    }
    if (!isLogChannelEnabled(channel)) {
        return;
    }
    if (std::this_thread::get_id() != g_mainThreadId) {
        throw std::logic_error("Must not log outside the main thread");
    }
    if (g_logToFile && !g_logFile) {
        boost::filesystem::path path(boost::filesystem::current_path());
        path.append(g_logFilename);
        g_logFile = std::make_unique<boost::filesystem::ofstream>(path);
    }
    auto &out = g_logToFile ? *g_logFile : std::cout;
    log(out, level, s, channel);
}

void initLog() {
    g_mainThreadId = std::this_thread::get_id();
}

void error(const std::string &s, int channel) {
    log(LogLevel::Error, s, channel);
}

void error(const boost::format &s, int channel) {
    log(LogLevel::Error, str(s), channel);
}

void warn(const std::string &s, int channel) {
    log(LogLevel::Warn, s, channel);
}

void warn(const boost::format &s, int channel) {
    log(LogLevel::Warn, str(s), channel);
}

void info(const std::string &s, int channel) {
    log(LogLevel::Info, s, channel);
}

void info(const boost::format &s, int channel) {
    log(LogLevel::Info, str(s), channel);
}

void debug(const std::string &s, int channel) {
    log(LogLevel::Debug, s, channel);
}

void debug(const boost::format &s, int channel) {
    return debug(str(s), channel);
}

bool isLogLevelEnabled(LogLevel level) {
    return static_cast<int>(level) <= static_cast<int>(g_level);
}

bool isLogChannelEnabled(int channel) {
    return g_channels & channel;
}

void setLogLevel(LogLevel level) {
    g_level = level;
}

void setLogChannels(int mask) {
    g_channels = mask;
}

void setLogToFile(bool log) {
    g_logToFile = log;
}

void setLogFilename(std::string filename) {
    g_logFilename = filename;
}

} // namespace reone
