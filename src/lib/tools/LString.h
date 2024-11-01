#ifndef MYTOOLS_STRING_H
#define MYTOOLS_STRING_H

#include <stdio.h>

namespace MyTools
{
	typedef unsigned short WChar;

	class LString 
	{
	private:
		WChar *_d; // RefCount|CharCount|BufferSize|chars...
		
		// Temporary buffers
		char *_buf_latin1;
		char *_buf_utf8;

	private:
		int ref() const;
		int unref() const;

		WChar *data();
		const WChar *data() const;

		unsigned short len() const;
		unsigned short &len_r();

		void init();
		void destroy();
		void clone();
		bool assureSize(unsigned int);

		void blkmov(int from, int offset);

	public:
		LString();
		LString(char);
		LString(const char *);
		LString(const unsigned short *unicodeAsUShorts, unsigned int len);
		LString(const LString &);
		~LString();

		LString &operator=(char);
		LString &operator=(const char *);
		LString &operator=(const LString &);

		static const LString null;

		bool isEmpty() const;
		bool isNull() const;

		unsigned int length() const;

		void clear();
		void truncate(unsigned int newLen);

		LString simplified() const;
		LString toLower() const;
		LString toUpper() const;

		LString &sprintf(const char *cformat, ...);
		static LString format(const char *cformat, ...);

		int indexOf(char c, int from = 0, bool caseSensitive = true) const;
		int indexOf(const char *str, int from = 0, bool caseSensitive = true) const;
		int indexOf(const LString &str, int from = 0, bool caseSensitive = true) const;
		int lastIndexOf(char c, int from = -1, bool caseSensitive = true) const;
		int lastIndexOf(const char *str, int from = -1, bool caseSensitive = true) const;
		int lastIndexOf(const LString &str, int from = -1, bool caseSensitive = true) const;
		int findCharOf(const char *chset) const;

		LString left(unsigned int len) const;
		LString right(unsigned int len) const;
		LString mid(unsigned int index, unsigned int len = 0xffffffff) const;

		LString *split(char sep, int *n) const;
		LString *split(const char *sep, int *n) const;
		LString *split(const LString &sep, int *n) const;

		LString &append(char c);
		LString &append(const char *);
		LString &append(const LString &);
		LString &appendUcs2Code(WChar wc);

		LString &prepend(char c);
		LString &prepend(const char *);
		LString &prepend(const LString &);
		LString &prependUcs2Code(WChar wc);

		LString &remove(unsigned int index, unsigned int len);

		short toShort(bool *ok = NULL, int base = 10) const;
		unsigned short toUShort(bool *ok = NULL, int base = 10) const;
		int toInt(bool *ok = NULL, int base = 10) const;
		unsigned int toUInt(bool *ok = NULL, int base = 10) const;
		long toLong(bool *ok = NULL, int base = 10) const;
		unsigned long toULong(bool *ok = NULL, int base = 10) const;
		float toFloat(bool *ok = NULL) const;
		double toDouble(bool *ok = NULL) const;

		LString &setNumber(short, int base = 10);
		LString &setNumber(unsigned short, int base = 10);
		LString &setNumber(int, int base = 10);
		LString &setNumber(unsigned int, int base = 10);
		LString &setNumber(long, int base = 10);
		LString &setNumber(unsigned long, int base = 10);
		LString &setNumber(float, char f = 'g', int prec = 6);
		LString &setNumber(double, char f = 'g', int prec = 6);

		static LString number(long, int base = 10);
		static LString number(unsigned long, int base = 10);
		static LString number(int, int base = 10);
		static LString number(unsigned int, int base = 10);
		static LString number(double, char f = 'g', int prec = 6);

		WChar operator[](int i) const;
		WChar &operator[](int i);

		const char *toLatin1() const;
		const char *toUtf8() const;
		const WChar *unicode() const;

		LString &setLatin1(const char *str, int len = -1);
		LString &setUnicode(const unsigned short *unicodeAsUShorts, unsigned int len);

		static LString fromLatin1(const char *str, int len = -1);
		static LString fromUtf8(const char *utf8, int len = -1);
		static LString fromUcs2(const char *ucs2, int len, bool bigEndian = true);
	};

	// LString inline functions

	inline bool LString::isEmpty() const
	{ return length() == 0; }

	inline bool LString::isNull() const
	{ return _d == NULL; }

	inline unsigned int LString::length() const
	{ return _d == NULL ? 0 : _d[1]; }

	inline int LString::indexOf(const char *str, int from, bool caseSensitive) const
	{ return indexOf(LString::fromLatin1(str), from, caseSensitive); }

	inline int LString::lastIndexOf(const char *str, int from, bool caseSensitive) const
	{ return lastIndexOf(LString::fromLatin1(str), from, caseSensitive); }

	inline LString *LString::split(char sep, int *n) const
	{ return split(LString(sep), n); }

	inline LString *LString::split(const char *sep, int *n) const
	{ return split(LString::fromLatin1(sep), n); }

	inline LString &LString::append(char c)
	{ return appendUcs2Code(c & 0xff); }

	inline LString &LString::prepend(char c)
	{ return prependUcs2Code(c & 0xff); }

	inline LString &LString::setNumber(short n, int base)
	{ return setNumber(static_cast<long>(n), base); }

	inline LString &LString::setNumber(unsigned short n, int base)
	{ return setNumber(static_cast<unsigned long>(n), base); }

	inline LString &LString::setNumber(int n, int base)
	{ return setNumber(static_cast<long>(n), base); }

	inline LString &LString::setNumber(unsigned int n, int base)
	{ return setNumber(static_cast<unsigned long>(n), base); }

	inline LString &LString::setNumber(float n, char f, int prec)
	{ return setNumber(static_cast<double>(n), f, prec); }

	inline WChar LString::operator[](int i) const
	{ return *(_d + 3 + i); }

	inline WChar &LString::operator[](int i)
	{ return *(_d + 3 + i); }

	inline const WChar *LString::unicode() const
	{ return _d == NULL ? NULL : _d + 3; }

	// LString non-members operators

	bool operator==(const LString &s1, const LString &s2); 
	bool operator==(const LString &s1, const char *s2); 
	bool operator==(const char *s1, const LString &s2); 
	bool operator!=(const LString &s1, const LString &s2); 
	bool operator!=(const LString &s1, const char *s2); 
	bool operator!=(const char *s1, const LString &s2); 
	bool operator<(const LString &s1, const LString &s2); 
	bool operator<(const LString &s1, const char *s2);
	bool operator<(const char *s1, const LString &s2); 
	bool operator<=(const LString &s1, const LString &s2); 
	bool operator<=(const LString &s1, const char *s2); 
	bool operator<=(const char *s1, const LString &s2); 
	bool operator>(const LString &s1, const LString &s2); 
	bool operator>(const LString &s1, const char *s2); 
	bool operator>(const char *s1, const LString &s2); 
	bool operator>=(const LString &s1, const LString &s2); 
	bool operator>=(const LString &s1, const char *s2); 
	bool operator>=(const char *s1, const LString &s2); 

	inline LString operator+(const LString &s1, const LString &s2)
	{
		LString res(s1);
		res.append(s2);
		return res;
	}

	inline LString operator+(const LString &s1, const char *s2)
	{
		LString res(s1);
		res.append(LString::fromLatin1(s2));
		return res;
	}

	inline LString operator+(const char *s1, const LString &s2)
	{
		LString res = LString::fromLatin1(s1);
		res.append(s2);
		return res;
	}

	inline LString operator+(const LString &s, char c)
	{
		LString res(s);
		res.append(c);
		return res;
	}

	inline LString operator+(char c, const LString &s)
	{
		LString res;
		res.append(c);
		res.append(s);
		return res;
	}
};

#endif
