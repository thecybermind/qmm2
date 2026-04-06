/*
QMM2 - Q3 MultiMod 2
Copyright 2025-2026
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#ifndef QMM2_LOG_H
#define QMM2_LOG_H

#ifdef __cplusplus

#include <aixlog/aixlog.hpp>
#include <string>

/**
* @brief Log message.
* 
* If messages with given severity are not being logged to any sink, this entire log expression will not evaluate.
*
* @param severity Log severity
* @param tag Log tag
*/
#define QMMLOG(severity, tag) if (log_level_match(severity)) LOG(severity, tag)

#ifdef _DEBUG
// Initial severity for log file
constexpr AixLog::Severity QMM2_LOG_DEFAULT_SEVERITY = AixLog::Severity::debug;
#else
// Initial severity for log file
constexpr AixLog::Severity QMM2_LOG_DEFAULT_SEVERITY = AixLog::Severity::info;
#endif

// Severity to log to game console
constexpr AixLog::Severity QMM2_LOG_CONSOLE_SEVERITY = AixLog::Severity::info;

/**
* @brief Check if a message with given severity should be logged
*
* @param severity Log level to check
* @return true if at least one sink should output a log with the given severity, false otherwise
*/
bool log_level_match(int severity);

/**
* @brief Initialize log file
*
* @param file Filename for log file
* @param severity Initial severity for log file
* @param append true if the log file should be appended-to after opening (not truncated), false otherwise
*/
void log_init(std::string file, AixLog::Severity severity = QMM2_LOG_DEFAULT_SEVERITY, bool append = false);

/**
* @brief Convert severity name to value
*
* @param severity Severity name to convert
* @return Severity value
*/
int log_severity_from_name(std::string severity);

/**
* @brief Convert severity value to name
*
* @param severity Severity value to convert
* @return Severity name
*/
std::string log_name_from_severity(int severity);

/**
* @brief Set severity of log file
*
* @param severity Severity name to set
*/
void log_set_severity(int severity);

/**
* @brief Add new log sink
*
* @param func Function to execute when logging
* @param level Level to log
*/
template <typename T>
void log_add_sink(T func, AixLog::Severity level = AixLog::Severity::notice) {
    AixLog::Log::instance().add_logsink<AixLog::SinkCallback>(level, func);
}

/**
* @brief Format a log message for custom sink
*
* @param metadata Log entry metadata (severity, tag, timestamp, etc)
* @param message Log message
* @param timestamp Should we output timestamp?
* @return Formatted log message
*/
std::string log_format(const AixLog::Metadata& metadata, const std::string& message, bool timestamp = true);

// Class like AixLog's SinkFile except it will not truncate the file on opening
struct SinkFileAppend : public AixLog::SinkFormat
{
    SinkFileAppend(const SinkFileAppend&) = delete;
    SinkFileAppend& operator=(const SinkFileAppend&) = delete;
    SinkFileAppend(const AixLog::Filter& filter, const std::string& filename, const std::string& format = "%Y-%m-%d %H:%M:%S.#ms [#severity] (#tag_func)")
        : SinkFormat(filter, format)
    {
        ofs.open(filename, std::ofstream::out | std::ios_base::app);
    }

    ~SinkFileAppend() override
    {
        ofs.close();
    }

    void log(const AixLog::Metadata& metadata, const std::string& message) override
    {
        do_log(ofs, metadata, message);
    }

protected:
    mutable std::ofstream ofs;
};

#endif // __cplusplus

enum {
    QMM_LOG_TRACE,              // Trace (lowest log level)
    QMM_LOG_DEBUG,              // Debug (above Trace, below Info)
    QMM_LOG_INFO,               // Info (above Debug, below Notice)
    QMM_LOG_NOTICE,             // Notice (above Info, below Warning)
    QMM_LOG_WARNING,            // Warning (above Notice, below Error)
    QMM_LOG_ERROR,              // Error (above Warning, below Fatal)
    QMM_LOG_FATAL               // Fatal (highest log level)
};

#endif // QMM2_LOG_H
