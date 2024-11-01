#ifndef MYTOOLS_LOGCAT
#define MYTOOLS_LOGCAT

#include <sys/types.h>
// #include <sys/ipc.h>
// #include <sys/shm.h>

#include "LString.h"

namespace MyTools
{
	struct LogListInfo;
	struct LogItemInfo;

	class LogCatItem
	{
	private:
		char *_p;
	
	private:
		// constructor for LogCat
		LogCatItem(char *p);
		LogCatItem(char *p, unsigned long sn, int type, const LString &tag, int flags, const LString &str);

	public:
		static int itemSize(unsigned int slen);

		unsigned long sn() const;
		int type() const;
		const char *tag() const;
		int flags() const;
		const char *logText() const;

		int size() const;
		void move(int offset);
		void print() const;

		friend class LogCat;
	};

	class LogCat
	{
	public:
		enum LogType { 
			Information = 1, 
			Debug = 2, 
			Warning = 4, 
			Error = 8 
		};

	private:
		int _lockfd;
		int _shmid;
		char *_m;

		unsigned long _snRead;

	protected:
		LogListInfo *header();
		char *dataArea();
		long long freeSpace();
		void clear();

		void rdlock();
		void wrlock();
		void unlock();

		char *itemAt(unsigned long sn);
		bool offerSize(unsigned int sz);

		char *addItem(LogType type, const LString &tag, int flags, const LString &str);

	public:
		LogCat();
		virtual ~LogCat();

		static LogCat *self();

		void sync(int typeMask = 0xff);

		void i(const LString &tag, const LString &str);
		void d(const LString &tag, const LString &str);
		void w(const LString &tag, const LString &str);
		void e(const LString &tag, const LString &str);
	};
};

#define LOG (*MyTools::LogCat::self())

#endif
