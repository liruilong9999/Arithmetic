#ifndef MYTOOLS_LOG_H
#define MYTOOLS_LOG_H

#include <stdio.h>
#include "tools_gloabal.h"

namespace MyTools
{
class LString;
	class TOOLS_EXPORT Logger
	{
	protected:
		virtual void debug(const char *tag, const char *msg) = 0;
		virtual void warn(const char *tag, const char *msg) = 0;
		virtual void error(const char *tag, const char *msg) = 0;

		friend class Log;
	};

	class TOOLS_EXPORT Log
	{
	private:
		static Logger *_logger;

	public:
		static Logger *setLogger(Logger *);

		static void d(const LString &tag, const LString &msg);
		static void w(const LString &tag, const LString &msg);
		static void e(const LString &tag, const LString &msg);
	};
};

#if defined(LOG_LEVEL_ALL)
	#define LOG_D(tag, msg) Log::d(tag, msg)
	#define LOG_W(tag, msg) Log::w(tag, msg)
	#define LOG_E(tag, msg) Log::e(tag, msg)
#elif defined(LOG_LEVEL_LESS)
	#define LOG_D(tag, msg) 
	#define LOG_W(tag, msg) Log::w(tag, msg)
	#define LOG_E(tag, msg) Log::e(tag, msg)
#elif defined(LOG_LEVEL_CRITICAL)
	#define LOG_D(tag, msg) 
	#define LOG_W(tag, msg) 
	#define LOG_E(tag, msg) Log::e(tag, msg)
#else
	#define LOG_D(tag, msg) 
	#define LOG_W(tag, msg) 
	#define LOG_E(tag, msg) 
#endif

#endif
