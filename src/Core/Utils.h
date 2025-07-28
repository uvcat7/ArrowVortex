#pragma once

#include <Core/Vector.h>

#include <stdlib.h>
#include <new>

namespace Vortex {

#define TT template <typename T>

// ================================================================================================
// General utility functions.

TT inline void ToggleFlags(T& bits, int flags) { bits ^= flags; }

TT inline void SetFlags(T& bits, int flags) { bits |= flags; }

TT inline void UnsetFlags(T& bits, int flags) { bits &= ~flags; }

TT inline void SetFlags(T& bits, int flags, bool state) {
    bits = (bits & ~flags) | (flags * state);
}

TT inline bool HasFlags(T bits, int flag) { return (bits & flag) == flag; }

TT inline bool HasLength(const vec2t<T>& v) {
    return v.x * v.x + v.y * v.y > 0.001f;
}

TT inline const T& min(const T& a, const T& b) { return (a < b) ? a : b; }

TT inline const T& max(const T& a, const T& b) { return (b < a) ? a : b; }

TT inline const T& clamp(const T& x, const T& min, const T& max) {
    return (x < min) ? min : ((max < x) ? max : x);
}

TT inline void swapValues(T& a, T& b) {
    T c(a);
    a = b;
    b = c;
}

TT inline T lerp(T begin, T end, T t) { return begin + (end - begin) * t; }

// ================================================================================================
// Rect operations.

TT inline rectt<T> operator+(const rectt<T>& r, const vec2t<T>& v) {
    return {r.x + v.x, r.y + v.y, r.w, r.h};
}

TT inline rectt<T> operator-(const rectt<T>& r, const vec2t<T>& v) {
    return {r.x - v.x, r.y - v.y, r.w, r.h};
}

TT inline void operator+=(rectt<T>& r, const vec2t<T>& v) {
    r.x += v.x, r.y += v.y;
}

TT inline void operator-=(rectt<T>& r, const vec2t<T>& v) {
    r.x -= v.x, r.y -= v.y;
}

TT inline bool IsInside(const rectt<T>& r, T x, T y) {
    return (x >= r.x && y >= r.y && x < r.x + r.w && y < r.y + r.h);
}

TT inline T CenterX(const rectt<T>& r) { return r.x + r.w / 2; }

TT inline T CenterY(const rectt<T>& r) { return r.y + r.h / 2; }

TT inline T RightX(const rectt<T>& r) { return r.x + r.w; }

TT inline T BottomY(const rectt<T>& r) { return r.y + r.h; }

TT inline vec2t<T> Pos(const rectt<T>& r) { return {r.x, r.y}; }

TT inline vec2t<T> Size(const rectt<T>& r) { return {r.w, r.h}; }

TT inline rectt<T> SideT(const rectt<T>& r, T h) { return {r.x, r.y, r.w, h}; }

TT inline rectt<T> SideL(const rectt<T>& r, T w) { return {r.x, r.y, w, r.h}; }

TT inline rectt<T> SideB(const rectt<T>& r, T h) {
    return {r.x, r.y + r.h - h, r.w, h};
}

TT inline rectt<T> SideR(const rectt<T>& r, T w) {
    return {r.x + r.w - w, r.y, w, r.h};
}

TT inline rectt<T> CornerTL(const rectt<T>& r, T w, T h) {
    return {r.x, r.y, w, h};
}

TT inline rectt<T> CornerTR(const rectt<T>& r, T w, T h) {
    return {r.x + r.w - w, r.y, w, h};
}

TT inline rectt<T> CornerBL(const rectt<T>& r, T w, T h) {
    return {r.x, r.y + r.h - h, w, h};
}

TT inline rectt<T> CornerBR(const rectt<T>& r, T w, T h) {
    return {r.x + r.w - w, r.y + r.h - h, w, h};
}

TT inline rectt<T> Shrink(const rectt<T>& r, T m) {
    return {r.x + m, r.y + m, r.w - m * 2, r.h - m * 2};
}

TT inline rectt<T> Shrink(const rectt<T>& v, T l, T t, T r, T b) {
    return {v.x + l, v.y + t, v.w - l - r, v.h - t - b};
}

TT inline rectt<T> Expand(const rectt<T>& r, T m) {
    return {r.x - m, r.y - m, r.w + m * 2, r.h + m * 2};
}

TT inline rectt<T> Expand(const rectt<T>& v, T l, T t, T r, T b) {
    return {v.x - l, v.y - t, v.w + l + r, v.h + t + b};
}

TT inline void SetSize(rectt<T>& r, const vec2t<T>& s) { r.w = s.x, r.h = s.y; }

TT inline void SetPos(rectt<T>& r, const vec2t<T>& p) { r.x = p.x, r.y = p.y; }

// ================================================================================================
// Vec2 operations.

TT inline vec2t<T> operator+(const vec2t<T>& a, const vec2t<T>& b) {
    return {a.x + b.x, a.y + b.y};
}

TT inline vec2t<T> operator-(const vec2t<T>& a, const vec2t<T>& b) {
    return {a.x - b.x, a.y - b.y};
}

TT inline vec2t<T> operator*(T f, const vec2t<T>& v) {
    return {v.x * f, v.y * f};
}

TT inline vec2t<T> operator*(const vec2t<T>& v, T f) {
    return {v.x * f, v.y * f};
}

TT inline bool operator==(const vec2t<T>& a, const vec2t<T>& b) {
    return a.x == b.x && a.y == b.y;
}

TT inline bool operator!=(const vec2t<T>& a, const vec2t<T>& b) {
    return a.x != b.x || a.y != b.y;
}

TT inline void operator+=(const vec2t<T>& a, const vec2t<T>& b) {
    a.x += b.x, a.y += b.y;
}

TT inline vec2t<T> ToVec2(T x, T y) { return {x, y}; }

TT inline areat<T> ToArea(rectt<T> r) {
    return {r.x, r.y, r.x + r.w, r.y + r.h};
}

TT inline rectt<T> ToRect(areat<T> a) {
    return {a.l, a.t, a.r - a.l, a.b - a.t};
}

TT inline vec2t<int> ToVec2i(const vec2t<T>& v) { return {(int)v.x, (int)v.y}; }

TT inline vec2t<float> ToVec2f(const vec2t<T>& v) {
    return {(float)v.x, (float)v.y};
}

#undef TT

// ================================================================================================
// Color operations.

inline colorf operator+(const colorf& a, const colorf& b) {
    return {a.r + b.r, a.g + b.g, a.b + b.b, a.a};
}

inline colorf operator-(const colorf& a, const colorf& b) {
    return {a.r - b.r, a.g - b.g, a.b - b.b, a.a};
}

inline colorf operator*(float f, const colorf& c) {
    return {c.r * f, c.g * f, c.b * f, c.a};
}

inline colorf operator*(const colorf& c, float f) {
    return {c.r * f, c.g * f, c.b * f, c.a};
}

inline bool operator==(const colorf& a, const colorf& b) {
    return (a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a);
}

inline bool operator!=(const colorf& a, const colorf& b) { return !(a == b); }

inline colorf ToColor(float l, float a = 1.0f) { return {l, l, l, a}; }

inline colorf ToColorf(uint32_t color) {
    union {
        uint8_t c[4];
        uint32_t c32;
    };
    c32 = color;
    return {c[0] / 255.0f, c[1] / 255.0f, c[2] / 255.0f, c[3] / 255.0f};
}

inline uint32_t ToColor32(const colorf& c) {
    union {
        uint8_t u8[4];
        uint32_t u32;
    };
    u8[0] = (uint8_t)min(max(0, (int)(c.r * 255.0f)), 255);
    u8[1] = (uint8_t)min(max(0, (int)(c.g * 255.0f)), 255);
    u8[2] = (uint8_t)min(max(0, (int)(c.b * 255.0f)), 255);
    u8[3] = (uint8_t)min(max(0, (int)(c.a * 255.0f)), 255);
    return u32;
}

inline uint32_t ToColor32(float r, float g, float b, float a) {
    union {
        uint8_t u8[4];
        uint32_t u32;
    };
    u8[0] = (uint8_t)min(max(0, (int)(r * 255.0f)), 255);
    u8[1] = (uint8_t)min(max(0, (int)(g * 255.0f)), 255);
    u8[2] = (uint8_t)min(max(0, (int)(b * 255.0f)), 255);
    u8[3] = (uint8_t)min(max(0, (int)(a * 255.0f)), 255);
    return u32;
}

static int gcd(int a, int b) {
    if (a == 0) {
        return b;
    }
    if (b == 0) {
        return a;
    }
    if (a > b) {
        return gcd(a - b, b);
    } else {
        return gcd(a, b - a);
    }
}
};  // namespace Vortex

#undef TT