#include "Log.h"

namespace MyTools
{
	class StdoutLogger : public Logger
	{
	public:
		void debug(const char *tag, const char *msg);
		void warn(const char *tag, const char *msg);
		void error(const char *tag, const char *msg);
	};

	// StdouLogger members

	void StdoutLogger::debug(const char *tag, const char *msg)
	{ printf("D/%s: %s\r\n", tag, msg); }

	void StdoutLogger::warn(const char *tag, const char *msg)
	{ printf("W/%s: %s\r\n", tag, msg); }

	void StdoutLogger::error(const char *tag, const char *msg)
	{ printf("E/%s: %s\r\n", tag, msg); }
};

using namespace MyTools;

Logger *Log::_logger = new StdoutLogger;

Logger *Log::setLogger(Logger *logger)
{
	Logger *ret = _logger;
	_logger = logger;
	return ret;
}

void Log::d(const String &tag, const String &str)
{
	_logger->debug(tag.toUtf8(), str.toUtf8());
}

void Log::w(const String &tag, const String &str)
{
	_logger->warn(tag.toUtf8(), str.toUtf8());
}

void Log::e(const String &tag, const String &str)
{
	_logger->error(tag.toUtf8(), str.toUtf8());
}
