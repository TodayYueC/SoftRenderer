#include <chrono>
#include <cmath>
#include <iostream>
#include <random>
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include "renderer.h"


class PhongShader : public IShader {
public:
    const Model *model;
    Vec3f l;
    Vec3f tri[3];

    PhongShader(const Model *model, Vec3f light) : model(model) {
        l = ((modelview * Vec4f(light,0)).toVec3()).normalize();
    }

    virtual Vec4f Vertex(const int face,const int vert) {
        Vec4f v = {model->vert(model->face(face)[vert]),1};
        tri[vert] =  (modelview * v).toVec3();
        return perspective * modelview *v;
    }

    virtual std::pair<bool,TGAColor> fragment(const Vec3f bar) const {
        TGAColor color = {255,255,255,255};
        Vec3f n = (tri[1]-tri[0]) ^ (tri[2]-tri[0]);
        n = n.normalize();//法线
        Vec3f r = (n * (n * l)*2 - l).normalize(); //反射光线
        double amb = 0.3; //环境光
        double diff = std::max(0.f,n*l);//漫反射
        double spec = std::pow(std::max(0.f,r.z),40);//高光

        for (int i= 0; i < 3; i++) {
            color[i] *= std::min(1., amb + .4 * diff + .9 * spec);
        }
        return {false, color};
    }


};

class RandomShader : public IShader {
public:
    const Model *model;
    TGAColor color;
    Vec3f tri[3];

    RandomShader(const Model *model) : model(model) {}

    virtual Vec4f Vertex(const int face,const int vert) {
        Vec4f v = {model->vert(model->face(face)[vert]),1};
        tri[vert] =  (modelview * v).toVec3();
        return perspective * modelview *v;
    }

    virtual std::pair<bool,TGAColor> fragment(const Vec3f bar) const {
        return {false, color};
    }
};

    int main() {

        //渲染数据定义

        int width = 800;
        int height = 800;

        Vec3f light(1,1,1);
        Vec3f eye(-1,0,2);
        Vec3f center(0,0,0);
        Vec3f up(0,1,0);

        zbufferf.assign(width * height, -1e10f);

        TGAImage img(width, height, TGAImage::RGB);

        //模型读取
        std::string path;
        std::cout << "Enter model path:";
        std::cin >> path;

        Model model(path);

        //MVP矩阵变换
        ModelView(eye, center, up);
        Perspective((eye - center).norm());
        Viewport(width/16,height/16,width * 7/8,height * 7/8);

        PhongShader shader(&model,light);
        for (int i = 0; i < model.nfaces(); i++) {
            Vec4f face[3] = {shader.Vertex(i,0),shader.Vertex(i,1),shader.Vertex(i,2)};
            Rasterize(img, face, shader);
        }

        img.write_tga_file("output.tga");
        CreateZBufferImage(zbufferf, width, height).write_tga_file("zbuffer.tga");

        return 0;
}


//旧版代码
/*
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

void Triangle(TGAImage &img,Vec3f v0, Vec3f v1, Vec3f v2,int x0, int y0, int x1, int y1, int x2, int y2, TGAColor color) {
    int boxMinX = std::max(0, std::min(x0, std::min(x1, x2)));
    int boxMaxX = std::min(width - 1, std::max(x0, std::max(x1, x2)));
    int boxMinY = std::max(0, std::min(y0, std::min(y1, y2)));
    int boxMaxY = std::min(height - 1, std::max(y0, std::max(y1, y2)));
    double triangleArea = TriangleArea(x0, y0, x1, y1, x2, y2);

    if (triangleArea<1) return;
    #pragma omp parallel for
    for (int x=boxMinX; x<=boxMaxX; x++) {
        for (int y=boxMinY; y<=boxMaxY; y++) {
            double alpha = TriangleArea(x, y, x1, y1, x2, y2) / triangleArea;
            double beta = TriangleArea(x, y, x2, y2, x0, y0) / triangleArea;
            double gamma = TriangleArea(x, y, x0, y0, x1, y1) / triangleArea;
            if (alpha<0 || beta<0 || gamma<0) continue;
            float z = alpha*v0.z + beta*v1.z + gamma*v2.z;
            if (z <= zbufferf[x + y * width]) continue;
            img.set(x, y, color);
            zbufferf[x + y * width] = z;
        }
    }
}

TGAImage CreateZBufferImage(const std::vector<float>& zbufferf, int width, int height) {
    float zmin = std::numeric_limits<float>::max();
    float zmax = -std::numeric_limits<float>::max();

    for (int i = 0; i < width * height; i++) {
        if (zbufferf[i] > -1e9f) {  // 排除背景像素
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


std::tuple<int,int,int> project(Vec3f v) {
    return { (v.x + 1.) *  width/2,
             (v.y + 1.) * height/2 ,
                (v.z + 1.) * 255/2
    };
}

Vec3f persp(Vec3f v) {

    double c = 3;
    return v/(1-v.z/c);
}
*/

/*for (int i = 0; i < model.nfaces(); i++) {
    std::vector<int> face = model.face(i);
    Mat4 rot = rotation_y(M_PI/6);
    Vec3f v0 = persp(rot * model.vert(face[0]));
    Vec3f v1 = persp(rot * model.vert(face[1]));
    Vec3f v2 = persp(rot * model.vert(face[2]));

    auto [x0, y0, z0] = project(v0);
    auto [x1, y1, z1] = project(v1);
    auto [x2, y2, z2] = project(v2);

    TGAColor rnd;
    for (int c=0; c<3; c++) rnd[c] = std::rand()%255;

    Triangle(img, v0,v1,v2,x0, y0, x1, y1,x2, y2,rnd);
}

img.write_tga_file("output.tga");
CreateZBufferImage(zbufferf, width, height).write_tga_file("zbuffer.tga");*/
