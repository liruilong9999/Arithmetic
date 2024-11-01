#include <stdio.h>
#include <stdarg.h>
//#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <assert.h>

#include <memory>
#include <cstring>

#include "debug_alloc.h"
#include "LString.h"

typedef unsigned int uint;
#define REGISTER register

#define MIN(a, b) ((a) < (b) ? (a) : (b))

using namespace MyTools;

static const char *g_EmptyString = "";
static const char *g_NullString = "(null)";

static int ucs2_to_utf8(WChar wc, char *buf)
{
	char *pbuf = buf;

	if (wc < 0x0080) {
		// 1 byte UTF-8 Character
		*pbuf++ = static_cast<char>(wc);
	}
	else if (wc < 0x0800) {
		// 2 bytes UTF-8 Character
		*pbuf++ = ((wc >> 6) & 0xff) | 0xc0;
		*pbuf++ = (wc & 0x003f) | 0x80;
	}
	else {
		// 3 bytes UTF-8 Character
		*pbuf++ = ((wc >> 12) & 0xff) | 0xe0;
		*pbuf++ = (((wc & 0x0fc0) >> 6) & 0xff) | 0x80;
		*pbuf++ = (wc & 0x003f) | 0x80;
	}

	return pbuf - buf;
}

static int utf8_to_ucs2(const char *buf, WChar *wc)
{
	const char *pbuf = buf;

	if ((*pbuf & 0x80) == 0) {
		// 1 byte UTF-8 character
		*wc = *pbuf++;
	}
	else if ((*pbuf & 0xe0) == 0xc0
			 && (*(pbuf + 1) & 0xc0) == 0x80) {
		// 2 bytes UTF-8 character
		WChar n1 = static_cast<WChar>(*pbuf++ & 0x1f) << 6;
		WChar n2 = static_cast<WChar>(*pbuf++ & 0x3f);
		*wc = n1 | n2;
	}
	else if ((*pbuf & 0xf0) == 0xe0
			 && (*(pbuf + 1) & 0xc0) == 0x80
			 && (*(pbuf + 2) & 0xc0) == 0x80) {
		// 3 bytes UTF-8 character
		WChar n1 = static_cast<WChar>(*pbuf++ & 0x0f) << 12;
		WChar n2 = static_cast<WChar>(*pbuf++ & 0x3f) << 6;
		WChar n3 = static_cast<WChar>(*pbuf++ & 0x3f);
		*wc = n1 | n2 | n3;
	}

	return pbuf - buf;
}

static int ucstrcmp(const LString &as, const LString &bs)
{
	const WChar *a = as.unicode();
	const WChar *b = bs.unicode();
	if (a == b)
		return 0;
	if (a == NULL)
		return 1;
	if (b == NULL)
		return -1;

	int l = MIN(as.length(), bs.length());
	while (l-- != 0 && *a == *b) {
		a++;
		b++;
	}
	if (l == -1)
		return (as.length() - bs.length());
	else
		return *a - *b;
}

static int ucstrncmp(const WChar *a, const WChar *b, int l)
{
	while (l-- != 0 && *a == *b) {
		++a;
		++b;
	}

	if (l == -1)
		return 0;
	else
		return *a - *b;
}

static int ucstrnicmp(const WChar *a, const WChar *b, int l)
{
	while (l-- != 0 && tolower(*a) == tolower(*b)) {
		++a;
		++b;
	}

	if (l == -1)
		return 0;
	else
		return tolower(*a) - tolower(*b);
}

inline static bool wc_is_space(WChar wc)
{
	return (wc > 0 && wc < 128 && isspace(wc));
}

inline static bool wc_is_digit(WChar wc)
{
	return (wc >= 48 && wc <= 57);
}

inline static int wc_digit_value(WChar wc)
{
	return wc - 48;
}

static bool wc_ok_in_base(WChar wc, int base)
{
	if (base <= 10)
		return wc_is_digit(wc) && wc_digit_value(wc) < base;
	else
		return wc_is_digit(wc) 
				|| (wc >= 'a' && wc < char('a' + base - 10))
				|| (wc >= 'A' && wc < char('A' + base - 10));
}

// LString members

const LString LString::null;

inline int LString::ref() const
{
	LString *self = const_cast<LString *>(this);
	return (*self->_d)++;
}

inline int LString::unref() const
{
	LString *self = const_cast<LString *>(this);
	return --(*self->_d);
}

inline WChar *LString::data()
{ return _d + 3; }

inline const WChar *LString::data() const
{ return _d + 3; }

inline unsigned short LString::len() const
{ return _d[1]; }

inline unsigned short &LString::len_r()
{ return _d[1]; }

inline void LString::init()
{
	_d = NULL;
	_buf_latin1 = NULL;
	_buf_utf8 = NULL;
}

void LString::destroy()
{
	if (_d != NULL && unref() == 0)
		DELETE_ARR(_d);
	_d = NULL; // whatever

	if (_buf_latin1 != NULL) {
		DELETE_ARR(_buf_latin1);
		_buf_latin1 = NULL;
	}

	if (_buf_utf8 != NULL) {
		DELETE_ARR(_buf_utf8);
		_buf_utf8 = NULL;
	}
}

void LString::clone()
{
	if (_d != NULL && _d[0] != 1) {
		unref();
		const uint nchars = len();
		WChar *newBuf = NEW WChar[nchars + 3];
		if (newBuf != NULL) {
			memcpy(newBuf + 3, _d + 3, sizeof(WChar) * nchars);
			newBuf[0] = 1;
			newBuf[1] = nchars;
			newBuf[2] = nchars;
		}
		_d = newBuf;
	}

	if (_buf_latin1 != NULL) {
		DELETE_ARR(_buf_latin1);
		_buf_latin1 = NULL;
	}

	if (_buf_utf8 != NULL) {
		DELETE_ARR(_buf_utf8);
		_buf_utf8 = NULL;
	}
}

bool LString::assureSize(uint asz)
{
	assert(_d == NULL || _d[0] == 1);

	if (_d != NULL && _d[2] >= asz)
		return true;

	const uint nchars = (_d == NULL ? 0 : _d[1]);
	uint newbufsz = (_d == NULL ? 0 : _d[2]);
	int i = 1;
	while (newbufsz < asz)
		newbufsz += 30 * i++;

	WChar *newbuf = NEW WChar[newbufsz + 3];
	if (newbuf == NULL)
		return false;
	newbuf[0] = 1;
	newbuf[1] = nchars;
	newbuf[2] = newbufsz;

	if (_d != NULL) {
		memcpy(newbuf + 3, _d + 3, sizeof(WChar) * nchars);
		DELETE_ARR(_d);
	}

	_d = newbuf;

	return true;
}

void LString::blkmov(int from, int offset)
{
	if (offset > 0) {
		REGISTER WChar *p = data() + len() + offset - 1;
		for (int n = len() - from; n > 0; --n, --p)
			*p = *(p - offset);
	}
	else if (offset < 0) {
		REGISTER WChar *p = data() + from;
		for (int n = len() - from; n > 0; --n, ++p)
			*(p + offset) = *p;
	}
}

LString::LString()
{
	init();
}

LString::LString(char c)
{
	init();

	char sbuf[2] = { c, 0 };
	setLatin1(sbuf, 1);
}

LString::LString(const char *str)
{
	init();
	setLatin1(str);
}

LString::LString(const unsigned short *unicodeAsUShorts, uint len)
{
	init();
	setUnicode(unicodeAsUShorts, len);
}

LString::LString(const LString &s)
	: _d(s._d), _buf_latin1(NULL), _buf_utf8(NULL)
{
	if (_d != NULL)
		ref();
}

LString::~LString()
{
	destroy();
}

LString &LString::operator=(char c)
{
	char sbuf[2] = { c, 0 };
	setLatin1(sbuf, 1);
	return *this;
}

LString &LString::operator=(const char *str)
{
	setLatin1(str);
	return *this;
}

LString &LString::operator=(const LString &s)
{
	if (s._d != NULL)
		s.ref();
	destroy();
	_d = s._d;
	return *this;
}

void LString::clear()
{
	if (_d != NULL) {
		if (_d[0] == 1) // your own only
			len_r() = 0;
		else {
			unref();
			_d = NULL;
		}
	}

	if (_buf_latin1 != NULL) {
		DELETE_ARR(_buf_latin1);
		_buf_latin1 = NULL;
	}

	if (_buf_utf8 != NULL) {
		DELETE_ARR(_buf_utf8);
		_buf_utf8 = NULL;
	}
}

void LString::truncate(uint newLen)
{
	if (newLen >= length())
		return;
	
	WChar *newBuf = NEW WChar[newLen + 3];
	if (newBuf != NULL) {
		memcpy(newBuf + 3, _d + 3, sizeof(WChar) * newLen);
		newBuf[0] = 1;
		newBuf[1] = newLen;
		newBuf[2] = newLen;
	}

	destroy();
	_d = newBuf;
}

/*
LString LString::stripWhiteSpace() const
{
	LString res;

	REGISTER const WChar *p = unicode();
	int start = 0;
	int end = length() - 1;
	while (start <= end && wc_is_space(p[start]))
		++start;
	if (start > end) // only white space
		return res;
	while (end > 0 && wc_is_space(p[end]))
		--end;

	int len = end - start + 1;
	if (res.assureSize(len)) {
		memcpy(res._d + 3, _d + 3 + start, sizeof(WChar) * len);
		res.len_r() = len;
	}

	return res;
}
*/

LString LString::simplified() const
{
	int n = static_cast<int>(length());
	if (n == 0)
		return LString::null;

	LString res;
	if (!res.assureSize(n))
		return LString::null;

	REGISTER const WChar *uthis = data();
	REGISTER WChar *ures = res.data();

	// skip the beginning space
	while (n > 0 && wc_is_space(*uthis)) {
		++uthis;
		--n;
	}

	while (n > 0) {
		if (!wc_is_space(*uthis))
			*ures++ = *uthis;
		else if (*(ures - 1) != ' ')
			*ures++ = ' ';
		++uthis;
		--n;
	}

	if (ures > res.data() && *(ures - 1) == ' ')
		--ures;

	res.len_r() = ures - res.data();
	return res;
}

LString LString::toLower() const
{
	uint n = length();
	LString res(*this);
	res.clone();
	if (!res.isEmpty()) {
		REGISTER WChar *ures = res.data();
		while (n-- != 0) {
			WChar wch = *ures & 0xff;
			*ures++ = tolower(wch);
		}
	}
	return res;
}

LString LString::toUpper() const
{
	uint n = length();
	LString res(*this);
	res.clone();
	if (!res.isEmpty()) {
		REGISTER WChar *ures = res.data();
		while (n-- != 0) {
			WChar wch = *ures & 0xff;
			*ures++ = toupper(wch);
		}
	}
	return res;
}

LString &LString::sprintf(const char *cformat, ...)
{
	assert(cformat != NULL);

	char *sbuf = NULL;
	int nchars = 0;
	int bufsz = 100;
	va_list ap;

	if ((sbuf = static_cast<char *>(MALLOC(bufsz))) == NULL)
		return *this;

	for (;;) {
		va_start(ap, cformat);
		nchars = vsnprintf(sbuf, bufsz, cformat, ap);
		va_end(ap);
		if (nchars >= 0 && nchars < bufsz)
			break;
		if (nchars >= 0)
			bufsz = nchars + 1;
		else
			bufsz *= 2;
		char *tmpp = static_cast<char *>(REALLOC(sbuf, bufsz));
		if (tmpp == NULL) {
			FREE(sbuf);
			return *this;
		}
		else
			sbuf = tmpp;
	}

	setLatin1(sbuf, nchars);
	FREE(sbuf);
	return *this;
}

LString LString::format(const char *cformat, ...)
{
	char *sbuf = NULL;
	int nchars = 0;
	int bufsz = 100;
	va_list ap;

	if ((sbuf = static_cast<char *>(MALLOC(bufsz))) == NULL)
		return LString::null;

	for (;;) {
		va_start(ap, cformat);
		nchars = vsnprintf(sbuf, bufsz, cformat, ap);
		va_end(ap);
		if (nchars > -1 && nchars < bufsz)
			break;
		if (nchars > -1)
			bufsz = nchars + 1;
		else
			bufsz *= 2;
		char *tmpp = static_cast<char *>(REALLOC(sbuf, bufsz));
		if (tmpp == NULL) {
			FREE(sbuf);
			return LString::null;
		}
		else
			sbuf = tmpp;
	}

	LString res(sbuf);
	FREE(sbuf);
	return res;
}

int LString::indexOf(char c, int from, bool caseSensitive) const
{
	const int lthis = static_cast<int>(length());
	if (from < 0)
		from += lthis;
	if (from < 0 || from >= lthis)
		return -1;

	WChar wch = (c & 0xff);
	REGISTER const WChar *uthis = data() + from;
	int i = from;
	if (caseSensitive) {
		for (; i < lthis && *uthis != wch; ++i, ++uthis)
			;
	}
	else {
		wch = tolower(wch);
		for (; i < lthis && tolower(*uthis) != wch; ++i, ++uthis)
			;
	}
	return i < lthis ? i : -1;
}

int LString::indexOf(const LString &str, int from, bool caseSensitive) const
{
	const int lthis = static_cast<int>(length());
	if (from < 0)
		from += lthis;
	if (from < 0 || from >= lthis)
		return -1;

	const int lstr = static_cast<int>(str.length());
	if (lstr == 0)
		return -1;
	const int delta = lthis - from - lstr;
	if (delta < 0)
		return -1;

	const WChar *uthis = data() + from;
	const WChar *ustr = str.data();
	uint hthis = 0;
	uint hstr = 0;
	int i = 0;
	if (caseSensitive) {
		for (i = 0; i < lstr; ++i) {
			hthis += uthis[i];
			hstr += ustr[i];
		}
		i = 0;
		for (;;) {
			if (hthis == hstr && ucstrncmp(uthis + i, ustr, lstr) == 0)
				return from + i;
			if (i == delta)
				return -1;
			hthis += uthis[i + lstr];
			hthis -= uthis[i];
			++i;
		}
	}
	else {
		for (i = 0; i < lstr; ++i) {
			hthis += tolower(uthis[i]);
			ustr += tolower(ustr[i]);
		}
		i = 0;
		for (;;) {
			if (hthis == hstr && ucstrnicmp(uthis + i, ustr, lstr) == 0)
				return from + i;
			if (i == delta)
				return -1;
			hthis += tolower(uthis[i + lstr]);
			hthis -= tolower(uthis[i]);
			++i;
		}
	}

	return -1;
}

int LString::lastIndexOf(char c, int from, bool caseSensitive) const
{
	const int lthis = static_cast<int>(length());
	if (from < 0)
		from += lthis;
	if (from < 0 || from >= lthis)
		return -1;

	WChar wch = c & 0xff;
	REGISTER const WChar *uthis = data() + from;
	int i = from;
	if (caseSensitive) {
		for (; i != -1 && *uthis != wch; --i, --uthis)
			;
	}
	else {
		wch = tolower(wch);
		for (; i != -1 && tolower(*uthis) != wch; --i, --uthis)
			;
	}

	return i;
}

int LString::lastIndexOf(const LString &str, int from, bool caseSensitive) const
{
	const int lstr = static_cast<int>(str.length());
	if (lstr == 0)
		return -1;

	const int lthis = static_cast<int>(length());
	if (from < 0)
		from += lthis;
	if (from < lstr - 1 || from >= lthis)
		return -1;

	const WChar *uthis = NULL;
	const WChar *ustr = str.data();
	uint hthis = 0;
	uint hstr = 0;
	int i;
	if (caseSensitive) {
		uthis = data() + from - lstr + 1;
		i = 0;
		for (; i < lstr; ++i) {
			hthis += uthis[i];
			hstr += ustr[i];
		}
		uthis = data();
		i = from - lstr + 1;
		for (;;) {
			if (hthis == hstr && ucstrncmp(uthis + i, ustr, lstr) == 0)
				return i;
			if (i == 0)
				return -1;
			--i;
			hthis += uthis[i];
			hthis -= uthis[i + lstr];
		}
	}
	else {
		uthis = data() + from - lstr + 1;
		i = 0;
		for (; i < lstr; ++i) {
			hthis += tolower(uthis[i]);
			hstr += tolower(ustr[i]);
		}
		uthis = data();
		i = from - lstr + 1;
		for (;;) {
			if (hthis == hstr && ucstrnicmp(uthis + i, ustr, lstr) == 0)
				return i;
			if (i == 0)
				return -1;
			--i;
			hthis += tolower(uthis[i]);
			hthis -= tolower(uthis[i + lstr]);
		}
	}

	return -1;
}

int LString::findCharOf(const char *chset) const
{
	assert(chset != NULL);

	uint n = length();
	REGISTER const WChar *uthis = data();
	for (; n > 0; --n, ++uthis) {
		const char *p = chset;
		while (*p != '\0' && *uthis != (*p & 0xff))
			++p;
		if (*p != '\0')
			return uthis - data();
	}

	return -1;
}

LString LString::left(uint len) const
{
	const uint lthis = length();
	if (lthis == 0 || len == 0)
		return LString::null;
	else if (len >= lthis)
		return *this;

	LString s;
	if (s.assureSize(len)) {
		memcpy(s.data(), data(), sizeof(WChar) * len);
		s.len_r() = len;
	}
	return s;
}

LString LString::right(uint len) const
{
	const uint lthis = length();
	if (lthis == 0 || len == 0)
		return LString::null;
	else if (len >= lthis)
		return *this;

	LString s;
	if (s.assureSize(len)) {
		memcpy(s.data(), data() + lthis - len, sizeof(WChar) * len);
		s.len_r() = len;
	}
	return s;
}

LString LString::mid(uint index, uint len) const
{
	const uint lthis = length();
	if (index >= lthis || len == 0)
		return LString::null;

	if (len > lthis - index)
		len = lthis - index;
	if (index == 0 && len == lthis)
		return *this;

	LString s;
	if (s.assureSize(len)) {
		memcpy(s.data(), data() + index, sizeof(WChar) * len);
		s.len_r() = len;
	}
	return s;
}

LString *LString::split(const LString &sep, int *n) const
{
	*n = 0;
	int bufsz = 8;
	int count = 0;
	LString *sl = NEW_OBJARR LString[bufsz];
	if (sl == NULL)
		return NULL;

	int pos1 = 0;
	int pos2;
	while ((pos2 = indexOf(sep, pos1)) != -1) {
		sl[count++] = mid(pos1, pos2 - pos1);
		pos1 = pos2 + sep.length();

		if (count == bufsz) {
			bufsz *= 2;
			LString *bigbuf = NEW_OBJARR LString[bufsz];
			if (bigbuf == NULL) {
				DELETE_OBJARR(sl);
				return NULL;
			}
			for (int i = 0; i < count; ++i)
				bigbuf[i] = sl[i];
			DELETE_OBJARR(sl);
			sl = bigbuf;
		}
	}

	if (pos1 < static_cast<int>(length()))
		sl[count++] = mid(pos1);

	*n = count;
	return sl;
}

LString &LString::appendUcs2Code(WChar wc)
{
	clone();

	if (assureSize(length() + 1)) {
		*(data() + len()) = wc;
		++len_r();
	}
	return *this;
}

LString &LString::append(const char *str)
{
	clone();

	const int lstr = strlen(str);
	if (assureSize(length() + lstr)) {
		REGISTER const char *pcs = str;
		REGISTER WChar *uthis = data() + len();
		for (int i = 0; i < lstr; ++i)
			*uthis++ = (*pcs++ & 0xff);
		len_r() += lstr;
	}
	return *this;
}

LString &LString::append(const LString &str)
{
	clone();

	const int lstr = static_cast<int>(str.length());
	if (assureSize(length() + lstr)) {
		REGISTER const WChar *ustr = str.data();
		REGISTER WChar *uthis = data() + len();
		for (int i = 0; i < lstr; ++i)
			*uthis++ = *ustr++;
		len_r() += lstr;
	}
	return *this;
}

LString &LString::prepend(const char *str)
{
	clone();

	const int lstr = strlen(str);
	if (assureSize(length() + lstr)) {
		blkmov(0, lstr);
		REGISTER const char *pcs = str;
		REGISTER WChar *uthis = data();
		for (int i = 0; i < lstr; ++i)
			*uthis++ = (*pcs++ & 0xff);
		len_r() += lstr;
	}
	return *this;
}

LString &LString::prepend(const LString &str)
{
	clone();

	const int lstr = static_cast<int>(str.length());
	if (assureSize(length() + lstr)) {
		blkmov(0, lstr);
		REGISTER const WChar *ustr = str.data();
		REGISTER WChar *uthis = data();
		for (int i = 0; i < lstr; ++i)
			*uthis++ = *ustr++;
		len_r() += lstr;
	}
	return *this;
}

LString &LString::prependUcs2Code(WChar wc)
{
	clone();
	
	if (assureSize(length() + 1)) {
		blkmov(0, 1);
		*data() = wc;
		++len_r();
	}
	return *this;
}

LString &LString::remove(uint index, uint len)
{
	uint olen = length();
	if (index >= olen) {
		// range problem
	}
	else if (index + len >= olen) {
		clone();
		len_r() = index;
	}
	else {
		clone();
		memmove(data() + index, data() + index + len, 
			sizeof(WChar) * (olen - index - len));
		len_r() = olen - len;
	}
	return *this;
}

short LString::toShort(bool *ok, int base) const
{
	long v = toLong(ok, base);
	if (ok != NULL && *ok && (v < -32768 || v > 32767)) {
		*ok = false;
		v = 0;
	}
	return static_cast<short>(v);
}

unsigned short LString::toUShort(bool *ok, int base) const
{
	unsigned long v = toULong(ok, base);
	if (ok != NULL && *ok && (v > 65535)) {
		*ok = false;
		v = 0;
	}
	return static_cast<unsigned short>(v);

}

int LString::toInt(bool *ok, int base) const
{
	return static_cast<int>(toLong(ok, base));
}

uint LString::toUInt(bool *ok, int base) const
{
	return static_cast<uint>(toULong(ok, base));
}

long LString::toLong(bool *ok, int base) const
{
	const WChar *p = unicode();
	long val = 0;
	int l = length();
	const long max_mult = INT_MAX / base;
	bool is_ok = false;
	int neg = 0;

	if (p == NULL)
		goto BYE;

	while (l != 0 && wc_is_space(*p)) {			// skip leading space
		--l;
		++p;
	}
	if (l != 0 && *p == '-') {
		--l;
		++p;
		neg = 1;
	} 
	else if (*p == '+') {
		--l;
		++p;
	}

	// NOTE: toULong() code is similar
	if (l == 0 || !wc_ok_in_base(*p, base))
		goto BYE;

	while (l != 0 && wc_ok_in_base(*p, base)) {
		--l;
		int dv;
		if (wc_is_digit(*p))
			dv = wc_digit_value(*p);
		else {
			if (*p >= 'a' && *p <= 'z')
				dv = *p - 'a' + 10;
			else
				dv = *p - 'A' + 10;
		}
		if (val > max_mult 
			|| (val == max_mult && dv > (INT_MAX % base) + neg))
			goto BYE;
		val = base * val + dv;
		++p;
	}
	if (neg)
		val = -val;
	while (l != 0 && wc_is_space(*p)) {	// skip trailing space
		--l;
		++p;
	}
	if (l == 0)
		is_ok = true;

BYE:
	if (ok != NULL)
		*ok = is_ok;
	return is_ok ? val : 0;
}

unsigned long LString::toULong(bool *ok, int base) const
{
	const WChar *p = unicode();
	unsigned long val = 0;
	int l = length();
	const unsigned long max_mult = UINT_MAX / base;
	bool is_ok = false;

	if (p == NULL)
		goto BYE;
	while (l != 0 && wc_is_space(*p)) {	// skip leading space
		--l;
		++p;
	}
	if (*p == '+') {
		--l;
		++p;
	}

	// NOTE: toLong() code is similar
	if (l == 0 || !wc_ok_in_base(*p, base))
		goto BYE;

	while (l != 0 && wc_ok_in_base(*p, base)) {
		--l;
		uint dv;
		if (wc_is_digit(*p)) 
			dv = wc_digit_value(*p);
		else {
			if (*p >= 'a' && *p <= 'z')
				dv = *p - 'a' + 10;
			else
				dv = *p - 'A' + 10;
		}
		if (val > max_mult 
			|| (val == max_mult && dv > (UINT_MAX % base)))
			goto BYE;
		val = base * val + dv;
		++p;
	}

	while (l != 0 && wc_is_space(*p)) {	// skip trailing space
		--l;
		++p;
	}
	if (l == 0)
		is_ok = true;

BYE:
	if (ok != NULL)
		*ok = is_ok;
	return is_ok ? val : 0;
}

float LString::toFloat(bool *ok) const
{
	char *end;
	const char *a = toLatin1();
	float val = strtof(a, &end);
	if (ok != NULL)
		*ok = (*a != '\0' && (end == NULL || *end == '\0'));
	return val;
}

double LString::toDouble(bool *ok) const
{
	char *end;
	const char *a = toLatin1();
	double val = strtod(a, &end);
	if (ok != NULL)
		*ok = (*a != '\0' && (end == NULL || *end == '\0'));
	return val;
}

LString &LString::setNumber(long n, int base)
{
	char sbuf[65];
	char *p = &sbuf[64];
	int  len = 0;
	bool neg = false;

	if (n < 0) {
		neg = true;
		if (n == INT_MIN) {
			// Cannot always negate this special case
			LString s1, s2;
			s1.setNumber(n / base);
			s2.setNumber((-(n + base)) % base);
			*this = s1 + s2;
			return *this;
		}
		n = -n;
	} 

	do {
		*--p = "0123456789abcdefghijklmnopqrstuvwxyz"[static_cast<int>(n % base)];
		n /= base;
		++len;
	} while (n != 0);

	if (neg) {
		*--p = '-';
		++len;
	}

	return setLatin1(p, len);
}

LString &LString::setNumber(unsigned long n, int base)
{
	char sbuf[65];
	char *p = &sbuf[64];
	int len = 0;

	do {
		*--p = "0123456789abcdefghijklmnopqrstuvwxyz"[static_cast<int>(n % base)];
		n /= base;
		++len;
	} while (n != 0);

	return setLatin1(p, len);
}

LString &LString::setNumber(double n, char f, int prec)
{
	char format[20];
	char sbuf[512]; // I just hope this is large enough in all cases.
	char *fs = format; // generate format string

	*fs++ = '%'; // "%.<prec>l<f>"
	if (prec >= 0) {
		if (prec > 99) // sbuf big enough for precision?
			prec = 99;
		*fs++ = '.';
		if (prec >= 10) {
			*fs++ = prec / 10 + '0';
			*fs++ = prec % 10 + '0';
		} 
		else
			*fs++ = prec + '0';
	}
	*fs++ = 'l';
	*fs++ = f;
	*fs = '\0';
	::sprintf(sbuf, format, n); // snprintf is unfortunately not portable.
	return setLatin1(sbuf);
}

LString LString::number(long n, int base)
{
	LString s;
	s.setNumber(n, base);
	return s;
}

LString LString::number(unsigned long n, int base)
{
	LString s;
	s.setNumber(n, base);
	return s;
}

LString LString::number(int n, int base)
{
	LString s;
	s.setNumber(n, base);
	return s;
}

LString LString::number(uint n, int base)
{
	LString s;
	s.setNumber(n, base);
	return s;
}

LString LString::number(double n, char f, int prec)
{
	LString s;
	s.setNumber(n, f, prec);
	return s;
}

const char *LString::toLatin1() const
{
	LString *self = const_cast<LString *>(this);

	if (self->_buf_latin1 != NULL)
		return self->_buf_latin1;

	if (isNull())
		return g_NullString;

	if (isEmpty())
		return g_EmptyString;

	self->_buf_latin1 = NEW char[len() + 1];
	if (self->_buf_latin1 == NULL)
		return g_EmptyString;

	REGISTER char *pcs = self->_buf_latin1;
	REGISTER const WChar *uthis = data();
	for (int i = 0; i < len(); ++i)
		*pcs++ = *uthis++;
	*pcs = 0;
	return self->_buf_latin1;
}

const char *LString::toUtf8() const
{
	LString *self = const_cast<LString *>(this);

	if (self->_buf_utf8 != NULL)
		return self->_buf_utf8;

	if (isNull())
		return g_NullString;

	if (isEmpty())
		return g_EmptyString;

	self->_buf_utf8 = NEW char[len() * 3 + 1];
	if (self->_buf_utf8 == NULL)
		return g_EmptyString;
	
	REGISTER char *dst = self->_buf_utf8;
	REGISTER const WChar *uthis = data();
	for (int i = 0; i < len(); ++i, ++uthis) {
		int nbytes = ucs2_to_utf8(*uthis, dst);
		dst += nbytes;
		if (nbytes == 0)
			break;
	}

	*dst = 0;
	return self->_buf_utf8;
}

LString &LString::setLatin1(const char *str, int len)
{
	clear();

	if (len == -1)
		len = strlen(str);

	if (!assureSize(len))
		return *this;

	REGISTER const char *pcs = str;
	REGISTER WChar *uthis = data();
	for (int i = 0; i < len; ++i)
		*uthis++ = (*pcs++ & 0xff);

	len_r() = len;
	return *this;
}

LString &LString::setUnicode(const unsigned short *unicodeAsUShorts, uint len)
{
	clear();

	if (!assureSize(len))
		return *this;

	REGISTER const WChar *usrc = unicodeAsUShorts;
	REGISTER WChar *uthis = data();
	for (int i = 0; i < static_cast<int>(len); ++i)
		*uthis++ = *usrc++;

	len_r() = len;
	return *this;
}

LString LString::fromLatin1(const char *str, int len)
{
	LString res;
	res.setLatin1(str, len);
	return res;
}

LString LString::fromUtf8(const char *utf8, int len)
{
	LString res;

	if (len == -1)
		len = strlen(utf8);

	if (!res.assureSize(len))
		return res;

	WChar *ures = res.data();
	int i = 0;
	int nchars = 0;
	while (i < len) {
		int nbytes = utf8_to_ucs2(utf8 + i, ures + nchars);
		i += nbytes;
		if (nbytes > 0)
			++nchars;
		else
			break;
	}

	res.len_r() = nchars;
	return res;
}

LString LString::fromUcs2(const char *ucs2buf, int len, bool bigEndian)
{
	LString res;

	const int nchars = len / 2;
	if (!res.assureSize(nchars))
		return res;

	REGISTER const char *pbuf = ucs2buf;
	REGISTER WChar *ures = res.data();
	if (bigEndian) {
		for (int i = 0; i < nchars; ++i) {
			*ures++ = (*pbuf << 8) | (*(pbuf + 1) & 0xff);
			pbuf += 2;
		}
	}
	else {
		for (int i = 0; i < nchars; ++i) {
			*ures++ = (*pbuf & 0xff) | (*(pbuf + 1) << 8);
			pbuf += 2;
		}
	}

	res.len_r() = nchars;
	return res;
}

// LString non-members operators

bool MyTools::operator==(const LString &s1, const LString &s2)
{
	return (s1.length() == s2.length()) 
			&& s1.isNull() == s2.isNull() 
			&& (ucstrncmp(s1.unicode(), s2.unicode(), s1.length()) == 0);
}

bool MyTools::operator==(const LString &s1, const char *s2)
{
	if (s2 == NULL)
		return s1.isNull();

	int n = s1.length();
	const WChar *us1 = s1.unicode();
	while (n > 0) {
		if (!(*s2) || *us1 != (*s2 & 0xff))
			break;
		++us1;
		++s2;
		--n;
	}
	return (n ? false : (*s2) ? false : true);
}

bool MyTools::operator==(const char *s1, const LString &s2)
{ return s1 == s2; }

bool MyTools::operator!=(const LString &s1, const LString &s2)
{ return !(s1 == s2); }

bool MyTools::operator!=(const LString &s1, const char *s2)
{ return !(s1 == s2); }

bool MyTools::operator!=(const char *s1, const LString &s2)
{ return !(s1 == s2); }

bool MyTools::operator<(const LString &s1, const LString &s2)
{ return ucstrcmp(s1, s2) < 0; }

bool MyTools::operator<(const LString &s1, const char *s2)
{ return ucstrcmp(s1, s2) < 0; }

bool MyTools::operator<(const char *s1, const LString &s2)
{ return ucstrcmp(s1, s2) < 0; }

bool MyTools::operator<=(const LString &s1, const LString &s2)
{ return ucstrcmp(s1, s2) <= 0; }

bool MyTools::operator<=(const LString &s1, const char *s2)
{ return ucstrcmp(s1, s2) <= 0; }

bool MyTools::operator<=(const char *s1, const LString &s2)
{ return ucstrcmp(s1, s2) <= 0; }

bool MyTools::operator>(const LString &s1, const LString &s2)
{ return ucstrcmp(s1, s2) > 0; }

bool MyTools::operator>(const LString &s1, const char *s2)
{ return ucstrcmp(s1, s2) > 0; }

bool MyTools::operator>(const char *s1, const LString &s2)
{ return ucstrcmp(s1, s2) > 0; }

bool MyTools::operator>=(const LString &s1, const LString &s2)
{ return ucstrcmp(s1, s2) >= 0; }

bool MyTools::operator>=(const LString &s1, const char *s2)
{ return ucstrcmp(s1, s2) >= 0; }

bool MyTools::operator>=(const char *s1, const LString &s2)
{ return ucstrcmp(s1, s2) >= 0; }
