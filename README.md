# SoftRenderer

A software renderer built while learning from the [tinyrenderer](https://github.com/ssloy/tinyrenderer) course. No graphics API dependency — everything runs on the CPU.

## Rendering Pipeline

```
Object Space ──ModelView──▶ Eye Space ──Perspective──▶ Clip Space ──÷w──▶ NDC ──Viewport──▶ Screen Space
```

### Implemented Stages

| Stage | Description |
|-------|-------------|
| **OBJ Loading** | Parse `.obj` vertex and face data |
| **ModelView Transform** | `lookat(eye, center, up)` — positions the camera in the scene |
| **Perspective Transform** | Perspective projection with focal length `f = ‖eye - center‖` |
| **Perspective Division** | Homogeneous `(x/w, y/w, z/w)` — converts clip space to NDC |
| **Viewport Transform** | Maps NDC `[-1,1]` to screen pixel coordinates |
| **Triangle Rasterization** | Barycentric coordinates via matrix method (`invert_transpose`) |
| **Z-Buffer Depth Test** | Float-precision depth buffer with per-pixel depth comparison |
| **Phong Lighting** | Ambient + Lambert diffuse + Phong specular in fragment shader |

### Pipeline Flow

```
for each face in model:
    1. Vertex Shader:  v_clip = Perspective × ModelView × Vec4(v, 1)
                      v_eye  = (ModelView × Vec4(v, 1)).toVec3()
                      light  = normalize((ModelView × Vec4(light_dir, 0)).xyz)
    2. Rasterize:
       a. Perspective division → NDC
       b. Viewport transform → screen coords
       c. Barycentric interpolation for each pixel inside the bounding box
       d. Z-buffer test
    3. Fragment Shader:
       n = normalize(cross(tri[1]-tri[0], tri[2]-tri[0]))   // face normal
       r = normalize(n*(n*l)*2 - l)                          // reflected light
       ambient  = 0.3
       diffuse   = max(0, n·l)
       specular  = pow(max(r.z, 0), 40)
       color     = white × min(1, ambient + 0.4*diffuse + 0.9*specular)
    4. Write pixel to framebuffer
```

## Project Structure

```
SoftRenderer/
├── main.cpp              # Main loop + PhongShader / RandomShader
├── renderer.h / cpp      # MVP matrices, Rasterize(), Z-buffer utilities
├── geometry.h            # Vec2/3/4, Matrix, rotation factories, inverse
├── Model.h / cpp         # OBJ file parser
├── tgaimage.h / cpp      # TGA image I/O (RGB/RGBA/Grayscale, RLE)
├── Note/                 # Learning notes
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
# Enter model path when prompted, e.g.: ../obj/african_head/african_head.obj
```

Outputs:
- `output.tga` — rendered image
- `zbuffer.tga` — depth buffer visualization

## Acknowledgments

This project is a learning exercise based on [ssloy/tinyrenderer](https://github.com/ssloy/tinyrenderer).