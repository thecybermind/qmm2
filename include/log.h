/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#ifndef __QMM2_LOG_H__
#define __QMM2_LOG_H__

// #define QMM_LOG_APPEND

#include <aixlog/aixlog.hpp>
#include <string>
#include "format.h"

#ifdef _DEBUG
#define QMM2_LOG_DEFAULT_SEVERITY AixLog::Severity::debug
#else
#define QMM2_LOG_DEFAULT_SEVERITY AixLog::Severity::info
#endif


void log_init(std::string file, AixLog::Severity severity = QMM2_LOG_DEFAULT_SEVERITY, bool append = false);

AixLog::Severity log_severity_from_name(std::string severity);
std::string log_name_from_severity(AixLog::Severity severity);
void log_set_severity(AixLog::Severity severity);

template <typename T>
void log_add_sink(T func, AixLog::Severity level = AixLog::Severity::notice) {
    AixLog::Log::instance().add_logsink<AixLog::SinkCallback>(level, func);
}

std::string log_format(const AixLog::Metadata& metadata, const std::string& message, bool timestamp = true);

enum {
    QMM_LOG_TRACE,
    QMM_LOG_DEBUG,
    QMM_LOG_INFO,
    QMM_LOG_NOTICE,
    QMM_LOG_WARNING,
    QMM_LOG_ERROR,
    QMM_LOG_FATAL
};

// class like AixLog's SinkFile except it will not truncate the file on opening
struct SinkFileAppend : public AixLog::SinkFormat
{
    SinkFileAppend(const AixLog::Filter& filter, const std::string& filename, const std::string& format = "%Y-%m-%d %H:%M:%S.#ms [#severity] (#tag_func)")
        : SinkFormat(filter, format)
    {
        ofs.open(filename.c_str(), std::ofstream::out | std::ios_base::app);
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

#endif // __QMM2_LOG_H__
