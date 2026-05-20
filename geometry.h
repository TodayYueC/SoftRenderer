/**
 * @file geometry.h
 * @brief 软件渲染器用的轻量线性代数库 —— 向量与小型矩阵
 *
 * =================================================================
 *  使用说明
 * =================================================================
 *
 *  【类型别名】
 *    Vec2i / Vec2f   —— 二维向量（int / float）
 *    Vec3i / Vec3f   —— 三维向量（int / float）
 *    Vec4i / Vec4f   —— 四维向量（int / float），常用于齐次坐标
 *    Mat2 / Mat3 / Mat4 —— 2×2 / 3×3 / 4×4 浮点方阵
 *
 *  【向量运算一览】（以 Vec3f 为例，Vec2 / Vec4 同理）
 *
 *    Vec3f a(1, 2, 3), b(4, 5, 6);
 *
 *    a + b            // 向量加法
 *    a - b            // 向量减法
 *    a * 2.f          // 标量乘法（向量 * 标量）
 *    2.f * a          // 标量左乘（标量 * 向量）
 *    a / 2.f          // 标量除法
 *    -a               // 取反
 *    a += b;          // 复合赋值：+=  -=  *=  /=  均支持
 *    a ^ b            // 叉积（仅 Vec3 有）
 *    a * b            // 点积（内积）
 *    a.norm()         // 欧几里得范数（长度）
 *    a.normalize()    // 原地归一化，返回自身引用
 *    a[0] / a[1] / a[2]  // 按索引访问分量
 *    a == b / a != b  // 相等 / 不等比较
 *    std::cout << a;  // 输出，格式：(1, 2, 3)
 *
 *  【Vec4 与齐次坐标】
 *
 *    Vec3f p(1, 2, 3);
 *    Vec4f v4(p);         // 从 Vec3 构造，默认 w=1（表示点）
 *    Vec4f v4d(p, 0);     // w=0（表示方向向量）
 *    v4.xyz()             // 返回 Vec3f(1, 2, 3)，直接截取前三分量
 *    v4.toVec3()          // 执行透视除法：(x/w, y/w, z/w)
 *
 *  【矩阵运算一览】
 *
 *    Matrix<ROWS, COLS, T> 是通用矩阵模板，支持任意行列数。
 *    Mat2 / Mat3 / Mat4 只是 Matrix<2,2,float> 等的别名，用法完全一致。
 *
 *    // ---------- 创建与赋值 ----------
 *    Mat4 m;                               // 4×4 零矩阵
 *    m[0][0] = 1.f;  m[0][1] = 2.f;      // m[row][col] 按行、列索引访问
 *
 *    // 非方阵同样支持，只需指定行数和列数：
 *    Matrix<3, 2, float> m32;             // 3行2列零矩阵
 *    Matrix<2, 3, float> m23;             // 2行3列零矩阵
 *    Matrix<4, 1, float> m41;             // 4行1列零矩阵（列向量）
 *    m32[0][1] = 5.f;                    // 第0行第1列赋值
 *
 *    // 单位矩阵（仅方阵可用）
 *    Mat4 I4 = identity_matrix<4, float>();
 *    Mat3 I3 = identity_matrix<3, float>();
 *    Mat2 I2 = identity_matrix<2, float>();
 *
 *    // ---------- 矩阵 × 矩阵 ----------
 *    // A 的列数必须等于 B 的行数，否则编译报错
 *    Mat4 a, b;
 *    Mat4 c = a * b;                       // 4×4 × 4×4 → 4×4
 *
 *    // 非方阵乘法示例：
 *    Matrix<3, 2, float> A;               // 3×2
 *    Matrix<2, 4, float> B;               // 2×4
 *    Matrix<3, 4, float> C = A * B;       // 3×2 × 2×4 → 3×4
 *
 *    // ---------- 矩阵 × 向量 ----------
 *    Vec4f v4(1, 2, 3, 1);
 *    Vec3f v3(1, 2, 3);
 *    Vec2f v2(1, 2);
 *
*    Vec4f r4 = a * v4;                   // 4×4 矩阵 × Vec4
 *    Vec3f r3 = mat3 * v3;                // 3×3 矩阵 × Vec3
 *    Vec2f r2 = mat2 * v2;                // 2×2 矩阵 × Vec2
 *
 *    // 4×4 矩阵可直接乘 Vec3，自动视作齐次坐标 (x,y,z,1) 并执行透视除法
 *    Vec3f r = a * v3;                    // 等价于 (a * Vec4f(v3,1)).toVec3()
 *
 *    // 一般用法：齐次坐标变换后转回三维坐标
 *    Vec3f world_pos = (a * Vec4f(v3, 1)).toVec3();
 *
 *    // ---------- 转置 ----------
 *    // 行列互换：Matrix<M,N> → Matrix<N,M>
 *    Mat4 t = a.transpose();              // 4×4 → 4×4
 *    Matrix<3, 2, float> At = m32.transpose(); // 3×2 → 2×3
 *
 *    // ---------- 求逆（仅方阵） ----------
 *    Mat4 inv = inverse(a);                // 若矩阵奇异，返回零矩阵
 *    Mat4 verify = a * inv;                // 应约等于单位矩阵
 *    // inverse() 不支持非方阵，仅对 Matrix<N,N,T> 可用
 *
 *    // ---------- 输出 ----------
 *    std::cout << I4;  // 格式：1, 0, 0, 0; 0, 1, 0, 0; 0, 0, 1, 0; 0, 0, 0, 1
 *    std::cout << m32; // 格式：0, 0; 0, 0; 0, 0
 *
 *    // ---------- 常见变换矩阵示例（4×4） ----------
 *    // 平移矩阵
 *    Mat4 translate = identity_matrix<4, float>();
 *    translate[0][3] = tx;
 *    translate[1][3] = ty;
 *    translate[2][3] = tz;
 *
 *    // 缩放矩阵
 *    Mat4 scale = identity_matrix<4, float>();
 *    scale[0][0] = sx;
 *    scale[1][1] = sy;
 *    scale[2][2] = sz;
 *
 *    // ---------- 旋转矩阵（传入弧度，如 M_PI / 6） ----------
 *    Mat4 rx = rotation_x(M_PI / 6);     // 绕 X 轴旋转 30°
 *    Mat4 ry = rotation_y(M_PI / 4);     // 绕 Y 轴旋转 45°
 *    Mat4 rz = rotation_z(M_PI / 3);     // 绕 Z 轴旋转 60°
 *    Mat4 ra = rotation_axis({1,1,0}, M_PI / 2);  // 绕任意轴旋转
 *    Mat3 r2d = rotation_2d(M_PI / 6);   // 2D 旋转（3×3 齐次）
 *
 *    // 组合变换：先缩放 → 再旋转 → 再平移
 *    Mat4 mvp = translate * rz * scale;
 *
 *  【注意事项】
 *    - operator* 在 Vec2/Vec3/Vec4 中是标量乘法，在 Vec3/Vec4 中还作为
 *      点积（两个同为 Vec3 的对象相乘返回内积标量）；
 *      叉积使用 operator^。
 *    - 标量乘除的参数类型为 float；如需 double 精度，请自行修改模板
 *      或定义 typedef Vec2<double> Vec2d 等别名。
 *    - Matrix 内部为行主序存储，m[row][col] 访问第 row 行第 col 列。
 *    - 矩阵乘法不可交换：A * B ≠ B * A。
 *    - inverse() 使用高斯-约当消元法（带选主元）；若矩阵近似奇异
 *      （主元 < 1e-6），返回零矩阵。
 *    - Matrix<ROWS, COLS, T> 模板支持非方阵（如 4×3 等），
 *      但 inverse() 仅适用于方阵。
 *    - Vec4::toVec3() 当 w==0 时不执行透视除法，直接截取 xyz。
 * =================================================================
 */

#ifndef __GEOMETRY_H__
#define __GEOMETRY_H__

#include <cmath>
#include <array>
#include <iostream>

// ========================== Vec2 ==========================

/**
 * @brief 二维向量模板类
 * @tparam T 分量数据类型（通常为 int 或 float）
 */
template <class T>
struct Vec2 {
    T x, y; ///< 两个分量

    /// @brief 默认构造，初始化为 (0, 0)
    Vec2() : x(0), y(0) {}
    /// @brief 用指定值构造
    Vec2(T _x, T _y) : x(_x), y(_y) {}
    /// @brief 从其他类型的 Vec2 构造，支持隐式转换
    template <class U> Vec2(const Vec2<U> &v) : x(v.x), y(v.y) {}

    /// @brief 向量加法
    Vec2<T> operator+(const Vec2<T> &V) const { return Vec2<T>(x + V.x, y + V.y); }
    /// @brief 向量减法
    Vec2<T> operator-(const Vec2<T> &V) const { return Vec2<T>(x - V.x, y - V.y); }
    /// @brief 向量与标量相乘
    Vec2<T> operator*(float scalar)    const { return Vec2<T>(x * scalar, y * scalar); }
    /// @brief 向量与标量相除
    Vec2<T> operator/(float scalar)    const { return Vec2<T>(x / scalar, y / scalar); }
    /// @brief 加法复合赋值
    Vec2<T>& operator+=(const Vec2<T> &V) { x += V.x; y += V.y; return *this; }
    /// @brief 减法复合赋值
    Vec2<T>& operator-=(const Vec2<T> &V) { x -= V.x; y -= V.y; return *this; }
    /// @brief 乘法复合赋值（标量）
    Vec2<T>& operator*=(float scalar) { x *= scalar; y *= scalar; return *this; }
    /// @brief 除法复合赋值（标量）
    Vec2<T>& operator/=(float scalar) { x /= scalar; y /= scalar; return *this; }
    /// @brief 取反向量（一元负号）
    Vec2<T> operator-() const { return Vec2<T>(-x, -y); }

    /// @brief 按索引访问分量（0 -> x, 1 -> y）
    T& operator[](const int i)       { return i == 0 ? x : y; }
    /// @brief 按索引只读访问分量
    const T& operator[](const int i) const { return i == 0 ? x : y; }

    /// @brief 相等比较
    bool operator==(const Vec2<T>& V) const { return x == V.x && y == V.y; }
    /// @brief 不等比较
    bool operator!=(const Vec2<T>& V) const { return !(*this == V); }
};

/// @brief 输出到流，格式为 (x, y)
template <class T>
inline std::ostream& operator<<(std::ostream& out, const Vec2<T>& v) {
    return out << "(" << v.x << ", " << v.y << ")";
}

/// @brief 标量左乘向量
template <class T>
inline Vec2<T> operator*(float scalar, const Vec2<T>& v) {
    return v * scalar;
}

/// @brief 二维整数向量
typedef Vec2<int>   Vec2i;
/// @brief 二维浮点向量
typedef Vec2<float> Vec2f;

// ========================== Vec3 ==========================

/**
 * @brief 三维向量模板类
 * @tparam T 分量数据类型（通常为 int 或 float）
 */
template <class T>
struct Vec3 {
    T x, y, z; ///< 三个分量

    /// @brief 默认构造，初始化为 (0, 0, 0)
    Vec3() : x(0), y(0), z(0) {}
    /// @brief 用指定值构造
    Vec3(T _x, T _y, T _z) : x(_x), y(_y), z(_z) {}
    /// @brief 从其他类型的 Vec3 构造，支持隐式转换
    template <class U> Vec3(const Vec3<U> &v) : x(v.x), y(v.y), z(v.z) {}

    /// @brief 向量加法
    Vec3<T> operator+(const Vec3<T> &V) const { return Vec3<T>(x + V.x, y + V.y, z + V.z); }
    /// @brief 向量减法
    Vec3<T> operator-(const Vec3<T> &V) const { return Vec3<T>(x - V.x, y - V.y, z - V.z); }
    /// @brief 向量与标量相乘
    Vec3<T> operator*(float scalar)    const { return Vec3<T>(x * scalar, y * scalar, z * scalar); }
    /// @brief 向量与标量相除
    Vec3<T> operator/(float scalar)    const { return Vec3<T>(x / scalar, y / scalar, z / scalar); }
    /// @brief 加法复合赋值
    Vec3<T>& operator+=(const Vec3<T> &V) { x += V.x; y += V.y; z += V.z; return *this; }
    /// @brief 减法复合赋值
    Vec3<T>& operator-=(const Vec3<T> &V) { x -= V.x; y -= V.y; z -= V.z; return *this; }
    /// @brief 乘法复合赋值（标量）
    Vec3<T>& operator*=(float scalar) { x *= scalar; y *= scalar; z *= scalar; return *this; }
    /// @brief 除法复合赋值（标量）
    Vec3<T>& operator/=(float scalar) { x /= scalar; y /= scalar; z /= scalar; return *this; }
    /// @brief 取反向量（一元负号）
    Vec3<T> operator-() const { return Vec3<T>(-x, -y, -z); }

    /// @brief 叉积（外积）
    Vec3<T> operator^(const Vec3<T> &V) const { return Vec3<T>(y * V.z - z * V.y, z * V.x - x * V.z, x * V.y - y * V.x); }
    /// @brief 点积（内积）
    T operator*(const Vec3<T> &V)       const { return x * V.x + y * V.y + z * V.z; }
    /// @brief 计算向量的长度（L2 范数）
    float norm()                        const { return std::sqrt(x * x + y * y + z * z); }
    /// @brief 归一化向量（原地修改）
    Vec3<T>& normalize() {
        float n = norm();
        if (n != 0) { x /= n; y /= n; z /= n; }
        return *this;
    }
    /// @brief 按索引访问分量（0 -> x, 1 -> y, 2 -> z）
    T& operator[](const int i)       { return i == 0 ? x : (i == 1 ? y : z); }
    /// @brief 按索引只读访问分量
    const T& operator[](const int i) const { return i == 0 ? x : (i == 1 ? y : z); }

    /// @brief 相等比较
    bool operator==(const Vec3<T>& V) const { return x == V.x && y == V.y && z == V.z; }
    /// @brief 不等比较
    bool operator!=(const Vec3<T>& V) const { return !(*this == V); }
};

/// @brief 输出到流，格式为 (x, y, z)
template <class T>
inline std::ostream& operator<<(std::ostream& out, const Vec3<T>& v) {
    return out << "(" << v.x << ", " << v.y << ", " << v.z << ")";
}

/// @brief 标量左乘向量
template <class T>
inline Vec3<T> operator*(float scalar, const Vec3<T>& v) {
    return v * scalar;
}

/// @brief 三维整数向量
typedef Vec3<int>   Vec3i;
/// @brief 三维浮点向量
typedef Vec3<float> Vec3f;

// ========================== Vec4 ==========================

/**
 * @brief 四维向量模板类（常用于齐次坐标）
 * @tparam T 分量数据类型（通常为 int 或 float）
 */
template <class T>
struct Vec4 {
    T x, y, z, w; ///< 四个分量，w 通常为齐次坐标分量

    /// @brief 默认构造，初始化为 (0, 0, 0, 0)
    Vec4() : x(0), y(0), z(0), w(0) {}
    /// @brief 用指定值构造
    Vec4(T _x, T _y, T _z, T _w) : x(_x), y(_y), z(_z), w(_w) {}
    /// @brief 从其他类型的 Vec4 构造
    template <class U> Vec4(const Vec4<U> &v) : x(v.x), y(v.y), z(v.z), w(v.w) {}
    /// @brief 从 Vec3 构造，默认 w = 1（适合表示 3D 点）
    Vec4(const Vec3<T>& v, T _w = 1) : x(v.x), y(v.y), z(v.z), w(_w) {}

    /// @brief 向量加法
    Vec4<T> operator+(const Vec4<T> &V) const { return Vec4<T>(x + V.x, y + V.y, z + V.z, w + V.w); }
    /// @brief 向量减法
    Vec4<T> operator-(const Vec4<T> &V) const { return Vec4<T>(x - V.x, y - V.y, z - V.z, w - V.w); }
    /// @brief 向量与标量相乘
    Vec4<T> operator*(float scalar)    const { return Vec4<T>(x * scalar, y * scalar, z * scalar, w * scalar); }
    /// @brief 向量与标量相除
    Vec4<T> operator/(float scalar)    const { return Vec4<T>(x / scalar, y / scalar, z / scalar, w / scalar); }
    /// @brief 加法复合赋值
    Vec4<T>& operator+=(const Vec4<T> &V) { x += V.x; y += V.y; z += V.z; w += V.w; return *this; }
    /// @brief 减法复合赋值
    Vec4<T>& operator-=(const Vec4<T> &V) { x -= V.x; y -= V.y; z -= V.z; w -= V.w; return *this; }
    /// @brief 乘法复合赋值（标量）
    Vec4<T>& operator*=(float scalar) { x *= scalar; y *= scalar; z *= scalar; w *= scalar; return *this; }
    /// @brief 除法复合赋值（标量）
    Vec4<T>& operator/=(float scalar) { x /= scalar; y /= scalar; z /= scalar; w /= scalar; return *this; }
    /// @brief 取反向量（一元负号）
    Vec4<T> operator-() const { return Vec4<T>(-x, -y, -z, -w); }

    /// @brief 点积（内积）
    T operator*(const Vec4<T> &V)       const { return x * V.x + y * V.y + z * V.z + w * V.w; }
    /// @brief 计算向量的长度（L2 范数）
    float norm()                        const { return std::sqrt(x * x + y * y + z * z + w * w); }
    /// @brief 归一化向量（原地修改）
    Vec4<T>& normalize() {
        float n = norm();
        if (n != 0) { x /= n; y /= n; z /= n; w /= n; }
        return *this;
    }
    /// @brief 按索引访问分量（0 -> x, 1 -> y, 2 -> z, 3 -> w）
    T& operator[](const int i)       { return i == 0 ? x : (i == 1 ? y : (i == 2 ? z : w)); }
    /// @brief 按索引只读访问分量
    const T& operator[](const int i) const { return i == 0 ? x : (i == 1 ? y : (i == 2 ? z : w)); }

    /// @brief 提取前三个分量，返回 Vec3
    Vec3<T> xyz() const { return Vec3<T>(x, y, z); }
    /// @brief 执行透视除法，将齐次坐标转为三维坐标（若 w != 0 则各分量除以 w）
    Vec3<T> toVec3() const { return (w != 0) ? Vec3<T>(x/w, y/w, z/w) : Vec3<T>(x, y, z); }

    /// @brief 相等比较
    bool operator==(const Vec4<T>& V) const { return x == V.x && y == V.y && z == V.z && w == V.w; }
    /// @brief 不等比较
    bool operator!=(const Vec4<T>& V) const { return !(*this == V); }
};

/// @brief 输出到流，格式为 (x, y, z, w)
template <class T>
inline std::ostream& operator<<(std::ostream& out, const Vec4<T>& v) {
    return out << "(" << v.x << ", " << v.y << ", " << v.z << ", " << v.w << ")";
}

/// @brief 标量左乘向量
template <class T>
inline Vec4<T> operator*(float scalar, const Vec4<T>& v) {
    return v * scalar;
}

/// @brief 四维整数向量
typedef Vec4<int>   Vec4i;
/// @brief 四维浮点向量
typedef Vec4<float> Vec4f;

// ========================== Matrix ==========================

/**
 * @brief 小型矩阵模板类，用于 2x2 ~ 4x4 等常见变换
 * @tparam ROWS 行数
 * @tparam COLS 列数
 * @tparam T 元素数据类型（通常为 float）
 */
template <size_t ROWS, size_t COLS, typename T>
struct Matrix {
    std::array<std::array<T, COLS>, ROWS> m{}; ///< 行主序存储的二维数组

    /// @brief 默认构造，元素自动初始化为 0（因 std::array 的值初始化）
    Matrix() = default;

    /// @brief 按行索引返回可修改的数组
    std::array<T, COLS>& operator[](size_t i) { return m[i]; }
    /// @brief 按行索引返回只读数组
    const std::array<T, COLS>& operator[](size_t i) const { return m[i]; }

    /**
     * @brief 矩阵乘法（this × other）
     * @tparam C2 右侧矩阵的列数
     * @return 结果矩阵，维度为 ROWS × C2
     */
    template <size_t C2>
    Matrix<ROWS, C2, T> operator*(const Matrix<COLS, C2, T>& other) const {
        Matrix<ROWS, C2, T> result;
        for (size_t i = 0; i < ROWS; ++i)
            for (size_t j = 0; j < C2; ++j)
                for (size_t k = 0; k < COLS; ++k)
                    result.m[i][j] += m[i][k] * other.m[k][j];
        return result;
    }

    /// @brief 返回当前矩阵的转置矩阵（行列互换）
    Matrix<COLS, ROWS, T> transpose() const {
        Matrix<COLS, ROWS, T> result;
        for (size_t i = 0; i < ROWS; ++i)
            for (size_t j = 0; j < COLS; ++j)
                result.m[j][i] = m[i][j];
        return result;
    }

    /// @brief 返回当前矩阵的余子式（删除第 r 行、第 c 列后的子矩阵）
    Matrix<ROWS - 1, COLS - 1, T> minor(size_t r, size_t c) const {
        Matrix<ROWS - 1, COLS - 1, T> result;
        for (size_t i = 0, ii = 0; i < ROWS; ++i) {
            if (i == r) continue;
            for (size_t j = 0, jj = 0; j < COLS; ++j) {
                if (j == c) continue;
                result.m[ii][jj++] = m[i][j];
            }
            ++ii;
        }
        return result;
    }

    /// @brief 返回当前方阵的行列式
    T det() const {
        static_assert(ROWS == COLS, "det() only supports square matrices");
        if constexpr (ROWS == 1) {
            return m[0][0];
        } else if constexpr (ROWS == 2) {
            return m[0][0] * m[1][1] - m[0][1] * m[1][0];
        } else {
            T d = T(0);
            for (size_t j = 0; j < COLS; ++j)
                d += ((j & 1) ? -1 : 1) * m[0][j] * minor(0, j).det();
            return d;
        }
    }
};

// ---------- 矩阵与向量乘法 ----------

/// @brief 2x2 矩阵乘以 Vec2
/// @return 变换后的二维向量
template <typename T>
inline Vec2<T> operator*(const Matrix<2, 2, T>& mat, const Vec2<T>& v) {
    return Vec2<T>(
        mat.m[0][0] * v.x + mat.m[0][1] * v.y,
        mat.m[1][0] * v.x + mat.m[1][1] * v.y
    );
}

/// @brief 3x3 矩阵乘以 Vec3
/// @return 变换后的三维向量
template <typename T>
inline Vec3<T> operator*(const Matrix<3, 3, T>& mat, const Vec3<T>& v) {
    return Vec3<T>(
        mat.m[0][0] * v.x + mat.m[0][1] * v.y + mat.m[0][2] * v.z,
        mat.m[1][0] * v.x + mat.m[1][1] * v.y + mat.m[1][2] * v.z,
        mat.m[2][0] * v.x + mat.m[2][1] * v.y + mat.m[2][2] * v.z
    );
}

/// @brief 4x4 矩阵乘以 Vec4（常用于 MVP 变换中的齐次坐标）
/// @return 变换后的四维向量
template <typename T>
inline Vec4<T> operator*(const Matrix<4, 4, T>& mat, const Vec4<T>& v) {
    return Vec4<T>(
        mat.m[0][0] * v.x + mat.m[0][1] * v.y + mat.m[0][2] * v.z + mat.m[0][3] * v.w,
        mat.m[1][0] * v.x + mat.m[1][1] * v.y + mat.m[1][2] * v.z + mat.m[1][3] * v.w,
        mat.m[2][0] * v.x + mat.m[2][1] * v.y + mat.m[2][2] * v.z + mat.m[2][3] * v.w,
        mat.m[3][0] * v.x + mat.m[3][1] * v.y + mat.m[3][2] * v.z + mat.m[3][3] * v.w
    );
}

/**
 * @brief 4x4 矩阵乘以 Vec3（视 Vec3 为齐次坐标 (x,y,z,1)）
 * @return 变换后的三维向量（已执行透视除法）
 * @note 等价于 (mat * Vec4(v, 1)).toVec3()
 */
template <typename T>
inline Vec3<T> operator*(const Matrix<4, 4, T>& mat, const Vec3<T>& v) {
    Vec4<T> r = mat * Vec4<T>(v, T(1));
    return r.toVec3();
}

/**
 * @brief Vec3 左乘 Mat4（等价于 (transpose(mat) * Vec4(v,1)).toVec3()）
 * @note 方便写作 v * mat 而非 mat * v，数学含义为行向量乘矩阵
 */
template <typename T>
inline Vec3<T> operator*(const Vec3<T>& v, const Matrix<4, 4, T>& mat) {
    return mat.transpose() * v;
}

// ---------- 矩阵工具函数 ----------

/**
 * @brief 生成 N×N 单位矩阵
 * @tparam N 矩阵维度
 * @tparam T 元素类型
 * @return 单位矩阵（主对角线为 1，其余为 0）
 */
template <size_t N, typename T>
inline Matrix<N, N, T> identity_matrix() {
    Matrix<N, N, T> result;
    for (size_t i = 0; i < N; ++i)
        result.m[i][i] = T(1);
    return result;
}

/**
 * @brief 矩阵求逆（高斯-约当消元法）
 * @tparam N 矩阵维度（仅支持方阵）
 * @tparam T 元素类型
 * @param mat 待求逆的方阵
 * @return 逆矩阵；若矩阵奇异（不可逆），则返回零矩阵
 * @note 内部使用选主元策略以提高数值稳定性
 */
template <size_t N, typename T>
inline Matrix<N, N, T> inverse(const Matrix<N, N, T>& mat) {
    Matrix<N, N, T> inv = identity_matrix<N, T>();
    Matrix<N, N, T> a = mat;

    for (size_t i = 0; i < N; ++i) {
        // 选主元：在当前列中找绝对值最大的行，避免除零或数值不稳定
        size_t pivot = i;
        for (size_t r = i + 1; r < N; ++r) {
            if (std::abs(a.m[r][i]) > std::abs(a.m[pivot][i]))
                pivot = r;
        }
        // 若主元接近零，说明矩阵奇异，无法求逆
        if (std::abs(a.m[pivot][i]) < T(1e-6)) {
            return Matrix<N, N, T>{};
        }
        // 交换当前行与主元行
        if (pivot != i) {
            std::swap(a.m[pivot], a.m[i]);
            std::swap(inv.m[pivot], inv.m[i]);
        }

        // 将主元行归一化，使主元变为 1
        T pivotVal = a.m[i][i];
        for (size_t c = 0; c < N; ++c) {
            a.m[i][c] /= pivotVal;
            inv.m[i][c] /= pivotVal;
        }

        // 用主元行消去其他行在当前列的值
        for (size_t r = 0; r < N; ++r) {
            if (r == i) continue;
            T factor = a.m[r][i];
            for (size_t c = 0; c < N; ++c) {
                a.m[r][c] -= factor * a.m[i][c];
                inv.m[r][c] -= factor * inv.m[i][c];
            }
        }
    }
    return inv;
}

/**
 * @brief 计算方阵的逆转置（即 (A^{-1})^T）
 * @tparam N 矩阵维度
 * @tparam T 元素类型
 * @param mat 待求逆转置的方阵
 * @return 逆矩阵的转置；若矩阵奇异则返回零矩阵的转置
 */
template <size_t N, typename T>
inline Matrix<N, N, T> invert_transpose(const Matrix<N, N, T>& mat) {
    return inverse(mat).transpose();
}

/// @brief 2x2 浮点矩阵
typedef Matrix<2, 2, float> Mat2;
/// @brief 3x3 浮点矩阵
typedef Matrix<3, 3, float> Mat3;
/// @brief 4x4 浮点矩阵（最常用，用于 3D 模型-视图-投影变换）
typedef Matrix<4, 4, float> Mat4;

// ---------- 旋转矩阵工厂函数 ----------

/**
 * @brief 生成绕 X 轴旋转的 4×4 矩阵
 * @param angle 旋转角度（弧度），正方向按右手定则
 * @return 4×4 旋转矩阵
 *
 *  | 1    0         0      0 |
 *  | 0  cos(a)  -sin(a)  0 |
 *  | 0  sin(a)   cos(a)  0 |
 *  | 0    0         0      1 |
 */
inline Mat4 rotation_x(float angle) {
    float c = std::cos(angle), s = std::sin(angle);
    Mat4 m = identity_matrix<4, float>();
    m[1][1] =  c;  m[1][2] = -s;
    m[2][1] =  s;  m[2][2] =  c;
    return m;
}

/**
 * @brief 生成绕 Y 轴旋转的 4×4 矩阵
 * @param angle 旋转角度（弧度），正方向按右手定则
 * @return 4×4 旋转矩阵
 *
 *  |  cos(a)  0  sin(a)  0 |
 *  |    0     1    0      0 |
 *  | -sin(a)  0  cos(a)  0 |
 *  |    0     0    0      1 |
 */
inline Mat4 rotation_y(float angle) {
    float c = std::cos(angle), s = std::sin(angle);
    Mat4 m = identity_matrix<4, float>();
    m[0][0] =  c;  m[0][2] =  s;
    m[2][0] = -s;  m[2][2] =  c;
    return m;
}

/**
 * @brief 生成绕 Z 轴旋转的 4×4 矩阵
 * @param angle 旋转角度（弧度），正方向按右手定则
 * @return 4×4 旋转矩阵
 *
 *  | cos(a)  -sin(a)  0  0 |
 *  | sin(a)   cos(a)  0  0 |
 *  |   0         0      1  0 |
 *  |   0         0      0  1 |
 */
inline Mat4 rotation_z(float angle) {
    float c = std::cos(angle), s = std::sin(angle);
    Mat4 m = identity_matrix<4, float>();
    m[0][0] =  c;  m[0][1] = -s;
    m[1][0] =  s;  m[1][1] =  c;
    return m;
}

/**
 * @brief 生成绕任意轴旋转的 4×4 矩阵（Rodrigues 公式）
 * @param axis 旋转轴（无需归一化，内部自动归一化）
 * @param angle 旋转角度（弧度），正方向按右手定则
 * @return 4×4 旋转矩阵
 * @note axis 为 (0,0,0) 时返回单位矩阵
 */
inline Mat4 rotation_axis(Vec3f axis, float angle) {
    float n = axis.norm();
    if (n == 0) return identity_matrix<4, float>();
    axis.normalize();
    float c = std::cos(angle), s = std::sin(angle);
    float t = 1 - c;
    float x = axis.x, y = axis.y, z = axis.z;

    Mat4 m = identity_matrix<4, float>();
    m[0][0] = t*x*x + c;    m[0][1] = t*x*y - s*z;  m[0][2] = t*x*z + s*y;
    m[1][0] = t*x*y + s*z;  m[1][1] = t*y*y + c;    m[1][2] = t*y*z - s*x;
    m[2][0] = t*x*z - s*y;  m[2][1] = t*y*z + s*x;  m[2][2] = t*z*z + c;
    return m;
}

/**
 * @brief 生成 2D 旋转的 3×3 矩阵（齐次坐标，可直接乘 Vec3(x,y,1)）
 * @param angle 旋转角度（弧度）
 * @return 3×3 旋转矩阵
 *
 *  | cos(a)  -sin(a)  0 |
 *  | sin(a)   cos(a)  0 |
 *  |   0         0      1 |
 */
inline Mat3 rotation_2d(float angle) {
    float c = std::cos(angle), s = std::sin(angle);
    Mat3 m = identity_matrix<3, float>();
    m[0][0] =  c;  m[0][1] = -s;
    m[1][0] =  s;  m[1][1] =  c;
    return m;
}

/// @brief 输出到流，格式按行显示，行内用逗号分隔，行间用分号分隔
template <size_t ROWS, size_t COLS, typename T>
inline std::ostream& operator<<(std::ostream& out, const Matrix<ROWS, COLS, T>& mat) {
    for (size_t i = 0; i < ROWS; ++i) {
        for (size_t j = 0; j < COLS; ++j) {
            out << mat.m[i][j];
            if (j + 1 < COLS) out << ", ";
        }
        if (i + 1 < ROWS) out << "; ";
    }
    return out;
}

#endif //__GEOMETRY_H__
