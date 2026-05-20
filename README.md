# SoftRenderer

A simple software renderer built while following the [tinyrenderer](https://github.com/ssloy/tinyrenderer) course.

## Features

- Bresenham line drawing
- Triangle rasterization with barycentric coordinates
- Z-buffer depth testing
- Model-View-Projection (MVP) transformation pipeline
- Phong lighting model (ambient + diffuse + specular)
- .obj model loading

## Build

Requires a C++17 compiler and CMake.

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

## Usage

```bash
SoftRenderer.exe <path/to/model.obj>
```

## Acknowledgments

This project is a learning exercise based on [ssloy/tinyrenderer](https://github.com/ssloy/tinyrenderer).