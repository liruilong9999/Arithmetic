#include <math.h>
#include <assert.h>

#include "utmproject.h"

using namespace Geo;

#define _M_E	2.7182818284590452354	// value of e
#define _M_PI	3.14159265358979323846	// value of pi
#define _M_PI_2	1.57079632679489661923	// value of pi/2
#define _M_PI_4	0.78539816339744830962	// value of pi/4

#define RAD_FACTOR 0.01745329251994329509	// value of pi/180 
#define DEGREE2RAD(d) ((d) * RAD_FACTOR)

#define DEG_FACTOR 57.29577951308232311		// value of 180/pi 
#define RAD2DEGREE(r) ((r) * DEG_FACTOR)

// Mercator members

void Mercator::initConst()
{
	_COS_STDLAT = cos(_stdLat);

	switch (_datumType) {
	case Krassovsky:
		_A = 6378245.0;
		_B = 6356863.0188;
		break;
	case IAG75:
		_A = 6378140.0;
		_B = 6356755.2882;
		break;
	case WGS84:
		_A = 6378137.0;
		_B = 6356752.3142;
		break;
	case WebMercator:
		_A = _B = 6378137.0;
		break;
	default:
		assert(0);
		break;
	}

	_AA_B = _A * _A / _B;
	_E1 = sqrt(1 - (_B / _A) * (_B / _A));
	_E1_2 = _E1 / 2;
	_E2 = sqrt((_A / _B) * (_A / _B) - 1);
	_NB0 = _AA_B / sqrt(1 + _E2 * _E2 * _COS_STDLAT * _COS_STDLAT);
	_K = _NB0 * _COS_STDLAT;
}

Mercator::Mercator(DatumType dtype)
	: _datumType(dtype), _stdLat(0.0), _origLon(0.0)
{
	initConst();
}

Mercator::~Mercator()
{
}

void Mercator::setDatumType(DatumType dtype)
{
	if (_datumType != dtype) {
		_datumType = dtype;
		initConst();
	}
}

void Mercator::setStdLatitude(double lat)
{
	_stdLat = DEGREE2RAD(lat);

	_COS_STDLAT = cos(_stdLat);

	_NB0 = _AA_B / sqrt(1 + _E2 * _E2 * _COS_STDLAT * _COS_STDLAT);
	_K = _NB0 * _COS_STDLAT;
}

void Mercator::setOrigLongitude(double lon)
{
	_origLon = DEGREE2RAD(lon);
}

void Mercator::project(double lon, double lat, double *x, double *y) const
{
	double rlon = DEGREE2RAD(lon);
	double rlat = DEGREE2RAD(lat);
	double sin_lat = sin(rlat);

	*x = _K * (rlon - _origLon);
	*y = _K * log(tan(_M_PI_4 + rlat / 2) 
			* pow((1 - _E1 * sin_lat) / (1 + _E1 * sin_lat), _E1_2));
}

void Mercator::aproject(double x, double y, double *lon, double *lat) const
{
	double rlon = x / _K + _origLon;
	double rlat = 0.0;

	for (int i = 0; i < 10; ++i) {
		double sin_lat = sin(rlat);
		rlat = _M_PI_2 - 2 * atan(pow(_M_E, (-y / _K)) 
				* pow(_M_E, _E1_2 * log((1 - _E1 * sin_lat) / (1 + _E1 * sin_lat))));
	}

	*lon = RAD2DEGREE(rlon);
	*lat = RAD2DEGREE(rlat);
}

#define CMF 111319.4907932735683082883485 // circumference factor (2πr/360)

double Geo::distance(double x1, double y1, double x2, double y2)
{
	x1 = DEGREE2RAD(x1);
	x2 = DEGREE2RAD(x2);
	y1 = DEGREE2RAD(y1);
	y2 = DEGREE2RAD(y2);
	double alpha = acos(cos(y1) * cos(y2) * cos(x1 - x2) + sin(y1) * sin(y2));
	return RAD2DEGREE(alpha) * CMF;
}
