#pragma once

#include "geometry.h"
#include "tgaimage.h"
#include <vector>

extern Mat4 modelview;
extern Mat4 perspective;
extern Mat4 viewport;
extern std::vector<float> zbufferf;

class IShader {
public:
    virtual std::pair<bool,TGAColor> fragment(const Vec3f bar) const = 0;
};

void ModelView(const Vec3f eye, const Vec3f center, const Vec3f up);
void Perspective(float f);
void Viewport(int x, int y, int w, int h);
void Rasterize(TGAImage &img, const Vec4f face[3], const IShader &shader);



TGAColor RandomColor();
TGAImage CreateZBufferImage(const std::vector<float>& zbufferf, int width, int height);