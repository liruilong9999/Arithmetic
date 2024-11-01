#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "gcoord.h"

using namespace Geo;

// GCoord members

inline void GCoord::pack(bool nsign, int d, int m, int s, int ts)
{
	_dms = 0;

	ts <<= 1;
	ts |= (nsign ? 1 : 0);

	_dms |= d;
	_dms <<= 8;
	_dms |= m;
	_dms <<= 8;
	_dms |= s;
	_dms <<= 8;
	_dms |= ts;
}

void GCoord::unpack()
{
	double tmp = (double)gcSecond(_dms) + (double)gcTSecond(_dms) / 10.0;
	_fm = (double)gcMinute(_dms) + tmp / 60.0;
	_fd = (double)gcDegree(_dms) + _fm / 60.0;
	_cached = true;
}

void GCoord::setCoord(double d)
{
	bool nsign = signbit(d);
	double ad = fabs(d);

	int d_part = (int)floor(ad);
	double tmp = (ad - (double)d_part) * 60.0; // fractional minutes

	_fd = ad;
	_fm = tmp;
	_cached = true;

	int m_part = (int)floor(tmp);
	tmp = (tmp - (double)m_part) * 60.0; // fractional seconds
	int s_part = (int)floor(tmp);
	tmp = tmp - (double)s_part;
	int ts_part = (int)(floor(tmp * 10.0));

	pack(nsign, d_part, m_part, s_part, ts_part);
}

void GCoord::setCoord(char ind, int d, double m)
{
	bool nsign = (ind == 's' || ind == 'w' || ind == 'S' || ind == 'W');

	int d_part = abs(d);
	double tmp = fabs(m); // fractional minutes
	int m_part = (int)floor(tmp);

	if (m_part >= 60) {
		d_part += m_part / 60;
		m_part %= 60;
	}

	tmp = (tmp - (double)m_part) * 60.0; // fractional seconds
	int s_part = (int)floor(tmp);
	tmp = tmp - (double)s_part;
	int ts_part = (int)(floor(tmp * 10.0));

	pack(nsign, d_part, m_part, s_part, ts_part);
}

void GCoord::setCoord(char ind, int d, int m, int s, int ts)
{
	bool nsign = (ind == 's' || ind == 'w' || ind == 'S' || ind == 'W');
	int d_part = abs(d);
	int m_part = abs(m);
	int s_part = abs(s);
	int ts_part = abs(ts);

	if (ts_part >= 10) {
		s_part += ts_part / 10;
		ts_part %= 10;
	}

	if (s_part >= 60) {
		m_part += s_part / 60;
		s_part %= 60;
	}

	if (m_part >= 60) {
		d_part += m_part / 60;
		m_part %= 60;
	}

	pack(nsign, d_part, m_part, s_part, ts_part);
}

double GCoord::getCoord()
{
	if (!_cached)
		unpack();

	return gcSign(_dms) ? -_fd : _fd;
}

void GCoord::getCoord(bool *nsign, int *d, double *m)
{
	if (!_cached)
		unpack();

	*d = gcDegree(_dms);
	*m = _fm;
	*nsign = (gcSign(_dms) == 1);
}

void GCoord::getCoord(bool *nsign, int *d, int *m, int *s, int *ts)
{
	*d = gcDegree(_dms);
	*m = gcMinute(_dms);
	*s = gcSecond(_dms);
	if (ts != NULL)
		*ts = gcTSecond(_dms);
	*nsign = (gcSign(_dms) == 1);
}

char *GCoord::toString(char *s, GCoord::Orientation o, 
					  int style, bool align, char dmark) const
{
	char sbuf[16];
	bool nsign = gcSign(_dms);
	bool isX = (o == GCoord::X);
	GCoord *self = (GCoord *)this;

	int flag = style & 0x0f;
	if (flag == GCoord::Dd) {
		if (!_cached)
			self->unpack();
		snprintf(sbuf, 15, "%.5f", _fd);
		if (align) {
			char *p = s;
			int nspaces = 0;
			const int deg = gcDegree(_dms);
			if (deg < 10)
				nspaces = 2;
			else if (deg < 100)
				nspaces = 1;
			for (; nspaces > 0; --nspaces)
				*p++ = ' ';
			strcpy(p, sbuf);
		}
		else {
			int i = strlen(sbuf) - 1;
			while (i > 0 && sbuf[i] == '0')
				--i;
			if (sbuf[i] == '.')
				++i;
			strncpy(s, sbuf, i + 1);
			s[i + 1] = '\0';
		}
	}
	else if (flag == GCoord::DMm) {
		if (!_cached)
			self->unpack();
		char *p = sbuf;
		char tmpbuf[16];
		if (align && gcMinute(_dms) < 10)
			*p++ = ' ';
		snprintf(p, 14, "%.3f", _fm);

		if (align)
			snprintf(tmpbuf, 15, "%3d%c", gcDegree(_dms), dmark);
		else {
			int i = strlen(sbuf) - 1;
			for (; i > 0 && sbuf[i] == '0'; --i) ;
			if (sbuf[i] == '.')
				i += 2;
			else
				++i;
			sbuf[i] = '\0';

			snprintf(tmpbuf, 15, "%d%c", gcDegree(_dms), dmark);
		}

		strcpy(s, tmpbuf);
		strcat(s, sbuf);
		strcat(s, "\'");
	}
	else if (flag == GCoord::DMS) {
		const char *fmt = NULL;
		if (align)
			fmt = "%3d%c%2d\'%2d\"";
		else
			fmt = "%d%c%d\'%d\"";
		snprintf(sbuf, 15, fmt, gcDegree(_dms), dmark, gcMinute(_dms), gcSecond(_dms));
		strcpy(s, sbuf);
	}

	// Save the result string
	strcpy(sbuf, s);
	char *p = s;

	flag = style & 0xf0;
	if (flag == GCoord::SignMark) {
		if (nsign) {
			int i = 0;
			while (sbuf[i] != '\0' && isspace(sbuf[i]))
				++i;
			if (i > 0)
				sbuf[i - 1] = '-';
			else
				*p++ = '-';
		}
	}
	else if (flag == GCoord::PrefixInd) {
		if (nsign)
			*p++ = isX ? 'W' : 'S';
		else
			*p++ = isX ? 'E' : 'N';
	}
	else if (flag == GCoord::SuffixInd) {
		if (nsign)
			strcat(sbuf, isX ? "W" : "S");
		else
			strcat(sbuf, isX ? "E" : "N");
	}

	strcpy(p, sbuf);
	return s;
}

/*
// Testing

#include <stdio.h>

int main()
{
GCoord c;
printf("Empty coordinate: [%s] [%s]\n", 
c.toString(GCoord::X).c_str(), 
c.toString(GCoord::Y, 0, true).c_str());

c.setCoord(116.33533);
printf("%s\n", c.toString(GCoord::X, GCoord::DMS | GCoord::SuffixInd, true, '^').c_str());
c.setCoord(-39.20567);
printf("%s\n", c.toString(GCoord::Y, GCoord::DMS | GCoord::SuffixInd, true, '^').c_str());

c.setCoord(116.33533);
printf("%s\n", c.toString(GCoord::X, GCoord::Dd | GCoord::SignMark, true, '^').c_str());
c.setCoord(-39.20567);
printf("%s\n", c.toString(GCoord::Y, GCoord::Dd | GCoord::SignMark, true, '^').c_str());

return 0;
}
 */
