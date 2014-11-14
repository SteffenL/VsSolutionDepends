#pragma once

#include <string>
#include <sstream>

class Logging
{
public:
    static void SetLogFilePath(const std::string& filePath);
    static std::string GetLogFilePath();
    static void EnableLogging(bool enable);
    static bool LoggingIsEnabled();

private:
    static std::string m_logFilePath;
    static bool m_enableLogging;
};

#define EZLOGGER_OUTPUT_FILENAME Logging::GetLogFilePath()
#define EZLOGGER_REPLACE_EXISTING_LOGFILE_

#pragma warning(push)
#pragma warning(disable: 4996)
#include <ezlogger_headers.hpp>

#define LOG_DEBUG() EZLOGGERVLSTREAM(axter::levels(axter::log_default_verbosity_level, axter::debug))
#define LOG_INFO() EZLOGGERVLSTREAM(axter::levels(axter::log_default_verbosity_level, axter::info))
#define LOG_WARN() EZLOGGERVLSTREAM(axter::levels(axter::log_default_verbosity_level, axter::warn)) 
#define LOG_ERROR() EZLOGGERVLSTREAM(axter::levels(axter::log_default_verbosity_level, axter::error))

#define LOGVL_DEBUG(verbosityLevel) EZLOGGERVLSTREAM(axter::levels(axter::log_#verbosityLevel, axter::debug))
#define LOGVL_INFO(verbosityLevel) EZLOGGERVLSTREAM(axter::levels(axter::log_#verbosityLevel, axter::info))
#define LOGVL_WARN(verbosityLevel) EZLOGGERVLSTREAM(axter::levels(axter::log_#verbosityLevel, axter::warn))
#define LOGVL_ERROR(verbosityLevel) EZLOGGERVLSTREAM(axter::levels(axter::log_#verbosityLevel, axter::error))
#pragma warning(pop)
