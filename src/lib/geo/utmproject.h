#ifndef UTMPROJECT_H
#define UTMPROJECT_H

#include "geo_gloabal.h"

namespace Geo {
class GEO_EXPORT Mercator
{
public:
    enum DatumType
    {
        Krassovsky = 1,
        IAG75,
        WGS84,
        WebMercator
    };

private:
    DatumType _datumType;
    double    _stdLat;  // radian value of standard latitude
    double    _origLon; // radian value of original longitude

    // lazy values
    double _COS_STDLAT; // cos(_stdLat)
    double _A;          // ellipse long half axis
    double _B;          // ellipse short half axis
    double _AA_B;       // (_A * _A / _B)
    double _E1;         // sqrt(1 - (_B / _A) * (_B / _A))
    double _E1_2;       // (_E1 / 2)
    double _E2;         // sqrt((_A / _B) * (_A / _B) - 1)
    double _NB0;
    double _K;

private:
    void initConst();

public:
    Mercator(DatumType dtype = WGS84);
    ~Mercator();

    void      setDatumType(DatumType dtype);
    DatumType datumType() const;

    // Sets standard latitude degree. (-90.0 90.0)
    void setStdLatitude(double lat);

    // Sets original longitude degree. (-180.0 180.0)
    void setOrigLongitude(double lon);

    // Do projection from geodetic coordinate (decimal degree) to
    // cartesian coordinate (meter).
    void project(double lon, double lat, double * x, double * y) const;

    // Do anti-projection from cartesian coordinate (meter) to
    // geodetic coordinate (decimal degree).
    void aproject(double x, double y, double * lon, double * lat) const;
};

// Mercator inline functions

inline Mercator::DatumType Mercator::datumType() const
{
    return _datumType;
}

// ~

// Returns spherical distance meters between two points.
// The two points represented by geodetic coordinate (decimal degree).
GEO_EXPORT double distance(double x1, double y1, double x2, double y2);
}; // namespace Geo

#endif
