#include <assert.h>

#include "grect.h"

using namespace Geo;

// FRect members

void GRect::setCoords(G_INT32 x1, G_INT32 y1, G_INT32 x2, G_INT32 y2)
{
	_x1 = x1;
	_y1 = y1;
	_x2 = x2;
	_y2 = y2;
}

void GRect::setRect(G_INT32 x, G_INT32 y, G_INT32 w, G_INT32 h)
{
	_x1 = x;
	_y1 = y;
	_x2 = x + w - 1;
	_y2 = y + h - 1;
}

GRect GRect::operator|(const GRect &r) const
{
	GRect tmp;
	tmp._x1 = MIN(_x1, r._x1);
	tmp._x2 = MAX(_x2, r._x2);
	tmp._y1 = MIN(_y1, r._y1);
	tmp._y2 = MAX(_y2, r._y2);
	return tmp;
}

GRect GRect::operator&(const GRect &r) const
{
	GRect tmp;
	tmp._x1 = MAX(_x1, r._x1);
	tmp._x2 = MIN(_x2, r._x2);
	tmp._y1 = MAX(_y1, r._y1);
	tmp._y2 = MIN(_y2, r._y2);
	return tmp;
}

GRect &GRect::operator|=(const GRect &r)
{
	*this = *this | r;
	return *this;
}

GRect &GRect::operator&=(const GRect &r)
{
	*this = *this & r;
	return *this;
}

GRect GRect::unite(const GRect &r) const
{
	return *this | r;
}

GRect GRect::intersect(const GRect &r) const
{
	return *this & r;
}

void GRect::expand(G_INT32 dx, G_INT32 dy)
{
	_x1 -= dx;
	_x2 += dx;
	_y1 -= dy;
	_y2 += dy;
}

void GRect::moveBy(G_INT32 dx, G_INT32 dy)
{
	_x1 += dx;
	_x2 += dx;
	_y1 += dy;
	_y2 += dy;
}
