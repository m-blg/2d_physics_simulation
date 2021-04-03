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

struct Physics_Data {
    f32 mass;
    vec2f velocity;
    bool is_static;
};

struct Physics_Object {
    const char* name;
    Transform transform;
    Collider collider;
    Physics_Data physics_data;

    Material_Sprite2D material;
};

darr<Physics_Object> quads;




bool do_go_away_from_each_other(vec2f p1, vec2f p2, vec2f vel1, vec2f vel2) {
    vec2f dir = p2 - p1;
    return (dot(dir, vel1) <= 0 && dot(dir, vel2) >= 0);
}

bool do_collide(Box_Collider2D *c1, Box_Collider2D *c2) {
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

// bool do_collide(Collider *c1, Collider *c2) {
//     if (c1->type == Collider_Type::Box_Collider2D) {
//         if (c2->type == Collider_Type::Box_Collider2D) {
//             return do_collide(&c1->box_collider2d, &c2->box_collider2d);
//         } else if (c2->type == Collider_Type::Sphere_Collider2D) {
//             return do_collide(&c1->box_collider2d, &c2->sphere_collider2d);
//         }    
//     } else if (c1->type == Collider_Type::Sphere_Collider2D) {
//         if (c2->type == Collider_Type::Box_Collider2D) {
//             return do_collide(&c1->sphere_collider2d, &c2->box_collider2d);
//         } else if (c2->type == Collider_Type::Sphere_Collider2D) {
//             return do_collide(&c1->sphere_collider2d, &c2->sphere_collider2d);
//         }    
//     }    
//     return false;
// }


Collider world_space_collider(Physics_Object *po) {
    Collider c;
    c.type = po->collider.type;

    mat4f model_m1 = model_matrix(&po->transform);
    switch (po->collider.type) {
        case Collider_Type::Box_Collider2D: 
        {
            Box_Collider2D& po_col = po->collider.box_collider2d;

            c.box_collider2d = { (vec2f)(model_m1 * vec4f(po_col.lb, 0, 1)), 
                (vec2f)(model_m1 * vec4f(po_col.rt, 0, 1)) }; 
        } break;
        case Collider_Type::Sphere_Collider2D:
        {
            Sphere_Collider2D& po_col = po->collider.sphere_collider2d;

            c.sphere_collider2d = { (vec2f)(model_m1 * vec4f(po_col.origin, 0, 1)), po_col.radius * max(po->transform.scale.x, po->transform.scale.y) };
            
        } break;
    }

    return c;
}

Box_Collider2D world_space_collider_b(Physics_Object *obj) {
    mat4f model_m1 = model_matrix(&obj->transform);
    Box_Collider2D& bc = obj->collider.box_collider2d;
    return { (vec2f)(model_m1 * vec4f(bc.lb, 0, 1)), 
        (vec2f)(model_m1 * vec4f(bc.rt, 0, 1)) };
}

Sphere_Collider2D world_space_collider_s(Physics_Object *obj) {
    mat4f model_m1 = model_matrix(&obj->transform);
    Sphere_Collider2D& sc = obj->collider.sphere_collider2d;
    return { (vec2f)(model_m1 * vec4f(sc.origin, 0, 1)), 
        sc.radius * max(obj->transform.scale.x, obj->transform.scale.y) };
}


vec2f min_vec(vec2f v1, vec2f v2) {
    return (magnitude(v1) < magnitude(v2) ? v1 : v2);
}

vec2f max_vec(vec2f v1, vec2f v2) {
    return (magnitude(v1) > magnitude(v2) ? v1 : v2);
}

darr<Collider> world_space_collider_cache;

void update_world_space_collider_cache() {
    if (world_space_collider_cache.cap != quads.cap) {
        world_space_collider_cache.buffer = m_ralloc(world_space_collider_cache.buffer, quads.cap);
        world_space_collider_cache.cap = quads.cap;
    }
    world_space_collider_cache.len = quads.len;

    auto cache_it = begin(&world_space_collider_cache);
    for (auto it = begin(&quads); it != end(&quads); it++, cache_it++) {
        *cache_it = world_space_collider(it);
    }
}

void resolve_collision_bb(Physics_Object* b1, Physics_Object *b2, Collider *c1, Collider *c2) {
    Box_Collider2D& bc1 = c1->box_collider2d;
    Box_Collider2D& bc2 = c2->box_collider2d;

    // if (do_go_away_from_each_other(centerof(bc1), centerof(bc2), b1->physics_data.velocity, b2->physics_data.velocity)) return;
    if (!do_collide(&bc1, &bc2)) return;

    Physics_Data& data1 = b1->physics_data;
    Physics_Data& data2 = b2->physics_data;
    // vec2f new_velocity1 = ( data1.velocity * (data1.mass - data2.mass) + 2 * data2.mass * data2.velocity ) / (data1.mass + data2.mass);
    // vec2f new_velocity2 = ( data2.velocity * (data2.mass - data1.mass) + 2 * data1.mass * data1.velocity ) / (data1.mass + data2.mass);

    vec2f delta;

    // find the direction boxes have collided
    vec2f size1 = bc1.rt - bc1.lb;
    vec2f size2 = bc2.rt - bc2.lb;
    vec2f dir = centerof(bc2) - centerof(bc1);
    if (abs(dir.x) - (size1.x + size2.x) / 2 > abs(dir.y) - (size1.y + size2.y) / 2) {
        if (dir.x > 0) {
            delta = { -1, 0 };
        } else {
            delta = { 1, 0 };
        }
    } else {
        if (dir.y > 0) {
            delta = { 0, -1 };
        } else {
            delta = { 0, 1 };  
        }
    }
    // colliders stuck in each other bug fix (sort of)
    if (do_go_away_from_each_other({0, 0}, delta, b2->physics_data.velocity, b1->physics_data.velocity)) return;

    f32 sm_delta = delta.x * delta.x + delta.y * delta.y;

    vec2f new_velocity1 = data1.velocity;
    vec2f new_velocity2 = data2.velocity;
    if (!data1.is_static && !data2.is_static) {
        new_velocity1 = data1.velocity - 2 * (data2.mass / (data1.mass + data2.mass)) *  
            dot(data1.velocity - data2.velocity, delta) * delta / sm_delta;

        delta = -delta;
        new_velocity2 = data2.velocity - 2 * (data1.mass / (data1.mass + data2.mass)) *  
            dot(data2.velocity - data1.velocity, delta) * delta / sm_delta;
    } else {
        if (!data2.is_static) {
            delta = -delta;
            new_velocity2 = data2.velocity - 2 * dot(data2.velocity - data1.velocity, delta) * delta / sm_delta;
        } else if (!data1.is_static) {
            new_velocity1 = data1.velocity - 2 * dot(data1.velocity - data2.velocity, delta) * delta / sm_delta;
        }            
    }

    data1.velocity = new_velocity1;
    data2.velocity = new_velocity2;
}
void resolve_collision_ss(Physics_Object* s1, Physics_Object *s2, Collider *c1, Collider *c2) {
    Sphere_Collider2D& sc1 = c1->sphere_collider2d;
    Sphere_Collider2D& sc2 = c2->sphere_collider2d;

    // colliders stuck in each other bug fix (sort of)
    if (do_go_away_from_each_other(sc1.origin, sc2.origin, s1->physics_data.velocity, s2->physics_data.velocity)) return;
    if (!do_collide(&sc1, &sc2)) return;

    Physics_Data& data1 = s1->physics_data;
    Physics_Data& data2 = s2->physics_data;

    vec2f delta = sc1.origin - sc2.origin;
    f32 sm_delta = delta.x * delta.x + delta.y * delta.y;

    vec2f new_velocity1 = data1.velocity;
    vec2f new_velocity2 = data2.velocity;
    if (!data1.is_static && !data2.is_static) {
        new_velocity1 = data1.velocity - 2 * (data2.mass / (data1.mass + data2.mass)) *  
            dot(data1.velocity - data2.velocity, delta) * delta / sm_delta;

        delta = -delta;
        new_velocity2 = data2.velocity - 2 * (data1.mass / (data1.mass + data2.mass)) *  
            dot(data2.velocity - data1.velocity, delta) * delta / sm_delta;
    } else {
        if (!data2.is_static) {
            delta = -delta;
            new_velocity2 = data2.velocity - 2 * dot(data2.velocity - data1.velocity, delta) * delta / sm_delta;
        } else if (!data1.is_static) {
            new_velocity1 = data1.velocity - 2 * dot(data1.velocity - data2.velocity, delta) * delta / sm_delta;
        }            
    }

    data1.velocity = new_velocity1;
    data2.velocity = new_velocity2;
}


void resolve_collision_bs(Physics_Object* b, Physics_Object *s, Collider *c1, Collider *c2) {
    Box_Collider2D& bc = c1->box_collider2d; 
    Sphere_Collider2D& sc = c2->sphere_collider2d;

    if (!do_collide(&bc, &sc)) return;

    vec2f lb = bc.lb;
    vec2f rt = bc.rt;
    vec2f lt = { lb.x, rt.y };
    vec2f rb = { rt.x, lb.y };

    vec2f delta1 = closest_point_segment(lb, lt, sc.origin) - sc.origin;
    vec2f delta2 = closest_point_segment(rb, rt, sc.origin) - sc.origin;
    vec2f delta3 = closest_point_segment(lb, rb, sc.origin) - sc.origin;
    vec2f delta4 = closest_point_segment(lt, rt, sc.origin) - sc.origin;

    vec2f delta = min_vec(min_vec(delta1, delta2), min_vec(delta3, delta4));
    // colliders stuck in each other bug fix (sort of)
    if (do_go_away_from_each_other({0, 0}, delta, s->physics_data.velocity, b->physics_data.velocity)) return;

    Physics_Data& data1 = b->physics_data;
    Physics_Data& data2 = s->physics_data;

    f32 sm_delta = delta.x * delta.x + delta.y * delta.y;

    vec2f new_velocity1 = data1.velocity;
    vec2f new_velocity2 = data2.velocity;
    if (!data1.is_static && !data2.is_static) {
        new_velocity1 = data1.velocity - 2 * (data2.mass / (data1.mass + data2.mass)) *  
            dot(data1.velocity - data2.velocity, delta) * delta / sm_delta;

        delta = -delta;
        new_velocity2 = data2.velocity - 2 * (data1.mass / (data1.mass + data2.mass)) *  
            dot(data2.velocity - data1.velocity, delta) * delta / sm_delta;
    } else {
        if (!data2.is_static) {
            delta = -delta;
            new_velocity2 = data2.velocity - 2 * dot(data2.velocity - data1.velocity, delta) * delta / sm_delta;
        } else if (!data1.is_static) {
            new_velocity1 = data1.velocity - 2 * dot(data1.velocity - data2.velocity, delta) * delta / sm_delta;
        }            
    }

    data1.velocity = new_velocity1;
    data2.velocity = new_velocity2;
}

void resolve_collision_sb(Physics_Object *obj1, Physics_Object *obj2, Collider *c1, Collider *c2) {
    resolve_collision_bs(obj2, obj1, c2, c1);
}

void(*resolve_collision_matrix[(u32)Collider_Type::count][(u32)Collider_Type::count])(Physics_Object*, Physics_Object*, Collider*, Collider*) {
    { resolve_collision_bb, resolve_collision_bs },
    { resolve_collision_sb, resolve_collision_ss }
};

void resolve_collision(Physics_Object* obj1, Physics_Object *obj2, Collider *c1, Collider* c2) {
    resolve_collision_matrix[(u32)obj1->collider.type][(u32)obj2->collider.type](obj1, obj2, c1, c2);
}


Physics_Object* is_over(vec2f p) {
    f32 depth = INT_MIN;
    Physics_Object* po = null;
    for (auto it = begin(&quads); it != end(&quads); it++) {
        Collider c = world_space_collider(it);
        // vec2f local_p = p - vec2f(it->transform.position.x, it->transform.position.y);
        if (is_contained(&c, p) && it->transform.position.z > depth) {
            po = it;
            depth = it->transform.position.z;
        }
    }
    return po;
}

void apply_force(vec2f force, f32 dt) {

}

vec2f gravity = {0, -90.8f};

void apply_gravity() {
    
}

void physics_update() {
    // void apply_gravity();
    for (auto it = begin(&quads); it != end(&quads); it++) {
        // if (it->collider.type == Collider_Type::Sphere_Collider2D)
        //     it->physics_data.velocity += gravity * GTime::fixed_dt;
        it->transform.position += vec3f(it->physics_data.velocity, 0) * 0.1f * GTime::fixed_dt;
    }

    update_world_space_collider_cache();
    Collider* cache_it1 = begin(&world_space_collider_cache);
    for (auto it1 = begin(&quads); it1 != end(&quads); it1++, cache_it1++) {
        Collider* cache_it2 = cache_it1 + 1;
        for (auto it2 = it1 + 1; it2 != end(&quads); it2++, cache_it2++) {
            resolve_collision(it1, it2, cache_it1, cache_it2);
        }
    }
}

