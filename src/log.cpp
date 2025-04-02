/*
QMM2 - Q3 MultiMod 2
Copyright 2025
https://github.com/thecybermind/qmm2/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
	Kevin Masterson < cybermind@gmail.com >

*/

#include <string>
#include "aixlog/aixlog.hpp"
#include "log.h"

void log_init(std::string file) {
	auto sink_cout = std::make_shared<AixLog::SinkCout>(AixLog::Severity::info);
	auto sink_file = std::make_shared<AixLog::SinkFile>(
#ifdef _DEBUG
 #ifdef LOG_TRACE
		AixLog::Severity::trace
 #else
		AixLog::Severity::debug
 #endif
#else
		AixLog::Severity::info
#endif
		, file);
	AixLog::Log::init({ sink_cout, sink_file });
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
