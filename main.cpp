#include <chrono>
#include <cmath>
#include <iostream>
#include <random>
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"

/*TGAColor white = {255, 255, 255, 255};  // RGBA 白色
TGAColor red   = {0, 0, 255, 255};        // RGBA 红色（注意 BGRA 顺序）
TGAColor green = {0, 255, 0, 255};        // RGBA 绿色
TGAColor blue  = {255, 0, 0, 255};        // RGBA 蓝色
TGAColor yellow = {0, 255, 255, 255};     // RGBA 黄色*/

int width = 800;
int height = 800;

void Line(TGAImage &img, int x0, int y0, int x1, int y1, TGAColor color)
{
    bool steep = abs(y1-y0) > abs(x1-x0);
    if (steep) {
        std::swap(x0, y0);
        std::swap(x1, y1);
    }
    if (x0 > x1) {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }
    for(int x=x0; x<=x1; x++) {
        float t = (x-x0)/(float)(x1-x0);
        int y = y0 + (int)(t * (y1-y0));
        if (steep) {
            img.set(y, x, color);
        } else {
            img.set(x, y, color);
        }
    }
}

double TriangleArea(int x0,int y0,int x1,int y1,int x2,int y2)
{
    return (x0*(y1-y2)+x2*(y0-y1)+(x1*(y2-y0)))*0.5;
}

void Triangle(TGAImage &img, int x0, int y0, int x1, int y1, int x2, int y2, TGAColor color) {
    int boxMinX = std::min(x0, std::min(x1, x2));
    int boxMaxX = std::max(x0, std::max(x1, x2));
    int boxMinY = std::min(y0, std::min(y1, y2));
    int boxMaxY = std::max(y0, std::max(y1, y2));
    double triangleArea = TriangleArea(x0, y0, x1, y1, x2, y2);

    if (triangleArea<1) return;
    #pragma omp parallel for
    for (int x=boxMinX; x<=boxMaxX; x++) {
        for (int y=boxMinY; y<=boxMaxY; y++) {

            double alpha = TriangleArea(x, y, x1, y1, x2, y2) / triangleArea;
            double beta = TriangleArea(x, y, x2, y2, x0, y0) / triangleArea;
            double gamma = TriangleArea(x, y, x0, y0, x1, y1) / triangleArea;

            if (alpha >= 0 && beta >= 0 && gamma >= 0) {
                img.set(x, y, color);
            }
        }
    }
}


std::tuple<int,int> project(Vec3f v) {
    return { (v.x + 1.) *  width/2,
             (v.y + 1.) * height/2 };
}
int main() {


    TGAImage img(width, height, TGAImage::RGB);

    std::string path;
    std::cout << "Enter model path:";
    std::cin >> path;

    Model model(path);

    for (int i = 0; i < model.nfaces(); i++) {
        std::vector<int> face = model.face(i);
        Vec3f v0 = model.vert(face[0]);
        Vec3f v1 = model.vert(face[1]);
        Vec3f v2 = model.vert(face[2]);

        auto [x0, y0] = project(v0);
        auto [x1, y1] = project(v1);
        auto [x2, y2] = project(v2);

        TGAColor rnd;
        for (int c=0; c<3; c++) rnd[c] = std::rand()%255;

        Triangle(img, x0, y0, x1, y1, x2, y2,rnd);
    }

    img.write_tga_file("output.tga");
    return 0;
}
