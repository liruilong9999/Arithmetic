#ifndef RCS_ROTATION_H
#define RCS_ROTATION_H

#include "trigonf.h"
#include "geo_gloabal.h"
namespace Geo
{
	// Rectanglar Coordinate System rotation
	class GEO_EXPORT RcsRotation
	{
	protected:
		int _angle;

		// Origin transform
		long _dx;
		long _dy;

		// Trigonometric function value saved
		trigonf_t _SIN;
		trigonf_t _COS;

	public:
		// Constructs an invalid object
		RcsRotation();
		// Constructs a rotation with (0,0) origin and a angle
		// The angle may be from -360 to 360 (all inclusive).
		RcsRotation(int a);
		// Constructs a rotation with origin and a angle
		// The origin will be translated to (x,y)
		RcsRotation(long x, long y, int a);

		bool isValid() const;

		void setRotation(long x, long y, int a);
		void setAngle(int);
		void translate(long x, long y);

		// For convenience, like setRotation(0, 0, 0)
		void zero();

		// Returns the rotation angle
		// The returns value is between 0 and 360 (NOT inclusive).
		int angle() const;

		void rotate(int x, int y, int *rx, int *ry) const;
		void frotate(double x, double y, double *rx, double *ry) const;
	};

	// RcsRotation inline functions

	inline bool RcsRotation::isValid() const
	{ return _angle >= 0; }

	inline int RcsRotation::angle() const
	{ return _angle; }
};

#endif
