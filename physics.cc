#pragma once
#include "gpu_graphics/draw.cc"

using Box_Collider2D = Rect<f32>;

bool is_contained(Box_Collider2D *c, vec2f p) {
    return is_contained(*c, p);
}

struct Sphere_Collider2D {
    vec2f origin;
    f32 radius;
};

bool is_contained(Sphere_Collider2D *c, vec2f p) {
    vec2f delta = c->origin - p;
    f32 rsum = c->radius;
    return (delta.x * delta.x + delta.y * delta.y <= rsum * rsum);
}

enum struct Collider_Type {
    Box_Collider2D, Sphere_Collider2D, count
};

struct Collider {
    Collider_Type type;
    union {
        Box_Collider2D box_collider2d;
        Sphere_Collider2D sphere_collider2d;
    };
};

bool is_contained(Collider *c, vec2f p) {
    if (c->type == Collider_Type::Box_Collider2D) {
        return is_contained(&c->box_collider2d, p);
    } else if (c->type == Collider_Type::Sphere_Collider2D) {
        return is_contained(&c->sphere_collider2d, p);
    }    
    return false;
}

bool do_go_away_from_each_other(vec2f p1, vec2f p2, vec2f vel1, vec2f vel2) {
    vec2f dir = p2 - p1;
    return (dot(dir, vel1) < 0 && dot(dir, vel2) > 0);
}

bool do_collide(Box_Collider2D *c1, Box_Collider2D *c2) {
    // if (do_go_away_from_each_other(center(*c1), center(*c2),))
    return do_intersect(*c1, *c2);
}

bool do_collide(Sphere_Collider2D *c1, Sphere_Collider2D *c2) {
    vec2f delta = c2->origin - c1->origin;
    f32 rsum = c1->radius + c2->radius;
    return (magnitude(delta) <= rsum);
}

bool do_collide(Box_Collider2D *c1, Sphere_Collider2D *c2) {
    vec2f lb = c1->lb;
    vec2f rt = c1->rt;
    vec2f lt = { lb.x, rt.y };
    vec2f rb = { rt.x, lb.y };

    // cld - closest point
    f32 d1 = magnitude(closest_point_segment(lb, lt, c2->origin) - c2->origin);
    f32 d2 = magnitude(closest_point_segment(rb, rt, c2->origin) - c2->origin);
    f32 d3 = magnitude(closest_point_segment(lb, rb, c2->origin) - c2->origin);
    f32 d4 = magnitude(closest_point_segment(lt, rt, c2->origin) - c2->origin);

    f32 d_min = min(min(d1, d2), min(d3, d4));
    return (d_min <= c2->radius);
}

bool do_collide(Sphere_Collider2D *c1, Box_Collider2D *c2) {
    return do_collide(c2, c1);
}

bool do_collide(Collider *c1, Collider *c2) {
    if (c1->type == Collider_Type::Box_Collider2D) {
        if (c2->type == Collider_Type::Box_Collider2D) {
            return do_collide(&c1->box_collider2d, &c2->box_collider2d);
        } else if (c2->type == Collider_Type::Sphere_Collider2D) {
            return do_collide(&c1->box_collider2d, &c2->sphere_collider2d);
        }    
    } else if (c1->type == Collider_Type::Sphere_Collider2D) {
        if (c2->type == Collider_Type::Box_Collider2D) {
            return do_collide(&c1->sphere_collider2d, &c2->box_collider2d);
        } else if (c2->type == Collider_Type::Sphere_Collider2D) {
            return do_collide(&c1->sphere_collider2d, &c2->sphere_collider2d);
        }    
    }    
    return false;
}
