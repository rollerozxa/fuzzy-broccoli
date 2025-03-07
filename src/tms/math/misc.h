#ifndef _MATH_MISC__H_
#define _MATH_MISC__H_

#include <stdlib.h>
#include <math.h>

#ifdef __cplusplus
extern "C"
{
#endif

static inline float tclampf(float d, float min, float max)
{
    const float t = d < min ? min : d;
    return t > max ? max : t;
}

static inline int tclampi(int d, int min, int max)
{
    const int t = d < min ? min : d;
    return t > max ? max : t;
}

static inline float tmath_adist(float a, float b)
{
    const float PI2 = M_PI*2.f;

    int i, x;
    float d, dn;
    float n = fmod(b, PI2);
    float m = roundf(a / PI2) * PI2;

    float t[3] = {
        m + n,
        m + n - PI2,
        m + n + PI2
    };

    for (x=1, dn = fabsf(a-t[0]), i=0; x<3; x++) {
        if ((d = fabsf(a-t[x])) < dn) {
            i = x;
            dn = d;
        }
    }

    return t[i]-a;
}

#ifdef TMS_FAST_MATH

void tmath_sincos(float x, float *r0, float *r1);
float tmath_atan2(float y, float x);
float tmath_sqrt(float x);

#else

#define tmath_sincos(x,y,z) sincosf(x,y,z)
#define tmath_atan2(y,x) atan2f(y,x)
#define tmath_sqrt(x) sqrtf(x)

#endif

#ifdef __cplusplus
}
#endif

#endif
