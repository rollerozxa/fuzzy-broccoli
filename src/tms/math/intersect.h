
#ifndef _TMATH_INTERSECT__H_
#define _TMATH_INTERSECT__H_

/** @relates tintersect @{ */

#include <tms/math/vector.h>

/**
 * Misc intersection functions.
 * The struct tintersect does not exist, all intersection functions take various arguments.
 **/
struct tintersect {
#if defined(__clang__) || defined(_MSC_VER)
    int dummy123;
#endif
}; /* only for doc purposes */

int tintersect_ray_plane(tvec3 *origin, tvec3 *dir, tvec4 *plane, tvec3 *intersection);

static inline float
tintersect_segment_point_distance(tvec2 v, tvec2 w, tvec2 p)
{
    float l2, t;
    tvec2 pr;

    l2 = tvec2_distsq(w, v);
    if (l2 == 0.0) return tvec2_dist(p, v);
    t = (tvec2_dot(tvec2_sub(p, v), tvec2_sub(w,v)) / l2);
    if (t < 0.0) return tvec2_dist(p, v);
    else if (t > 1.0) return tvec2_dist(p, w);
    pr = tvec2_add(v, tvec2_mul(tvec2_sub(w, v), t));
    return tvec2_dist(p, pr);
}

static inline float
tintersect_point_poly_distance(tvec2 *p, tvec2 *verts, int num_verts)
{
    float n_dist = INFINITY;

    for (int x=0; x<num_verts; x++) {
        float dist = tintersect_segment_point_distance(verts[x], verts[(x+1)%num_verts], *p);
        if (dist < n_dist)
            n_dist = dist;
    }

    return n_dist;
}

#endif
