# SSAO — 屏幕空间环境光遮蔽

> 参考：[haqr.eu - SSAO](https://haqr.eu/tinyrenderer/ssao/) / Screen Space Ambient Occlusion

---

## 一、什么是 SSAO？

**环境光遮蔽（Ambient Occlusion）** 模拟现实中缝隙、角落处因几何遮挡导致环境光减少的现象。比如：

- 角落比开放平面更暗
- 折叠的缝隙比平面更暗
- 物体接触地面的地方有接触阴影

**全精度 AO** 需要在每个点向半球发射光线做射线检测，代价极高。**SSAO** 是一种屏幕空间的近似：仅利用已渲染的深度缓冲，在后处理阶段近似计算遮蔽。

---

## 二、算法原理

### 2.1 核心思路

对屏幕上每个像素：

1. 取该像素的深度值 `center_z`
2. 在周围一定半径内随机撒 N 个采样点
3. 检查每个采样点的深度 `neighbor_z` 是否比 `center_z` 更靠近相机
4. 如果更近（高墙遮挡），则认为该方向被遮蔽
5. 用遮蔽比例调制像素颜色

### 2.2 深度缓冲的方向约定

本项目的 Z-buffer 约定：**值越大越靠近相机**（因为初始化为 `-1e10f`，渲染时取 `max` 写入）。

因此：
- `neighbor_z > center_z` → 邻居比当前像素更近 → 当前像素被遮挡
- `neighbor_z < center_z` → 邻居比当前像素更远 → 当前像素开阔

---

## 三、代码实现

```cpp
void ApplySSAO(TGAImage &img, const std::vector<float> &zbuffer, int width, int height) {
    std::default_random_engine generator(2026);        // 固定随机种子
    std::uniform_real_distribution<float> random_floats(0.0, 1.0);

    #pragma omp parallel for
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {

            float center_z = zbuffer[x + y * width];

            // 背景像素跳过
            if (center_z < -1e9f) continue;

            float total_occlusion = 0.0f;
            int sample_count = 16;     // 采样点数
            float radius = 15.0f;      // 采样半径（像素）

            for (int i = 0; i < sample_count; i++) {
                // 随机角度和半径
                float angle = random_floats(generator) * 2.0f * M_PI;
                float r = random_floats(generator) * radius;

                // 采样点屏幕坐标
                int sample_x = x + (int)(r * cos(angle));
                int sample_y = y + (int)(r * sin(angle));

                // 越界保护
                if (sample_x < 0 || sample_x >= width ||
                    sample_y < 0 || sample_y >= height) continue;

                float neighbor_z = zbuffer[sample_x + sample_y * width];

                // 邻居更近 → 遮蔽
                if (neighbor_z > center_z) {
                    float depth_diff = neighbor_z - center_z;
                    if (depth_diff < 15.0f) {     // 深度差异阈值
                        total_occlusion += 1.0f;
                    }
                }
            }

            // 计算遮蔽因子
            float occlusion_factor = total_occlusion / (float)sample_count;
            float shadow_factor = 1.0f - occlusion_factor * 0.65f;

            // 调制颜色
            TGAColor color = img.get(x, y);
            for (int c = 0; c < 3; c++) {
                color[c] = (int)(color[c] * shadow_factor);
            }
            img.set(x, y, color);
        }
    }
}
```

---

## 四、关键参数解析

### 4.1 采样参数

| 参数 | 取值 | 说明 |
|------|------|------|
| `sample_count` | 16 | 每像素采样点数。越多越平滑，但越慢 |
| `radius` | 15.0f | 采样半径（像素）。决定缝隙阴影的粗细 |
| `generator seed` | 2026 | 固定种子保证每帧结果一致 |

### 4.2 深度差异阈值

```cpp
if (depth_diff < 15.0f) {
    total_occlusion += 1.0f;
}
```

**为什么需要阈值？**

没有阈值时，远处飞过的一个物体可能"遮蔽"近处的墙面——它们在屏幕空间距离近，但实际空间距离远。

深度阈值 `15.0f` 的含义：**只有深度差异小于此值的遮挡才计入遮蔽**。超过此值说明两个表面实际距离较远，不属于同一个转折/缝隙。

| depth_diff | 判定 |
|-----------|------|
| `< 15.0` | 同一区域附近的几何遮挡，计入遮蔽 |
| `≥ 15.0` | 距离过远的物体，忽略 |

### 4.3 遮蔽强度

```cpp
float shadow_factor = 1.0f - occlusion_factor * 0.65f;
```

| `0.65f` 的含义 | 效果 |
|---------------|------|
| 值越大 | 缝隙越黑，画面更硬朗 |
| 值越小 | 缝隙轻微变暗，画面更柔和 |

`shadow_factor` 的范围：
- 完全开阔：`occlusion_factor = 0` → `shadow_factor = 1.0`（颜色不变）
- 完全遮蔽：`occlusion_factor = 1.0` → `shadow_factor = 0.35`（亮度降为 35%）

---

## 五、采样分布示意

```
          ·  ·
       ·        ·
      ·    ●     ·      ● = 当前像素 (x, y)
       ·        ·       · = 随机采样点（16个）
          ·  ·           采样半径 = 15 像素
```

每个采样点的坐标计算：

```
sample_x = x + r × cos(angle)
sample_y = y + r × sin(angle)

其中 angle ∈ [0, 2π)，r ∈ [0, radius)
```

这种极坐标随机采样产生圆形分布，避免方格采样带来的方向性偏差。

---

## 六、SSAO 在渲染管线中的位置

```
┌──────────────────────────────────────────────────────┐
│                 渲染管线                               │
│                                                       │
│  Pass 1: Shadow Map 渲染                              │
│     ↓                                                 │
│  Pass 2: Phong 渲染（含阴影测试）                      │
│     ↓                                                 │
│  ★ ApplySSAO(img, zbufferf, width, height)  ← 后处理 │
│     ↓                                                 │
│  输出 output.tga                                      │
│                                                       │
└──────────────────────────────────────────────────────┘
```

SSAO 是**后处理（Post-processing）**步骤，在所有 3D 渲染完成后执行。它只需要：
- 已渲染的图像 `img`
- 深度缓冲 `zbufferf`
- 图像尺寸

不需要任何 3D 场景信息，这是"屏幕空间"方法的本质优势。

---

## 七、SSAO 的局限性

| 问题 | 说明 |
|------|------|
| **仅基于深度** | 不考虑法线方向，可能在平面上产生不自然的遮蔽 |
| **屏幕空间** | 屏幕外的几何体无法产生遮蔽（disocclusion 伪影） |
| **固定半径** | 近处和远处使用相同的像素半径，远处效果可能过强 |
| **随机噪声** | 16 个采样可能不够，产生噪点 |
| **深度阈值硬切** | `depth_diff < 15.0f` 是硬切换，可能产生带状伪影 |

### 可能的改进方向

1. **法线感知的 SSAO**：利用半球采样（沿法线方向），避免背面采样
2. **多尺度模糊**：SSAO 后加一圈高斯模糊，消除噪点
3. **深度感知半径**：远处物体使用较小半径，避免过度遮蔽
4. **增加采样数**：如 32 或 64，但需平衡性能
5. **HBAO（Horizon-Based AO）**：沿各方向找最大遮蔽角，更精确
