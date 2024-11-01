#include <stdio.h>
//#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
// #include <unistd.h>
// #include <sys/file.h>
#include <assert.h>
#include <memory>
#include <cstring> // C++ ·ç¸ñ

#include "logcat.h"

static const char *g_logLock = "/tmp/dlogcat.lck";

const unsigned long SPACE_SIZE = 4096;

namespace MyTools
{
	struct LogListInfo
	{
		unsigned long snBegin;
		unsigned long snEnd;
		char *head;
		char *tail;
		char *endSpace;
	};

	struct LogItemInfo
	{
		unsigned long sn;
		char type;
		char tag[32];
		short flags;
		short logLength;
	};

	static LogCat *g_logCat = NULL;
};

using namespace MyTools;

// LogCatItem members

LogCatItem::LogCatItem(char *p)
	: _p(p)
{
}

LogCatItem::LogCatItem(char *p, unsigned long sn, int type, const String &tag, int flags, const String &str)
	: _p(p)
{
	assert(str != NULL);

	LogItemInfo *h = reinterpret_cast<LogItemInfo *>(p);
	h->sn = sn;
	h->type = type;
	memset(h->tag, 0, 32);
	if (!tag.isEmpty()) strncpy(h->tag, tag.toUtf8(), 31);
	h->flags = flags;
	int lstr = str.length();
	h->logLength = lstr;
	memcpy(p + sizeof(LogItemInfo), str.toUtf8(), lstr);
	*(p + sizeof(LogItemInfo) + lstr) = '\0';
}

int LogCatItem::itemSize(unsigned int slen)
{
	return sizeof(LogItemInfo) + slen + 1;
}

unsigned long LogCatItem::sn() const
{
	return reinterpret_cast<LogItemInfo *>(_p)->sn;
}

int LogCatItem::type() const
{
	return reinterpret_cast<LogItemInfo *>(_p)->type;
}

const char *LogCatItem::tag() const
{
	return reinterpret_cast<LogItemInfo *>(_p)->tag;
}

int LogCatItem::flags() const
{
	return reinterpret_cast<LogItemInfo *>(_p)->flags;
}

const char *LogCatItem::logText() const
{
	return reinterpret_cast<const char *>(_p) + sizeof(LogItemInfo);
}

int LogCatItem::size() const
{
	return sizeof(LogItemInfo) + reinterpret_cast<LogItemInfo *>(_p)->logLength + 1;
}

void LogCatItem::move(int offset)
{
	int n = size();

	if (offset < 0) {
		const char *src = _p;
		char *dst = const_cast<char *>(src) + offset;
		while (n--)
			*dst++ = *src++;
	}
	else if (offset > 0) {
		const char *src = _p + n - 1;
		char *dst = const_cast<char *>(src) + offset;
		while (n--)
			*dst-- = *src--;
	}

	_p += offset;
}

void LogCatItem::print() const
{
	char tc;
	switch (type()) {
	case LogCat::Information:
		tc = 'I';
		break;
	case LogCat::Debug:
		tc = 'D';
		break;
	case LogCat::Warning:
		tc = 'W';
		break;
	case LogCat::Error:
		tc = 'E';
		break;
	default:
		assert(0);
		break;
	}

	if (strlen(tag()) == 0)
		printf("%c| %s\r\n", tc, logText());
	else
		printf("%c|%s: %s\r\n", tc, tag(), logText());
}

// LogCat members

inline LogListInfo *LogCat::header()
{ 
	return reinterpret_cast<LogListInfo *>(_m); 
}

inline char *LogCat::dataArea()
{
	return _m + sizeof(LogListInfo);
}

long long LogCat::freeSpace()
{
	LogListInfo *h = header();
	if (h->tail >= h->head)
		return h->endSpace - h->tail;
	else
		return h->head - h->tail;
}

void LogCat::clear()
{
	LogListInfo *h = header();
	h->snBegin = 0;
	h->snEnd = 0;
	h->head = h->tail = dataArea();
	h->endSpace = _m + SPACE_SIZE;
}

void LogCat::rdlock()
{
	/*
	struct flock lck;
	lck.l_type = F_RDLCK;
	lck.l_start = 0;
	lck.l_whence = SEEK_SET;
	lck.l_len = 0;

	for (;;) {
		int ret = fcntl(_lockfd, F_SETLKW, &lck);
		if (ret == 0)
			break;
		else if (errno != EINTR) {
			perror("fcntl()");
			exit(1);
		}
	}
	*/
#if 0 //TODO

	for (;;) {
		int ret = flock(_lockfd, LOCK_SH);
		if (ret == 0)
			break;
		else if (errno != EINTR) {
			perror("flock()");
			exit(1);
		}
	}

#endif
}

void LogCat::wrlock()
{
	/*
	struct flock lck;
	lck.l_type = F_WRLCK;
	lck.l_start = 0;
	lck.l_whence = SEEK_SET;
	lck.l_len = 0;

	for (;;) {
		int ret = fcntl(_lockfd, F_SETLKW, &lck);
		if (ret == 0)
			break;
		else if (errno != EINTR) {
			perror("fcntl()");
			exit(1);
		}
	}
	*/
#if 0 //TODO
	for (;;) {
		int ret = flock(_lockfd, LOCK_EX);
		if (ret == 0)
			break;
		else if (errno != EINTR) {
			perror("flock()");
			exit(1);
		}
	}
#endif
}

void LogCat::unlock()
{
	/*
	struct flock lck;
	lck.l_type = F_UNLCK;
	lck.l_start = 0;
	lck.l_whence = SEEK_SET;
	lck.l_len = 0;

	if (fcntl(_lockfd, F_SETLK, &lck) == -1) {
		perror("fcntl()");
		exit(1);
	}
	*/
#if 0 //TODO

	if (flock(_lockfd, LOCK_UN) == -1) {
		perror("flock()");
		exit(1);
	}

#endif
}

LogCat::LogCat()
{
#if 0 //TODO
	bool initialized = false;

	_snRead = 0;

	_lockfd = open(g_logLock, O_CREAT | O_RDWR, 0666);
	if (_lockfd == -1) {
		perror("open()");
		exit(1);
	}

	key_t key = ftok(g_logLock, 'L');
	if (key == -1) {
		perror("ftok()");
		exit(1);
	}

	_shmid = shmget(key, SPACE_SIZE, IPC_CREAT | IPC_EXCL | 0666);
	if (_shmid == -1 && errno == EEXIST) {
		initialized = true;
		_shmid = shmget(key, SPACE_SIZE, 0666);
	}
	if (_shmid == -1) {
		perror("shmget()");
		exit(1);
	}

	_m = reinterpret_cast<char *>(shmat(_shmid, NULL, 0));
	if (_m == reinterpret_cast<char *>(-1)) {
		perror("shmat()");
		exit(1);
	}

	if (!initialized) {
		wrlock();
		clear();
		unlock();
	}

#endif
}

LogCat::~LogCat()
{
#if 0 //TODO

	if (_shmid != -1)
		shmctl(_shmid, IPC_RMID, 0);

	if (_lockfd != -1)
		close(_lockfd);

#endif
}

char *LogCat::itemAt(unsigned long sn)
{
	assert(sn != 0);

	LogListInfo *h = header();
	if (sn < h->snBegin || sn > h->snEnd)
		return NULL;

	char *p = h->head;
	for (;;) {
		LogCatItem item(p);
		if (item.sn() == sn)
			return p;
		p += item.size();
		if (p == h->tail)
			return NULL;
		if (p == h->endSpace)
			p = dataArea();
	}

	return NULL;
}

bool LogCat::offerSize(unsigned int sz)
{
	LogListInfo *h = header();
	if (freeSpace() >= sz)
		return true;
	else if (h->tail == h->head) // no more space to give
		return false;

	if (h->tail > h->head) {
		h->endSpace = h->tail;
		h->tail = dataArea();
	}

	while (freeSpace() < sz) {
		h->head += LogCatItem(h->head).size();
		if (h->head == h->endSpace) {
			h->head = dataArea();
			h->endSpace = _m + SPACE_SIZE;
		}
		if (h->head == h->tail) {
			clear();
			break;
		}
		else
			h->snBegin = LogCatItem(h->head).sn();
	}

	return freeSpace() >= sz;
}

char *LogCat::addItem(LogCat::LogType type, const String &tag, int flags, const String &str)
{
	wrlock();

	if (!offerSize(LogCatItem::itemSize(str.length())))
		return NULL;

	unsigned long sn = header()->snEnd + 1;
	if (sn == 0)
		sn = 1;
	LogCatItem item(header()->tail, sn, type, tag, flags, str);
	header()->snEnd = sn;
	header()->tail += item.size();
	char *p = header()->tail;

	unlock();
	return p;
}

LogCat *LogCat::self()
{
	if (g_logCat == NULL)
		g_logCat = new LogCat();
	return g_logCat;
}

void LogCat::sync(int typeMask)
{
	rdlock();

	LogListInfo *h = header();
	if (_snRead < h->snBegin || _snRead > h->snEnd)
		_snRead = h->snBegin;
	if (_snRead == 0 || _snRead == h->snEnd)
		return;

	char *p = itemAt(_snRead + 1);
	assert(p != NULL);

	for (;;) {
		LogCatItem item(p);
		if (item.type() & typeMask)
			item.print();
		_snRead = item.sn();
		p += item.size();
		if (p == h->head)
			return;
		if (p == h->endSpace)
			p = dataArea();
	}

	unlock();
}

void LogCat::i(const String &tag, const String &str)
{
	addItem(LogCat::Information, tag, 0, str);
}

void LogCat::d(const String &tag, const String &str)
{
	addItem(LogCat::Debug, tag, 0, str);
}

void LogCat::w(const String &tag, const String &str)
{
	addItem(LogCat::Warning, tag, 0, str);
}

void LogCat::e(const String &tag, const String &str)
{
	addItem(LogCat::Error, tag, 0, str);
}

