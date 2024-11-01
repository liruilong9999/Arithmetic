#ifndef TRIGONF_H
#define TRIGONF_H

#include "geo_gloabal.h"

namespace Geo
{
    typedef long trigonf_t; // Trigonometric function value type

    class GEO_EXPORT Trigonf
    {
    private:
        static trigonf_t _sinv[];
        static trigonf_t _cosv[];
        static trigonf_t _tanv[];

    private:
        static int bsearch_asc(trigonf_t x, const trigonf_t *arr, int sz);
        static int bsearch_desc(trigonf_t x, const trigonf_t *arr, int sz);

    public:
        const static trigonf_t MULTIPLER;

        // Returns the sine, cosine and tangent of x, 
        // where x is given in degrees defined to be between 0 and 360 (NOT inclusive).
        static trigonf_t sin(int x);
        static trigonf_t cos(int x);
        static trigonf_t tan(int x);

        // Returns the arc sine, cosine and tangent in degrees 
        // and the value is defined to be 
        //   asin: between -90 and  90 (inclusive).
        //   acos: between   0 and 180 (inclusive).
        //   atan: between -90 and  90 (inclusive).
        static int asin(trigonf_t x);
        static int acos(trigonf_t x);
        static int atan(trigonf_t x);

        // The function calculates the arc tangent of the two variables x and y. 
        // It is similar to calculating the arc tangent of y / x, except that 
        // the signs of both arguments are used to determine the quadrant of the result.
        // Returns the result in degrees, which is between 0 and 360 (NOT inclusive).
        static int atan2(int y, int x);

        // The function likes atan2, but calculates the PERPENDICULAR of arc tangent 
        // of the two variables x and y. 
        // Returns the result in degrees, which is between 0 and 360 (NOT inclusive).
        static int atan_p(int y, int x);
    };
};

#endif
