#include "log.h"

void Logging::SetLogFilePath(const std::string& filePath)
{
    m_logFilePath = filePath;
}

std::string Logging::GetLogFilePath()
{
    return m_logFilePath;
}

void Logging::EnableLogging(bool enable)
{
    m_enableLogging = enable;
    axter::ezlogger<>::set_verbosity_level_tolerance(enable ? axter::log_default_verbosity_level : axter::log_verbosity_not_set);
}

bool Logging::LoggingIsEnabled()
{
    return m_enableLogging;
}

std::string Logging::m_logFilePath;
bool Logging::m_enableLogging = false;
