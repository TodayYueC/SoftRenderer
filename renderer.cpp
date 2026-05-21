#include "renderer.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>

Mat4 modelview, perspective, viewport;
std::vector<float> zbufferf;

void ModelView(const Vec3f eye, const Vec3f center, const Vec3f up) {
    Vec3f n = (eye - center).normalize();
    Vec3f l = (up ^ n).normalize();
    Vec3f m = (n ^ l).normalize();
    Mat4 rot;
    rot[0]= {l.x, l.y, l.z, 0};
    rot[1] = {m.x, m.y, m.z, 0};
    rot[2] = {n.x, n.y, n.z, 0};
    rot[3] = {0, 0, 0, 1};
    Mat4 trans;
    trans[0]= {1, 0, 0, -center.x};
    trans[1] = {0, 1, 0, -center.y};
    trans[2] = {0, 0, 1, -center.z};
    trans[3] = {0, 0, 0, 1};
    modelview = rot * trans;
}

void Perspective(float f) {
    perspective[0] = {1, 0, 0, 0};
    perspective[1] = {0, 1, 0, 0};
    perspective[2] = {0, 0, 1, 0};
    perspective[3] = {0, 0, -1/f, 1};
}

void Viewport(int x, int y, int w, int h) {
    viewport[0] = {w/2.f,0,0,x+w/2.f};
    viewport[1] = {0,h/2.f,0,y+h/2.f};
    viewport[2] = {0,0,1,0};
    viewport[3] = {0,0,0,1};
}

void Rasterize(TGAImage &img, const Vec4f face[3],const IShader &shader) {
    Vec4f ndc[3] = {
        Vec4f(face[0].x/face[0].w, face[0].y/face[0].w, face[0].z/face[0].w, 1),
        Vec4f(face[1].x/face[1].w, face[1].y/face[1].w, face[1].z/face[1].w, 1),
        Vec4f(face[2].x/face[2].w, face[2].y/face[2].w, face[2].z/face[2].w, 1),
    };
    for (int i=0; i<3; i++) {
        ndc[i] = viewport * ndc[i];
    }
    Vec2f screen[3] = {Vec2f(ndc[0].x, ndc[0].y), Vec2f(ndc[1].x, ndc[1].y), Vec2f(ndc[2].x, ndc[2].y)};
    Mat3 area;
    area[0] = {screen[0].x, screen[0].y,1};
    area[1] = {screen[1].x, screen[1].y,1};
    area[2] = {screen[2].x, screen[2].y,1};
    float triangleArea = area.det();
    if (triangleArea<1) return;

    auto [minxx,maxxx] = std::minmax({screen[0].x, screen[1].x, screen[2].x});
    auto [minyt,maxyy] = std::minmax({screen[0].y, screen[1].y, screen[2].y});

    int minx = std::max((int)minxx, 0);
    int maxx = std::min((int)maxxx, img.width()-1);
    int miny = std::max((int)minyt, 0);
    int maxy = std::min((int)maxyy, img.height()-1);
    #pragma omp parallel for
    for (int x = minx; x <= maxx; x++) {
        for (int y = miny; y <= maxy; y++) {
            Vec3f heavyPoint = invert_transpose(area) * Vec3f(x, y, 1);
            if (heavyPoint.x < 0 || heavyPoint.y < 0 || heavyPoint.z < 0) continue;
            float z = heavyPoint * Vec3f{ndc[0].z,ndc[1].z,ndc[2].z};
            if (z <= zbufferf[x+y*img.width()]) continue;
            Vec3f bar = Vec3f(heavyPoint.x/face[0].w, heavyPoint.y/face[1].w, heavyPoint.z/face[2].w);
            bar = bar/(bar.x+bar.y+bar.z);
            auto [discard, color] = shader.fragment(bar);
            if (discard) continue;
            zbufferf[x+y*img.width()] = z;
            img.set(x, y, color);
        }
    }
}

TGAColor RandomColor() {
    TGAColor rnd;
    for (int c = 0; c < 3; c++) rnd[c] = std::rand() % 255;
    return rnd;
}

TGAImage CreateZBufferImage(const std::vector<float>& zbufferf, int width, int height) {
    float zmin = std::numeric_limits<float>::max();
    float zmax = -std::numeric_limits<float>::max();

    for (int i = 0; i < width * height; i++) {
        if (zbufferf[i] > -1e9f) {
            zmin = std::min(zmin, zbufferf[i]);
            zmax = std::max(zmax, zbufferf[i]);
        }
    }

    TGAImage zimg(width, height, TGAImage::GRAYSCALE);
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            float z = zbufferf[x + y * width];
            uint8_t val = (uint8_t)((z - zmin) / (zmax - zmin) * 255);
            zimg.set(x, y, TGAColor{val});
        }
    }
    return zimg;
}