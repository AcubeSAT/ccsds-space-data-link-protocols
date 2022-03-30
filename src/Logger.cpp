#include <Logger.hpp>

etl::format_spec Logger::format;

Logger::LogEntry::LogEntry(LogLevel level) : level(level) {}

Logger::LogEntry::~LogEntry() {
	// When the destructor is called, the log message is fully "designed". Now we can finally "display" it to the user.
	Logger::log(level, message);
}
