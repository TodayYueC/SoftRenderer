#ifndef __GEOMETRY_H__
#define __GEOMETRY_H__

#include <cmath>

// ========================== Vec2 ==========================

template <class T>
struct Vec2 {
    T x, y;
    Vec2() : x(0), y(0) {}
    Vec2(T _x, T _y) : x(_x), y(_y) {}
    template <class U> Vec2(const Vec2<U> &v) : x(v.x), y(v.y) {}

    Vec2<T> operator+(const Vec2<T> &V) const { return Vec2<T>(x + V.x, y + V.y); }
    Vec2<T> operator-(const Vec2<T> &V) const { return Vec2<T>(x - V.x, y - V.y); }
    Vec2<T> operator*(float scalar)    const { return Vec2<T>(x * scalar, y * scalar); }
    T& operator[](const int i)       { return i == 0 ? x : y; }
    const T& operator[](const int i) const { return i == 0 ? x : y; }
};

typedef Vec2<int>   Vec2i;
typedef Vec2<float> Vec2f;

// ========================== Vec3 ==========================

template <class T>
struct Vec3 {
    T x, y, z;
    Vec3() : x(0), y(0), z(0) {}
    Vec3(T _x, T _y, T _z) : x(_x), y(_y), z(_z) {}
    template <class U> Vec3(const Vec3<U> &v) : x(v.x), y(v.y), z(v.z) {}

    Vec3<T> operator+(const Vec3<T> &V) const { return Vec3<T>(x + V.x, y + V.y, z + V.z); }
    Vec3<T> operator-(const Vec3<T> &V) const { return Vec3<T>(x - V.x, y - V.y, z - V.z); }
    Vec3<T> operator*(float scalar)    const { return Vec3<T>(x * scalar, y * scalar, z * scalar); }
    Vec3<T> operator^(const Vec3<T> &V) const { return Vec3<T>(y * V.z - z * V.y, z * V.x - x * V.z, x * V.y - y * V.x); } // cross product
    T operator*(const Vec3<T> &V)       const { return x * V.x + y * V.y + z * V.z; } // dot product
    float norm()                        const { return std::sqrt(x * x + y * y + z * z); }
    Vec3<T>& normalize() {
        float n = norm();
        if (n != 0) { x /= n; y /= n; z /= n; }
        return *this;
    }
    T& operator[](const int i)       { return i == 0 ? x : (i == 1 ? y : z); }
    const T& operator[](const int i) const { return i == 0 ? x : (i == 1 ? y : z); }
};

typedef Vec3<int>   Vec3i;
typedef Vec3<float> Vec3f;

#endif //__GEOMETRY_H__
