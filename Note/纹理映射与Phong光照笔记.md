# 纹理映射、法线贴图与 Phong 光照

> 参考：[haqr.eu - Textures and shadows](https://haqr.eu/tinyrenderer/textures/) / Normal mapping / Phong shading

---

## 一、Model 类扩展 — 纹理坐标与法线

### 1.1 新增数据成员

旧版 Model 只解析 `v`（顶点）和 `f`（面索引），现在完整支持 OBJ 格式：

```cpp
class Model {
private:
    std::vector<Vec3f> verts_;                   // 顶点坐标
    std::vector<Vec3f> norms_;                   // 顶点法线          ← 新增
    std::vector<Vec2f> uvs_;                     // 纹理坐标          ← 新增
    std::vector<std::vector<int>> faces_;        // 面顶点索引
    std::vector<std::vector<int>> face_norms_;   // 面法线索引         ← 新增
    std::vector<std::vector<int>> face_uvs_;     // 面纹理坐标索引     ← 新增
    TGAImage diffusemap_;                        // 漫反射贴图         ← 新增
    TGAImage normalmap_;                         // 法线贴图           ← 新增
};
```

### 1.2 OBJ 格式完整解析

OBJ 文件中面的格式有四种变体：

```
f v1 v2 v3                  # 仅顶点
f v1/vt1 v2/vt2 v3/vt3      # 顶点 + 纹理坐标
f v1/vt1/vn1 ...            # 顶点 + 纹理 + 法线
f v1//vn1 v2//vn2 v3//vn3   # 顶点 + 法线（无纹理）
```

解析逻辑（`Model.cpp`）：

```cpp
if (!line.compare(0, 2, "f ")) {
    std::vector<int> f, fn, fuv;
    iss >> trash;
    for (int i = 0; i < 3; i++) {
        int vidx, vtidx = 0, vnidx = 0;
        iss >> vidx;
        if (iss.peek() == '/') {
            iss >> trash;
            if (iss.peek() == '/') {
                iss >> trash >> vnidx;           // v//vn 格式
            } else {
                iss >> vtidx;
                if (iss.peek() == '/') {
                    iss >> trash >> vnidx;       // v/vt/vn 格式
                }
                // 否则是 v/vt 格式，vnidx 保持 0
            }
        }
        // 否则是 v 格式，vtidx 和 vnidx 都保持 0
        f.push_back(vidx - 1);     // OBJ 索引从 1 开始，转为 0-based
        fn.push_back(vnidx - 1);
        fuv.push_back(vtidx - 1);
    }
}
```

### 1.3 自动加载贴图

构造函数末尾根据 OBJ 文件名自动寻找贴图：

```cpp
std::string texfile(filename);
int dot = texfile.find_last_of('.');
texfile.erase(dot);
normalmap_.read_tga_file(texfile + "_nm_tangent.tga");   // 法线贴图
normalmap_.flip_vertically();
diffusemap_.read_tga_file(texfile + "_diffuse.tga");     // 漫反射贴图
diffusemap_.flip_vertically();
```

例如 `african_head.obj` 会自动加载 `african_head_nm_tangent.tga` 和 `african_head_diffuse.tga`。

### 1.4 新增访问接口

| 方法 | 说明 |
|------|------|
| `normal(int face, int vert)` | 返回指定面第 vert 个顶点的法线向量（来自 `vn` 数据） |
| `uv(int face, int vert)` | 返回指定面第 vert 个顶点的纹理坐标（来自 `vt` 数据） |
| `normal(Vec2f uv)` | 从法线贴图采样，将 [0,255] 映射到 [-1,1] 范围，返回切线空间法线 |
| `diffuse(Vec2f uv)` | 从漫反射贴图采样颜色 |
| `nnorms()` | 返回法线数量 |
| `load_normal_map(...)` | 手动加载法线贴图 |
| `load_diffuse_map(...)` | 手动加载漫反射贴图 |

**法线贴图采样的颜色→法线转换**：

```cpp
Vec3f Model::normal(Vec2f uv) const {
    TGAColor c = normalmap_.get(uv.x * normalmap_.width(), uv.y * normalmap_.height());
    Vec3f res;
    for (int i = 0; i < 3; i++)
        res[2 - i] = c[i] / 255.f * 2.f - 1.f;   // [0,255] → [-1,1]
    return res;
}
```

注意 `res[2-i] = c[i]`：因为 TGA 存储为 BGRA 顺序，这里做了 BGR→XYZ 的重映射。

---

## 二、PhongShader — 纹理 + 法线贴图 + Phong 光照

### 2.1 Shader 类结构

```cpp
class PhongShader : public IShader {
public:
    const Model *model;
    Vec3f l;                    // 眼空间的光照方向
    Vec2f varying_uv[3];        // 插值用纹理坐标
    Vec3f varying_normal[3];    // 插值用法线（眼空间）
    Vec4f tri[3];               // 眼空间顶点位置
};
```

**varying 变量**：Vertex 着色器写入、Fragment 着色器通过重心坐标插值读取的变量。命名源自 GLSL 中的 `varying` 关键字。

### 2.2 Vertex 着色器

```cpp
virtual Vec4f Vertex(const int face, const int vert) {
    Vec4f v = {model->vert(model->face(face)[vert]), 1};
    varying_uv[vert] = model->uv(face, vert);
    varying_normal[vert] = (invert_transpose(modelview) * Vec4f(model->normal(face, vert), 0)).toVec3().normalize();
    tri[vert] = modelview * v;
    return perspective * modelview * v;
}
```

**逐行解析**：

1. **获取模型空间顶点**：`Vec4f v = {model->vert(...), 1}` — w=1 表示这是一个点
2. **保存纹理坐标**：`varying_uv[vert] = model->uv(face, vert)` — 供 fragment 插值采样
3. **法线变换**：`invert_transpose(modelview) * Vec4f(normal, 0)` — 用**逆转置矩阵**变换法线
4. **保存眼空间顶点**：`tri[vert] = modelview * v` — 供 TBN 矩阵构建
5. **返回 Clip Space 坐标**：`perspective * modelview * v`

### 2.3 法线变换为什么用逆转置矩阵？

对模型表面的法线向量 `n`，如果模型变换矩阵为 `M`，那么法线不能直接乘 `M`，而应该乘 `(M⁻¹)ᵀ`。

**直觉理解**：法线垂直于表面切线 `t`，即 `n·t = 0`。变换后需要 `n'·t' = 0`，其中 `t' = M·t`。代入得：

```
n'·(M·t) = 0
(M⁻ᵀ·n')·t = 0      （因为 n·t = (M⁻ᵀ·n')·t 当且仅当 n' = Mᵀ·n）
```

所以 `n' = (M⁻¹)ᵀ · n`。

**代码实现**：`invert_transpose(modelview) * Vec4f(normal, 0)`，其中 w=0 表示方向向量，平移不影响法线。

---

## 三、TBN 矩阵与法线贴图

### 3.1 切线空间（Tangent Space）

法线贴图中存储的法线是在**切线空间**（Tangent Space）中定义的。切线空间以三角形表面为基准：

- **T（Tangent）**：沿纹理 U 方向的切线
- **B（Bitangent）**：沿纹理 V 方向的切线
- **N（Normal）**：表面法线

切线空间中法线默认朝上 `(0, 0, 1)`，法线贴图记录的是偏离这个默认方向的扰动。

### 3.2 TBN 矩阵的构建

需要将切线空间法线变换到眼空间，为此构建 TBN 矩阵。

**数学推导**：

三角形上一点在眼空间的坐标差（边向量）可以表示为 T 和 B 的线性组合：

```
E₀ = tri[1] - tri[0] = Δu₀·T + Δv₀·B
E₁ = tri[2] - tri[0] = Δu₁·T + Δv₁·B
```

写成矩阵形式：

```
[E₀]       [Δu₀  Δv₀] [T]
[E₁]   =                [B]
           [Δu₁  Δv₁]
```

即 `E = U · [T; B]`，所以 `[T; B] = U⁻¹ · E`。

**代码实现**（在 `fragment()` 中）：

```cpp
// 边矩阵（眼空间）
Matrix<2,4,float> E;
Vec4f e0 = tri[1] - tri[0];
Vec4f e1 = tri[2] - tri[0];
E[0] = {e0.x, e0.y, e0.z, e0.w};
E[1] = {e1.x, e1.y, e1.z, e1.w};

// UV 差分矩阵
Matrix<2,2,float> U;
Vec2f u0 = varying_uv[1] - varying_uv[0];
Vec2f u1 = varying_uv[2] - varying_uv[0];
U[0] = {u0.x, u0.y};
U[1] = {u1.x, u1.y};

// 切线矩阵 T = U⁻¹ · E，行分别是 T 和 B
Matrix<2,4,float> T = inverse(U) * E;

// 组装 TBN 矩阵（4×4）
Mat4 A;
Vec4f a0 = {T[0][0], T[0][1], T[0][2], T[0][3]};  // Tangent
Vec4f a1 = {T[1][0], T[1][1], T[1][2], T[1][3]};  // Bitangent
a0.normalize();
a1.normalize();
A[0] = {a0.x, a0.y, a0.z, a0.w};
A[1] = {a1.x, a1.y, a1.z, a1.w};

// N 从顶点法线插值获得
Vec3f a2 = varying_normal[0]*bar.x + varying_normal[1]*bar.y + varying_normal[2]*bar.z;
a2.normalize();
A[2] = {a2.x, a2.y, a2.z, 0};
A[3] = {0, 0, 0, 1};
```

### 3.3 使用 TBN 变换法线

```cpp
Vec3f n = (A * Vec4f(model->normal(uv), 0)).toVec3().normalize();
```

`model->normal(uv)` 从法线贴图采样切线空间法线，`A` 将其变换到眼空间。

---

## 四、Phong 光照模型

### 4.1 三分量光照

Phong 光照 = 环境光（Ambient）+ 漫反射（Diffuse）+ 高光（Specular）：

```
I = I_ambient + I_diffuse + I_specular
  = k_a        + k_d · max(0, n·l) + k_s · max(0, r·v)^p
```

### 4.2 代码实现

```cpp
Vec3f l = ...;  // 眼空间光照方向（在构造函数中变换）

// 在 fragment 中：
Vec3f r = (n * (n * l * 2.f) - l).normalize();   // 反射向量
double amb  = 0.3;                                  // 环境光系数
double diff = std::max(0.f, n * l);                 // Lambert 漫反射
double spec = std::pow(std::max(0.f, r.z), 40);     // Phong 高光

for (int i = 0; i < 3; i++) {
    color[i] *= std::min(1., amb + .4 * diff + .9 * spec);
}
```

**逐项说明**：

| 分量 | 公式 | 系数 | 说明 |
|------|------|------|------|
| **Ambient** | 常数 | 0.3 | 模拟间接光照，确保暗面不完全黑 |
| **Diffuse** | `max(0, n·l)` | 0.4 | Lambert 余弦定律，法线与光照方向的点积 |
| **Specular** | `max(0, r·v)^p` | 0.9, p=40 | 反射向量与视线的点积的幂，控制高光亮度和大小 |

### 4.3 光照方向的处理

```cpp
PhongShader(const Model *model, Vec3f light) : model(model) {
    l = ((modelview * Vec4f(light, 0)).toVec3()).normalize();
}
```

光照方向在世界空间定义 `Vec3f light(1,1,1)`，通过 `modelview` 变换到眼空间。使用 w=0 因为光照方向是方向向量，平移不应影响它。

### 4.4 反射向量

```
r = 2(n·l)n - l
```

**推导**：入射光线 `l` 关于法线 `n` 的反射。将 `l` 分解为法线分量 `(-n·l)n` 和切线分量 `l - (-n·l)n`，反射后切线分量不变、法线分量取反：

```
r = (l - (-n·l)n) + (-1)(-n·l)n = l + 2(n·l)n
```

代码中 `n * (n*l*2.f) - l` 等价于 `2(n·l)n - l`。

### 4.5 高光观察方向

代码中 `spec = pow(max(0, r.z), 40)`，使用 `r.z` 而非 `r·v`。这是因为在眼空间（相机空间）中，视线方向始终是 `(0, 0, -1)`（朝 -Z 看去），所以 `r·v = r·(0,0,1) = r.z`。这是一个常见的简化。

### 4.6 颜色最终计算

```cpp
color[i] *= std::min(1., amb + .4 * diff + .9 * spec);
```

漫反射贴图颜色 `color` 乘以光照强度。`min(1., ...)` 防止过曝。

---

## 五、渲染流程（Phong 版）

```
┌──────────────────────────────────────────────────────────────────────┐
│                       Phong 渲染流程                                 │
├──────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  1. 加载模型（OBJ + diffuse贴图 + normal贴图）                        │
│     ↓                                                                │
│  2. 构建 MVP 矩阵                                                    │
│     ↓                                                                │
│  3. 对每个三角形：                                                    │
│     ├─ Vertex(f, 0/1/2)：                                            │
│     │   ├─ 读取顶点坐标 → modelview → 眼空间位置（保存到 tri[]）       │
│     │   ├─ 读取 UV 坐标 → 保存到 varying_uv[]                         │
│     │   ├─ 读取法线 → invert_transpose(modelview) → 眼空间法线         │
│     │   │   （保存到 varying_normal[]）                                │
│     │   └─ 返回 perspective × modelview × v → Clip Space             │
│     ↓                                                                │
│  4. Rasterize：透视除法 → NDC → Viewport → 屏幕坐标                  │
│     ├─ 重心坐标插值 bar = (α, β, γ)                                  │
│     ├─ fragment(bar)：                                                │
│     │   ├─ 构建 TBN 矩阵（从眼空间边向量和 UV 差分计算）               │
│     │   ├─ 插值 UV → 采样漫反射贴图 → 基础颜色                        │
│     │   ├─ 采样法线贴图 → TBN 变换 → 眼空间法线                       │
│     │   ├─ Phong 光照 = ambient + diffuse + specular                  │
│     │   └─ 颜色 × 光照强度                                           │
│     └─ Z-test → 写入帧缓冲                                           │
│     ↓                                                                │
│  5. 输出 output.tga + zbuffer.tga                                    │
│                                                                      │
└──────────────────────────────────────────────────────────────────────┘
```

---

## 六、IShader 接口的当前设计

```cpp
class IShader {
public:
    virtual std::pair<bool,TGAColor> fragment(const Vec3f bar) const = 0;

    static TGAColor sample2D(const TGAImage &img, const Vec2f &uvf) {
        return img.get(uvf[0] * img.width(), uvf[1] * img.height());
    }
};
```

**注意**：`Vertex()` 方法**未声明在 IShader 基类中**。当前两个 Shader（PhongShader、RandomShader）各自定义了自己的 `Vertex()` 方法，主循环中直接通过具体类型调用。

`sample2D()` 是新增的静态辅助方法，用于纹理采样，但当前 PhongShader 直接调用 `model->diffuse(uv)` 而未使用此方法。

---

## 七、主函数变化

```cpp
int main() {
    int width = 800, height = 800;
    Vec3f light(1,1,1);          // ← 新增：世界空间光照方向
    Vec3f eye(-1,0,2), center(0,0,0), up(0,1,0);

    // ... 初始化 ...

    Model model(path);           // 现在自动加载纹理和法线贴图

    ModelView(eye, center, up);
    Perspective((eye - center).norm());
    Viewport(width/16, height/16, width*7/8, height*7/8);

    PhongShader shader(&model, light);     // ← 改用 PhongShader
    for (int i = 0; i < model.nfaces(); i++) {
        Vec4f face[3] = {shader.Vertex(i,0), shader.Vertex(i,1), shader.Vertex(i,2)};
        Rasterize(img, face, shader);
    }
    // ...
}
```

**与旧版对比**：

| 项目 | 旧版（RandomShader） | 新版（PhongShader） |
|------|---------------------|---------------------|
| Shader | `RandomShader(&model)` | `PhongShader(&model, light)` |
| 颜色来源 | `RandomColor()` 随机颜色 | 漫反射贴图采样 |
| 光照 | 无 | Phong 三分量光照 |
| 法线 | 未使用 | 顶点法线 + 法线贴图 |
| 纹理 | 未使用 | UV 插值 + 贴图采样 |

---

## 八、更新后的功能清单

| 功能 | 状态 | 说明 |
|------|------|------|
| TGA 图像读写 | ✅ | 支持 RGB、RGBA、灰度，RLE 压缩 |
| 几何向量运算 | ✅ | Vec2/3/4，点积、叉积、归一化 |
| 矩阵运算库 | ✅ | 乘法、转置、行列式、求逆、逆转置、旋转工厂 |
| OBJ 模型加载 | ✅ | **完整解析** v/vt/vn 和所有 face 格式 |
| 漫反射贴图 | ✅ **新增** | 自动加载 + UV 采样 |
| 法线贴图 | ✅ **新增** | 自动加载 + 切线空间→眼空间变换（TBN） |
| 纹理映射 | ✅ **新增** | 重心坐标插值 UV + 贴图采样 |
| Phong 光照 | ✅ **新增** | 环境光 + Lambert 漫反射 + Phong 高光 |
| 法线变换 | ✅ **新增** | 逆转置矩阵正确变换法线到眼空间 |
| MVP 矩阵 | ✅ | ModelView / Perspective / Viewport |
| 三角形光栅化 | ✅ | 重心坐标矩阵法 + OpenMP 并行 |
| Z 缓冲（浮点） | ✅ | float 深度缓冲 |
| Shader 接口 | ✅ | IShader + PhongShader + RandomShader |
| 背面剔除 | ❌ | 仅退化三角形检测（`area < 1`），非真正背面剔除 |
| 视锥体裁剪 | ❌ | 未实现 |
| 阴影映射 | ❌ | 未实现 |
| Model 矩阵分离 | ❌ | Model 和 View 仍合并为 ModelView |

---

## 九、后续扩展方向

1. **背面剔除**：检查屏幕空间三角形面积符号（有符号面积 ≤ 0 则朝后），跳过背面三角形
2. **视锥体裁剪**：在光栅化前检查顶点是否在 NDC `[-w, w]` 范围内，裁剪越界图元
3. **阴影映射**：从光源视角渲染深度图，在 fragment shader 中比较深度判断阴影
4. **Model 矩阵分离**：将模型自身变换（旋转/缩放/平移）从 ModelView 中独立为 `Model × View × Projection`
5. **Blinn-Phong 光照**：用半程向量 `h = normalize(l + v)` 代替反射向量 `r`，计算更稳定
6. **IShader 接口完善**：将 `Vertex()` 声明为 IShader 虚函数，实现多态调用
