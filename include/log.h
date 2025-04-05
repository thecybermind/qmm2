/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
	Kevin Masterson < cybermind@gmail.com >

*/

#ifndef __QMM2_LOG_H__
#define __QMM2_LOG_H__

#include <aixlog/aixlog.hpp>
#include "format.h"

#ifdef _DEBUG
#define QMM2_LOG_DEFAULT_SEVERITY AixLog::Severity::debug
#else
#define QMM2_LOG_DEFAULT_SEVERITY AixLog::Severity::info
#endif


void log_init(std::string file, AixLog::Severity severity = QMM2_LOG_DEFAULT_SEVERITY);

AixLog::Severity log_severity_from_name(std::string severity);
std::string log_name_from_severity(AixLog::Severity severity);
void log_set_severity(AixLog::Severity severity);

template <typename T>
void log_add_sink(T func, AixLog::Severity level = AixLog::Severity::notice) {
	AixLog::Log::instance().add_logsink<AixLog::SinkCallback>(level, func);
}

std::string log_format(const AixLog::Metadata& metadata, const std::string& message, bool timestamp = true);

#endif // __QMM2_LOG_H__
