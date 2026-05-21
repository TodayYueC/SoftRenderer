# SoftRenderer

A software renderer built while learning from the [tinyrenderer](https://github.com/ssloy/tinyrenderer) course. No graphics API dependency — everything runs on the CPU.

## Features

- **Phong Shading** — Ambient + Lambert diffuse + Phong specular lighting model
- **Tangent-Space Normal Mapping** — Per-pixel normals sampled from texture, constructed via TBN matrix from screen-space edge differences
- **Shadow Mapping** — Two-pass rendering: depth-only pass from the light's point of view, then shadow comparison in the fragment shader with depth bias
- **SSAO (Screen-Space Ambient Occlusion)** — Post-process pass that darkens creases and contact areas using randomized depth sampling
- **Perspective-Correct Interpolation** — Barycentric coordinates divided by `1/w` for correct texture and attribute interpolation
- **Z-Buffer Depth Test** — Float-precision depth buffer with per-pixel comparison
- **Multi-Model Rendering** — Load and render multiple OBJ models in a single scene

## Rendering Pipeline

```
Object Space ──ModelView──▶ Eye Space ──Perspective──▶ Clip Space ──÷w──▶ NDC ──Viewport──▶ Screen Space
```

### Implemented Stages

| Stage | Description |
|-------|-------------|
| **OBJ Loading** | Parse `.obj` vertex, face, normal, and UV data |
| **ModelView Transform** | `lookat(eye, center, up)` — positions the camera in the scene |
| **Perspective Transform** | Perspective projection with focal length `f = ‖eye - center‖` |
| **Perspective Division** | Homogeneous `(x/w, y/w, z/w)` — converts clip space to NDC |
| **Viewport Transform** | Maps NDC `[-1,1]` to screen pixel coordinates |
| **Triangle Rasterization** | Barycentric coordinates via matrix method (`invert_transpose`) + perspective-correct interpolation |
| **Z-Buffer Depth Test** | Float-precision depth buffer with per-pixel depth comparison |
| **Normal Mapping** | Tangent-space normals transformed to view space via TBN matrix derived from screen-space derivatives |
| **Shadow Mapping** | Two-pass depth rendering from light POV; shadow comparison with bias in fragment shader |
| **Phong Lighting** | Ambient + Lambert diffuse + Blinn-Phong specular in fragment shader |
| **SSAO** | Screen-space ambient occlusion post-process with randomized depth sampling |

### Pipeline Flow

```
Pass 1 — Shadow Map Generation:
  for each face in each model:
      Vertex Shader (light POV): v_clip = Viewport * Perspective_Light * ModelView_Light * Vec4(v, 1)
      Rasterize → populate shadow depth buffer

Pass 2 — Main Rendering:
  for each face in each model:
      1. Vertex Shader:
         v_clip = Perspective * ModelView * Vec4(v, 1)
         v_eye  = (ModelView * Vec4(v, 1)).toVec3()
         light  = normalize((ModelView * Vec4(light_dir, 0)).xyz)
         TBN    = construct from screen-space edge differences and UVs

      2. Rasterize:
         a. Perspective division → NDC
         b. Viewport transform → screen coords
         c. Barycentric interpolation (perspective-correct) for each pixel
         d. Z-buffer test

      3. Fragment Shader:
         // Normal mapping
         uv     = interpolate(varying_uv, bar)
         n_tex  = model->normal(uv)               // sampled from normal map
         n      = normalize(TBN * n_tex)           // tangent-space → view-space

         // Shadow mapping
         p_light = M_shadow * p_view               // transform to light clip space
         if (p_light.z + bias < shadowBuffer[...])
             shadow_diff = 0.05, shadow_spec = 0   // in shadow

         // Phong lighting
         r         = reflect(n, light_dir)
         v_dir     = -normalize(p_view)
         ambient   = amb
         diffuse   = max(0, n·l)
         specular  = pow(max(r·v_dir, 0), 40)
         color     = diffuse_color × min(1, amb + 2*shadow_diff*diff + 2*shadow_spec*spec)

      4. Write pixel to framebuffer

Post-Process — SSAO:
  for each screen pixel (x, y):
      sample 16 random neighbors within radius
      for each sample:
          if neighbor_z > center_z and depth_diff < threshold:
              total_occlusion += 1
      shadow_factor = 1 - (total_occlusion / 16) * 0.65
      pixel_color *= shadow_factor
```

## Project Structure

```
SoftRenderer/
├── main.cpp              # Main loop: shadow pass, PhongShader, SSAO post-process
├── renderer.h / cpp      # MVP matrices, Rasterize(), Z-buffer utilities
├── geometry.h            # Vec2/3/4, Matrix, rotation factories, inverse
├── Model.h / cpp         # OBJ file parser (vertices, normals, UVs, textures)
├── tgaimage.h / cpp      # TGA image I/O (RGB/RGBA/Grayscale, RLE)
├── Note/                 # Learning notes (shadow mapping, SSAO, etc.)
└── CMakeLists.txt
```

## Build & Run

Requires a C++20 compiler, CMake, and OpenMP.

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

```bash
./SoftRenderer
# Enter model count, then model paths, e.g.:
# 2
# ../obj/african_head/african_head.obj
# ../obj/diablo/diablo3_pose.obj
```

Outputs:
- `output.tga` — rendered image (with shadows, normal mapping, and SSAO)
- `zbuffer.tga` — depth buffer visualization

## Acknowledgments

This project is a learning exercise based on [ssloy/tinyrenderer](https://github.com/ssloy/tinyrenderer).