/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#include <string>
#include "log.h"
#include "format.h"

static AixLog::log_sink_ptr s_log_sink_file = nullptr;


void log_init(std::string file, AixLog::Severity severity, bool append) {
    if (append)
        s_log_sink_file = std::make_shared<SinkFileAppend>(severity, file);
    else
        s_log_sink_file = std::make_shared<AixLog::SinkFile>(severity, file);

    AixLog::Log::init({ s_log_sink_file });
}


AixLog::Severity log_severity_from_name(std::string severity) {
    return AixLog::to_severity(severity, QMM2_LOG_DEFAULT_SEVERITY);
}


std::string log_name_from_severity(AixLog::Severity severity) {
    return AixLog::to_string(severity);
}


void log_set_severity(AixLog::Severity severity) {
    (*s_log_sink_file).filter.add_filter(severity);
}


#if 0
// actually in log.h, just here for visibility
template <typename T>
void log_add_sink(T func, AixLog::Severity level = AixLog::Severity::notice) {
    AixLog::Log::instance().add_logsink<AixLog::SinkCallback>(level, func);
}
#endif


std::string log_format(const AixLog::Metadata& metadata, const std::string& message, bool timestamp) {
    std::string output = fmt::format("({}) {}\n", metadata.tag.text, message);
    if ((int)metadata.severity >= 5)
        output = fmt::format("[{}] {}", AixLog::to_string(metadata.severity), output);
    if (timestamp && metadata.timestamp)
        output = fmt::format("{} {}", metadata.timestamp.to_string(), output);
    return output;
}
