#pragma once

namespace guinevere::gfx {

struct Rect {
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;
};

inline Rect intersect(Rect a, Rect b)
{
    const float x0 = (a.x > b.x) ? a.x : b.x;
    const float y0 = (a.y > b.y) ? a.y : b.y;

    const float ax1 = a.x + a.w;
    const float ay1 = a.y + a.h;
    const float bx1 = b.x + b.w;
    const float by1 = b.y + b.h;

    const float x1 = (ax1 < bx1) ? ax1 : bx1;
    const float y1 = (ay1 < by1) ? ay1 : by1;

    Rect r;
    r.x = x0;
    r.y = y0;
    r.w = x1 - x0;
    r.h = y1 - y0;

    if(r.w < 0.0f) {
        r.w = 0.0f;
    }

    if(r.h < 0.0f) {
        r.h = 0.0f;
    }

    return r;
}

inline bool is_empty(Rect r)
{
    return r.w <= 0.0f || r.h <= 0.0f;
}

} // namespace guinevere::gfx
