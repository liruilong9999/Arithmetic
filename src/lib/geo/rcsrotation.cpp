#include <math.h>
#include <assert.h>

#include "rcsrotation.h"

using namespace Geo;

// RcsRotation members

RcsRotation::RcsRotation()
{
	_angle = -1;
	_dx = 0;
	_dy = 0;
	_SIN = 0;
	_COS = 0;
}

RcsRotation::RcsRotation(int a)
{
	_dx = 0;
	_dy = 0;
	setAngle(a);
}

RcsRotation::RcsRotation(long x, long y, int a)
{
	setRotation(x, y, a);
}

void RcsRotation::setRotation(long x, long y, int a)
{
	_dx = x;
	_dy = y;
	setAngle(a);
}

void RcsRotation::setAngle(int a)
{
	assert(a >= -360 && a <= 360);

	if (a < 0)
		a += 360;

	if (a == 0) {
		_SIN = 0;
		_COS = Trigonf::MULTIPLER;
	}
	else if (_angle != a) {
		_SIN = Trigonf::sin(a);
		_COS = Trigonf::cos(a);
	}

	_angle = a;
}

void RcsRotation::translate(long x, long y)
{
	_dx = x;
	_dy = y;
}

void RcsRotation::zero()
{
	_angle = 0;
	_dx = 0;
	_dy = 0;
	_SIN = 0;
	_COS = Trigonf::MULTIPLER;
}

void RcsRotation::rotate(int x, int y, int *rx, int *ry) const
{
	long tx = x - _dx;
	long ty = y - _dy;

	const long error = Trigonf::MULTIPLER / 2;
	long tmpx = (tx * _COS + ty * _SIN);
	long tmpy = (ty * _COS - tx * _SIN);

	if (tmpx < 0)
		tmpx -= error;
	else
		tmpx += error;

	if (tmpy < 0)
		tmpy -= error;
	else
		tmpy += error;
	
	*rx = tmpx / Trigonf::MULTIPLER + _dx;
	*ry = tmpy / Trigonf::MULTIPLER + _dy;
}

void RcsRotation::frotate(double x, double y, double *rx, double *ry) const
{
	double tx = x - _dx;
	double ty = y - _dy;
	double tmpx = (tx * _COS + ty * _SIN) / Trigonf::MULTIPLER;
	double tmpy = (ty * _COS - tx * _SIN) / Trigonf::MULTIPLER;

	if (signbit(tmpx))
		tmpx -= 0.5;
	else
		tmpx += 0.5;

	if (signbit(tmpy))
		tmpy -= 0.5;
	else
		tmpy += 0.5;

	*rx = tmpx + _dx;
	*ry = tmpy + _dy;
}
