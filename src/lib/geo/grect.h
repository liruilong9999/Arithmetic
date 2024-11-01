#ifndef GRECT_H
#define GRECT_H

#include "geo_gloabal.h"

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

namespace Geo
{
	typedef long G_INT32;

	const G_INT32 G_MAXV = +2147483647;
	const G_INT32 G_MINV = -2147483647;

	class GEO_EXPORT GPoint
	{
	private:
		G_INT32 _x;
		G_INT32 _y;

	public:
		GPoint();
		GPoint(G_INT32 x, G_INT32 y);

		G_INT32 x() const;
		G_INT32 y() const;

		void setX(G_INT32 x);
		void setY(G_INT32 y);

		void setPoint(G_INT32 x, G_INT32 y);

		G_INT32 &rx();
		G_INT32 &ry();

		bool operator==(const GPoint &p) const;
		bool operator!=(const GPoint &p) const;
	};

	// GPoint inline functions

	inline GPoint::GPoint() { /* do nothing */ }

	inline GPoint::GPoint(G_INT32 x, G_INT32 y) : _x(x), _y(y) {}

	inline G_INT32 GPoint::x() const
	{ return _x; }

	inline G_INT32 GPoint::y() const
	{ return _y; }

	inline void GPoint::setX(G_INT32 x)
	{ _x = x; }

	inline void GPoint::setY(G_INT32 y)
	{ _y = y; }

	inline void GPoint::setPoint(G_INT32 x, G_INT32 y)
	{ _x = x; _y = y; }

	inline G_INT32 &GPoint::rx()
	{ return _x; }

	inline G_INT32 &GPoint::ry()
	{ return _y; }

	inline bool GPoint::operator==(const GPoint &p) const
	{ return _x == p._x && _y == p._y; }

	inline bool GPoint::operator!=(const GPoint &p) const
	{ return _x != p._x || _y != p._y; }

	// ~

	// The GRect class defines a rectangle in the Cartesion 
	// coordinate system. It normally indicates a geodetic
	// rectangle after projection.
	class GEO_EXPORT GRect
	{
	private:
		G_INT32 _x1; // minimum x (left)
		G_INT32 _y1; // minimum y (bottom)
		G_INT32 _x2; // maximum x (right)
		G_INT32 _y2; // maximum y (top)

	public:
		// Constructs a invalid geo-rectangle.
		GRect();
		// Constructs a geo-rectangle with bottomLeft as the 
		// bottom left corner and topRight as the top right corner.
		GRect(const GPoint &bottomLeft, const GPoint &topRight);
		// Constructs a geo-rectangle with the bottom left corner 
		// and width and height.
		GRect(G_INT32 x, G_INT32 y, G_INT32 width, G_INT32 height);

		G_INT32 left() const;
		G_INT32 top() const;
		G_INT32 right() const;
		G_INT32 bottom() const;

		G_INT32 width() const;
		G_INT32 height() const;

		// Returns the center point of the geo-rectangle.
		GPoint center() const;

		bool isValid() const;

		void setLeft(G_INT32);
		void setTop(G_INT32);
		void setRight(G_INT32);
		void setBottom(G_INT32);

		GPoint topLeft() const;
		GPoint topRight() const;
		GPoint bottomLeft() const;
		GPoint bottomRight() const;

		// Sets the coordinates of the geo-rectangle's bottom left 
		// corner to (x1, y1), and the coordinates of its top right 
		// corner to (x2, y2).
		void setCoords(G_INT32 x1, G_INT32 y1, G_INT32 x2, G_INT32 y2);
		// Sets the coordinates of the geo-rectangle's bottom left 
		// corner to (x, y), and its size to (w, h).
		void setRect(G_INT32 x, G_INT32 y, G_INT32 w, G_INT32 h);

		bool contains(const GPoint &p) const;
		bool contains(G_INT32 x, G_INT32 y) const;
		bool contains(const GRect &r) const;
		bool intersects(const GRect &r) const;

		GRect operator|(const GRect &r) const;
		GRect operator&(const GRect &r) const;
		GRect &operator|=(const GRect &r);
		GRect &operator&=(const GRect &r);

		GRect unite(const GRect &r) const;
		GRect intersect(const GRect &r) const;

		// Expands the geo-rectangle by dx and dy. (Positive value results in shrinking.)
		void expand(G_INT32 dx, G_INT32 dy);
		// Moves the geo-rectangle dx along the X axis and dy along the Y axis, 
		// relative to the current position.
		void moveBy(G_INT32 dx, G_INT32 dy);

		bool operator==(const GRect &r);
		bool operator!=(const GRect &r);
	};

	// GRect inline functions

	inline GRect::GRect()
		: _x1(G_MAXV), _y1(G_MAXV), _x2(G_MINV), _y2(G_MINV) {}

	inline GRect::GRect(const GPoint &bottomLeft, const GPoint &topRight)
		: _x1(bottomLeft.x()), _y1(bottomLeft.y()), _x2(topRight.x()), _y2(topRight.y()) {}

	inline GRect::GRect(G_INT32 x, G_INT32 y, G_INT32 width, G_INT32 height)
		: _x1(x), _y1(y), _x2(x + width - 1), _y2(y + height - 1) {}

	inline G_INT32 GRect::left() const
	{ return _x1; }

	inline G_INT32 GRect::top() const
	{ return _y2; }

	inline G_INT32 GRect::right() const
	{ return _x2; }

	inline G_INT32 GRect::bottom() const
	{ return _y1; }

	inline G_INT32 GRect::width() const
	{ return _x2 - _x1 + 1; }

	inline G_INT32 GRect::height() const
	{ return _y2 - _y1 + 1; }

	inline bool GRect::isValid() const
	{ return _x2 >= _x1 && _y2 >= _y1; }

	inline GPoint GRect::center() const
	{ return GPoint((_x1 + _x2) / 2, (_y1 + _y2) / 2); }

	inline void GRect::setLeft(G_INT32 pos)
	{ _x1 = pos; }

	inline void GRect::setTop(G_INT32 pos)
	{ _y2 = pos; }

	inline void GRect::setRight(G_INT32 pos)
	{ _x2 = pos; }

	inline void GRect::setBottom(G_INT32 pos)
	{ _y1 = pos; }

	inline GPoint GRect::topLeft() const
	{ return GPoint(_x1, _y2); }

	inline GPoint GRect::topRight() const
	{ return GPoint(_x2, _y2); }

	inline GPoint GRect::bottomLeft() const
	{ return GPoint(_x1, _y1); }

	inline GPoint GRect::bottomRight() const
	{ return GPoint(_x2, _y1); }

	inline bool GRect::contains(const GPoint &p) const
	{ return p.x() >= _x1 && p.x() <= _x2 && p.y() >= _y1 && p.y() <= _y2; }

	inline bool GRect::contains(G_INT32 x, G_INT32 y) const
	{ return x >= _x1 && x <= _x2 && y >= _y1 && y <= _y2; }

	inline bool GRect::contains(const GRect &r) const
	{ return r._x1 >= _x1 && r._x2 <= _x2 && r._y1 >= _y1 && r._y2 <= _y2; }

	inline bool GRect::intersects(const GRect &r) const
	{ return MAX(_x1, r._x1) <= MIN(_x2, r._x2) && MAX(_y1, r._y1) <= MIN(_y2, r._y2); }

	inline bool GRect::operator==(const GRect &r)
	{ return _x1 == r._x1 && _x2 == r._x2 && _y1 == r._y1 && _y2 == r._y2; }

	inline bool GRect::operator!=(const GRect &r)
	{ return _x1 != r._x1 || _x2 != r._x2 || _y1 != r._y1 || _y2 != r._y2; }
};

#endif 
