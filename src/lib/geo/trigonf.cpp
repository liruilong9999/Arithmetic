#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <assert.h>

#include "trigonf.h"

using namespace Geo;

// Trigonf members

trigonf_t Trigonf::_sinv[91] = 
{
        0,  175,  349,  523,  698,  872, 1045, 1219, 1392, 1564, /*  0 */
     1736, 1908, 2079, 2250, 2419, 2588, 2756, 2924, 3090, 3256, /* 10 */
     3420, 3584, 3746, 3907, 4067, 4226, 4384, 4540, 4695, 4848, /* 20 */
     5000, 5150, 5299, 5446, 5592, 5736, 5878, 6018, 6157, 6293, /* 30 */
     6428, 6561, 6691, 6820, 6947, 7071, 7193, 7314, 7431, 7547, /* 40 */
     7660, 7771, 7880, 7986, 8090, 8192, 8290, 8387, 8480, 8572, /* 50 */
     8660, 8746, 8829, 8910, 8988, 9063, 9135, 9205, 9272, 9336, /* 60 */
     9397, 9455, 9511, 9563, 9613, 9659, 9703, 9744, 9781, 9816, /* 70 */
     9848, 9877, 9903, 9925, 9945, 9962, 9976, 9986, 9994, 9998, /* 80 */
    10000                                                        /* 90 */
};

trigonf_t Trigonf::_cosv[91] = 
{
    10000, 9998, 9994, 9986, 9976, 9962, 9945, 9925, 9903, 9877, 
     9848, 9816, 9781, 9744, 9703, 9659, 9613, 9563, 9511, 9455, 
     9397, 9336, 9272, 9205, 9135, 9063, 8988, 8910, 8829, 8746, 
     8660, 8572, 8480, 8387, 8290, 8192, 8090, 7986, 7880, 7771, 
     7660, 7547, 7431, 7314, 7193, 7071, 6947, 6820, 6691, 6561, 
     6428, 6293, 6157, 6018, 5878, 5736, 5592, 5446, 5299, 5150, 
     5000, 4848, 4695, 4540, 4384, 4226, 4067, 3907, 3746, 3584, 
     3420, 3256, 3090, 2924, 2756, 2588, 2419, 2250, 2079, 1908, 
     1736, 1564, 1392, 1219, 1045,  872,  698,  523,  349,  175, 
		0 
};

trigonf_t Trigonf::_tanv[91] = 
{
	    0,   175,   349,   524,   699,   875,    1051,   1228,   1405,   1584, 
     1763,  1944,  2126,  2309,  2493,   2679,   2867,   3057,   3249,   3443, 
     3640,  3839,  4040,  4245,  4452,   4663,   4877,   5095,   5317,   5543, 
     5774,  6009,  6249,  6494,  6745,   7002,   7265,   7536,   7813,   8098, 
     8391,  8693,  9004,  9325,  9657,  10000,  10355,  10724,  11106,  11504, 
    11918, 12349, 12799, 13270, 13764,  14281,  14826,  15399,  16003,  16643, 
    17321, 18040, 18807, 19626, 20503,  21445,  22460,  23559,  24751,  26051, 
    27475, 29042, 30777, 32709, 34874,  37321,  40108,  43315,  47046,  51446, 
    56713, 63138, 71154, 81443, 95144, 114301, 143007, 190811, 286363, 572900, 
    INT_MAX /* tan(PI) is undefined */
};

const trigonf_t Trigonf::MULTIPLER = 10000;

// binary searching in ascending order
int Trigonf::bsearch_asc(trigonf_t x, const trigonf_t *arr, int sz)
{
    int i = 0;
    int j = sz;
    int mid;

    while (j > i) {
        mid = (i + j) / 2;
        if (x > arr[mid])
            i = mid + 1;
        else if (x < arr[mid])
            j = mid;
        else 
            i = j = mid; // found x at position mid
    }

    return i;
}

// binary searching in descending order
int Trigonf::bsearch_desc(trigonf_t x, const trigonf_t *arr, int sz)
{
    int i = 0;
    int j = sz;
    int mid;

    while (j > i) {
        mid = (i + j) / 2;
        if (x > arr[mid])
            j = mid;
        else if (x < arr[mid])
            i = mid + 1;
        else
            i = j = mid; // found x at position mid
    }

    return i;
}

trigonf_t Trigonf::sin(int x)
{
    trigonf_t res;

    assert(x >= 0 && x < 360);

    if (x <= 90)
        res = _sinv[x];
    else if (x <= 180)
        res = _sinv[180 - x];
    else if (x <= 270)
        res = -_sinv[x - 180];
    else
        res = -_sinv[360 - x];

    return res;
}

trigonf_t Trigonf::cos(int x)
{
    trigonf_t res;

    assert(x >= 0 && x < 360);

    if (x <= 90)
        res = _cosv[x];
    else if (x <= 180)
        res = -_cosv[180 - x];
    else if (x <= 270)
        res = -_cosv[x - 180];
    else
        res = _cosv[360 - x];

    return res;
}

trigonf_t Trigonf::tan(int x)
{
    trigonf_t res;

    assert(x >= 0 && x < 360);
    assert(x != 90 && x != 270);

    if (x <= 90)
        res = _tanv[x];
    else if (x <= 180)
        res = -_tanv[180 - x];
    else if (x <= 270)
        res = _tanv[x - 180];
    else 
        res = -_tanv[360 -x ];

    return res;
}

int Trigonf::asin(trigonf_t x)
{
    assert(x >= -10000 && x <= 10000);

    trigonf_t ax = x;
    bool negative = false;
    if (x < 0) {
        ax = abs(x);
        negative = true;
    }

    int res = bsearch_asc(ax, _sinv, 91);
    return negative ? -res : res;
}

int Trigonf::acos(trigonf_t x)
{
    assert(x >= -10000 && x <= 10000);

    trigonf_t ax = x;
    bool negative = false;
    if (x < 0) {
        ax = abs(x);
        negative = true;
    }

    int res = bsearch_desc(ax, _cosv, 91);
    return negative ? 90 + res : res;
}

int Trigonf::atan(trigonf_t x)
{
    trigonf_t ax = x;
    bool negative = false;
    if (x < 0) {
        ax = abs(x);
        negative = true;
    }

    int res = bsearch_asc(ax, _tanv, 91);
    return negative ? -res : res;
}

int Trigonf::atan2(int y, int x)
{
    int res = 0;
    float ax = abs(x);
    float ay = abs(y);

    if (x >= 0 && y > 0)		// quadrant 1
        res = Trigonf::atan((int)(ax / ay * Trigonf::MULTIPLER));
    else if (x > 0 && y <= 0)	// quadrant 2
        res = Trigonf::atan((int)(ay / ax * Trigonf::MULTIPLER)) + 90;
    else if (x <= 0 && y < 0)	// quadrant 3
        res = Trigonf::atan((int)(ax / ay * Trigonf::MULTIPLER)) + 180;
    else if (x < 0 && y >= 0)	// quadrant 4
        res = Trigonf::atan((int)(ay / ax * Trigonf::MULTIPLER)) + 270;

    return res % 360;
}

int Trigonf::atan_p(int y, int x)
{
    int res = 0;
    float ax = abs(x);
    float ay = abs(y);

    if (x >= 0 && y > 0)		// quadrant 1
        res = Trigonf::atan((int)(ax / ay * Trigonf::MULTIPLER)) + 270;
    else if (x > 0 && y <= 0)	// quadrant 2
        res = Trigonf::atan((int)(ay / ax * Trigonf::MULTIPLER));
    else if (x <= 0 && y < 0)	// quadrant 3
        res = Trigonf::atan((int)(ax / ay * Trigonf::MULTIPLER)) + 90;
    else if (x < 0 && y >= 0)	// quadrant 4
        res = Trigonf::atan((int)(ay / ax * Trigonf::MULTIPLER)) + 180;

    return res % 360;
}
