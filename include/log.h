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

// #define LOG_TRACE

void log_init(std::string file);

template <typename T>
void log_add_sink(T func, AixLog::Severity level = AixLog::Severity::notice) {
	AixLog::Log::instance().add_logsink<AixLog::SinkCallback>(level, func);
}

std::string log_format(const AixLog::Metadata& metadata, const std::string& message, bool timestamp = true);

#endif // __QMM2_LOG_H__
