#ifndef MYTOOLS_STRING_H
#define MYTOOLS_STRING_H

#include <stdio.h>

namespace MyTools
{
	typedef unsigned short WChar;

	class String 
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
		String();
		String(char);
		String(const char *);
		String(const unsigned short *unicodeAsUShorts, unsigned int len);
		String(const String &);
		~String();

		String &operator=(char);
		String &operator=(const char *);
		String &operator=(const String &);

		static const String null;

		bool isEmpty() const;
		bool isNull() const;

		unsigned int length() const;

		void clear();
		void truncate(unsigned int newLen);

		String simplified() const;
		String toLower() const;
		String toUpper() const;

		String &sprintf(const char *cformat, ...);
		static String format(const char *cformat, ...);

		int indexOf(char c, int from = 0, bool caseSensitive = true) const;
		int indexOf(const char *str, int from = 0, bool caseSensitive = true) const;
		int indexOf(const String &str, int from = 0, bool caseSensitive = true) const;
		int lastIndexOf(char c, int from = -1, bool caseSensitive = true) const;
		int lastIndexOf(const char *str, int from = -1, bool caseSensitive = true) const;
		int lastIndexOf(const String &str, int from = -1, bool caseSensitive = true) const;
		int findCharOf(const char *chset) const;

		String left(unsigned int len) const;
		String right(unsigned int len) const;
		String mid(unsigned int index, unsigned int len = 0xffffffff) const;

		String *split(char sep, int *n) const;
		String *split(const char *sep, int *n) const;
		String *split(const String &sep, int *n) const;

		String &append(char c);
		String &append(const char *);
		String &append(const String &);
		String &appendUcs2Code(WChar wc);

		String &prepend(char c);
		String &prepend(const char *);
		String &prepend(const String &);
		String &prependUcs2Code(WChar wc);

		String &remove(unsigned int index, unsigned int len);

		short toShort(bool *ok = NULL, int base = 10) const;
		unsigned short toUShort(bool *ok = NULL, int base = 10) const;
		int toInt(bool *ok = NULL, int base = 10) const;
		unsigned int toUInt(bool *ok = NULL, int base = 10) const;
		long toLong(bool *ok = NULL, int base = 10) const;
		unsigned long toULong(bool *ok = NULL, int base = 10) const;
		float toFloat(bool *ok = NULL) const;
		double toDouble(bool *ok = NULL) const;

		String &setNumber(short, int base = 10);
		String &setNumber(unsigned short, int base = 10);
		String &setNumber(int, int base = 10);
		String &setNumber(unsigned int, int base = 10);
		String &setNumber(long, int base = 10);
		String &setNumber(unsigned long, int base = 10);
		String &setNumber(float, char f = 'g', int prec = 6);
		String &setNumber(double, char f = 'g', int prec = 6);

		static String number(long, int base = 10);
		static String number(unsigned long, int base = 10);
		static String number(int, int base = 10);
		static String number(unsigned int, int base = 10);
		static String number(double, char f = 'g', int prec = 6);

		WChar operator[](int i) const;
		WChar &operator[](int i);

		const char *toLatin1() const;
		const char *toUtf8() const;
		const WChar *unicode() const;

		String &setLatin1(const char *str, int len = -1);
		String &setUnicode(const unsigned short *unicodeAsUShorts, unsigned int len);

		static String fromLatin1(const char *str, int len = -1);
		static String fromUtf8(const char *utf8, int len = -1);
		static String fromUcs2(const char *ucs2, int len, bool bigEndian = true);
	};

	// String inline functions

	inline bool String::isEmpty() const
	{ return length() == 0; }

	inline bool String::isNull() const
	{ return _d == NULL; }

	inline unsigned int String::length() const
	{ return _d == NULL ? 0 : _d[1]; }

	inline int String::indexOf(const char *str, int from, bool caseSensitive) const
	{ return indexOf(String::fromLatin1(str), from, caseSensitive); }

	inline int String::lastIndexOf(const char *str, int from, bool caseSensitive) const
	{ return lastIndexOf(String::fromLatin1(str), from, caseSensitive); }

	inline String *String::split(char sep, int *n) const
	{ return split(String(sep), n); }

	inline String *String::split(const char *sep, int *n) const
	{ return split(String::fromLatin1(sep), n); }

	inline String &String::append(char c)
	{ return appendUcs2Code(c & 0xff); }

	inline String &String::prepend(char c)
	{ return prependUcs2Code(c & 0xff); }

	inline String &String::setNumber(short n, int base)
	{ return setNumber(static_cast<long>(n), base); }

	inline String &String::setNumber(unsigned short n, int base)
	{ return setNumber(static_cast<unsigned long>(n), base); }

	inline String &String::setNumber(int n, int base)
	{ return setNumber(static_cast<long>(n), base); }

	inline String &String::setNumber(unsigned int n, int base)
	{ return setNumber(static_cast<unsigned long>(n), base); }

	inline String &String::setNumber(float n, char f, int prec)
	{ return setNumber(static_cast<double>(n), f, prec); }

	inline WChar String::operator[](int i) const
	{ return *(_d + 3 + i); }

	inline WChar &String::operator[](int i)
	{ return *(_d + 3 + i); }

	inline const WChar *String::unicode() const
	{ return _d == NULL ? NULL : _d + 3; }

	// String non-members operators

	bool operator==(const String &s1, const String &s2); 
	bool operator==(const String &s1, const char *s2); 
	bool operator==(const char *s1, const String &s2); 
	bool operator!=(const String &s1, const String &s2); 
	bool operator!=(const String &s1, const char *s2); 
	bool operator!=(const char *s1, const String &s2); 
	bool operator<(const String &s1, const String &s2); 
	bool operator<(const String &s1, const char *s2);
	bool operator<(const char *s1, const String &s2); 
	bool operator<=(const String &s1, const String &s2); 
	bool operator<=(const String &s1, const char *s2); 
	bool operator<=(const char *s1, const String &s2); 
	bool operator>(const String &s1, const String &s2); 
	bool operator>(const String &s1, const char *s2); 
	bool operator>(const char *s1, const String &s2); 
	bool operator>=(const String &s1, const String &s2); 
	bool operator>=(const String &s1, const char *s2); 
	bool operator>=(const char *s1, const String &s2); 

	inline String operator+(const String &s1, const String &s2)
	{
		String res(s1);
		res.append(s2);
		return res;
	}

	inline String operator+(const String &s1, const char *s2)
	{
		String res(s1);
		res.append(String::fromLatin1(s2));
		return res;
	}

	inline String operator+(const char *s1, const String &s2)
	{
		String res = String::fromLatin1(s1);
		res.append(s2);
		return res;
	}

	inline String operator+(const String &s, char c)
	{
		String res(s);
		res.append(c);
		return res;
	}

	inline String operator+(char c, const String &s)
	{
		String res;
		res.append(c);
		res.append(s);
		return res;
	}
};

#endif
