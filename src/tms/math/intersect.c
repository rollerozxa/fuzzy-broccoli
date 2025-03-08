
#include <math.h>

#include "intersect.h"


/**
 * Intersect a ray with a plane.
 *
 * @relates tintersect
 **/
int
tintersect_ray_plane(tvec3 *origin, tvec3 *dir,
                     tvec4 *plane, tvec3 *intersection)
{
    float d = tvec3_dot(dir, (tvec3*)plane);
    if (fabsf(d)>0.00001f) {
        float t = -(tvec3_dot(origin, (tvec3*)plane) + plane->w)/d;
        if (t<0.f) return 0;
        intersection->x = origin->x + (dir->x*t);
        intersection->y = origin->y + (dir->y*t);
        intersection->z = origin->z + (dir->z*t);
        return 1;
    } else {
        /* TODO: check if the origin is in the plane */
    }
    return 0;
}
