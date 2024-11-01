#ifndef GCOORD_H
#define GCOORD_H

#include <stdio.h>
#include "geo_gloabal.h"

// The GCoord class represents a coordinate item of 
// latitude/longitude.

namespace Geo
{
	typedef unsigned long gcDMS;

	inline int gcDegree(gcDMS v)
	{ return static_cast<int>((v >> 24) & 0xff); }

	inline int gcMinute(gcDMS v)
	{ return static_cast<int>((v >> 16) & 0xff); }

	inline int gcSecond(gcDMS v)
	{ return static_cast<int>((v >> 8) & 0xff); }

	inline int gcTSecond(gcDMS v)
	{ return static_cast<int>((v & 0xff) >> 1); }

	inline int gcSign(gcDMS v)
	{ return v & 0x1; }

	class GEO_EXPORT GCoord
	{
	public:
		enum Orientation { X, Y };

		// Coordinate format
		enum Format { 
			Dd = 0x00,  // d.ddddd
			DMm = 0x01, // d m.mmm
			DMS = 0x02  // d m s
		};

		// Direction indicator
		enum Indicator {
			SignMark = 0x00,  // +/-
			PrefixInd = 0x10, // Prefix N/S/E/W
			SuffixInd = 0x20  // Suffix N/S/E/W
		};

	private:
		gcDMS _dms; // package value

		double _fd; // fractional degree (no sign)
		double _fm; // fractional minute (no sign)
		bool _cached;

	private:
		void pack(bool nsign, int d, int m, int s, int ts = 0);
		void unpack();

	public:
		// Constructs a zero coordinate.
		GCoord();
		// Constructs a coordinate with decimal degree.
		// Negative for south/west, positive for north/east.
		GCoord(double d);
		// Constructs a coordinate with degree and decimal minute.
		// The indicator ind may be "NnSs" or "EeWw"
		GCoord(char ind, int d, double m);
		// Constructs a coordinate with detail sections.
		// The indicator ind may be "NnSs" or "EeWw"
		GCoord(char ind, int d, int m, int s, int ts = 0);

		// Sets coordinate to decimal degree.
		// Negative for south/west, positive for north/east.
		void setCoord(double d);
		// Sets coordinate to degree and decimal minute.
		// The indicator ind may be "NnSs" or "EeWw"
		void setCoord(char ind, int d, double m);
		// Sets coordinate to detail sections.
		// The indicator ind may be "NnSs" or "EeWw"
		void setCoord(char ind, int d, int m, int s, int ts = 0);

		double getCoord();
		void getCoord(bool *nsign, int *d, double *m);
		void getCoord(bool *nsign, int *d, int *m, int *s, int *ts = NULL);

		gcDMS dms() const;

		// Returns the coordinate as string.
		// The style is bitwise OR between Format and Indicator.
		char *toString(char *s, Orientation, int style = 0, 
				bool align = false, char dmark = ' ') const;
	};

	// GCoord inline functions

	inline GCoord::GCoord()
		: _dms(0), _fd(0.0), _fm(0.0), _cached(false) {}

	inline GCoord::GCoord(double d)
	{ setCoord(d); }

	inline GCoord::GCoord(char ind, int d, double m)
	{ setCoord(ind, d, m); }

	inline GCoord::GCoord(char ind, int d, int m, int s, int ts)
	{ setCoord(ind, d, m, s, ts); }

	inline gcDMS GCoord::dms() const
	{ return _dms; }
};

#endif
