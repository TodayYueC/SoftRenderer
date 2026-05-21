#include <chrono>
#include <cmath>
#include <iostream>
#include <random>
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include "renderer.h"

class MapShader : public IShader {
public:
    const Model *model;

    MapShader(const Model *model) : model(model) {}

    virtual Vec4f Vertex(const int face,const int vert) {
        Vec4f v = {model->vert(model->face(face)[vert]),1};
        return perspective * modelview * v;
    }

    virtual std::pair<bool, TGAColor> fragment(const Vec3f bar) const {
        // 第一遍不关心颜色，直接返回。Rasterize 内部会自动更新全局 zbufferf
        TGAColor color{0,0,0,0};
        return {false, color};
    }

};


class PhongShader : public IShader {
public:
    const Model *model;
    Vec3f l;
    Vec2f varying_uv[3];
    Vec3f varying_normal[3];
    Vec4f tri[3];
    std::vector<float> shadowBuffer;
    Mat4 M_shadow;

    int shadow_width;
    int shadow_height;

    PhongShader(const Model *model, Vec3f light,std::vector<float> sb,Mat4 ms,int w,int h) : model(model) ,shadowBuffer(sb),M_shadow(ms)
    ,shadow_width(w),shadow_height(h) {
        l = ((modelview * Vec4f(light,0)).toVec3()).normalize();
    }

    virtual Vec4f Vertex(const int face,const int vert) {
        Vec4f v = {model->vert(model->face(face)[vert]),1};
        varying_uv[vert] = model->uv(face, vert);
        varying_normal[vert] =(invert_transpose(modelview) * Vec4f(model->normal(face, vert), 0)).toVec3().normalize();

        tri[vert] = modelview * v;

        return perspective * modelview *v;
    }

    virtual std::pair<bool,TGAColor> fragment(const Vec3f bar) const {

        Matrix<2,4, float> E;
        Vec4f e0 = tri[1]-tri[0];
        Vec4f e1 = tri[2]-tri[0];
        E[0] = {e0.x, e0.y, e0.z, e0.w};
        E[1] = {e1.x, e1.y, e1.z, e1.w};

        Matrix<2,2,float> U;
        Vec2f u0 = varying_uv[1] - varying_uv[0];
        Vec2f u1 = varying_uv[2] - varying_uv[0];
        U[0] = {u0.x, u0.y};
        U[1] = {u1.x, u1.y};

        Matrix<2,4,float> T = inverse(U) * E;

        Mat4 A;
        Vec4f a0 = {T[0][0], T[0][1], T[0][2], T[0][3]};
        Vec4f a1 = {T[1][0], T[1][1], T[1][2], T[1][3]};
        a0.normalize();
        a1.normalize();
        A[0] = {a0.x, a0.y, a0.z, a0.w};
        A[1] = {a1.x, a1.y, a1.z, a1.w};
        Vec3f a2 = varying_normal[0] * bar.x + varying_normal[1] * bar.y + varying_normal[2] * bar.z;
        a2.normalize();
        A[2] = {a2.x, a2.y, a2.z, 0};
        A[3] = {0, 0, 0, 1};

        Vec2f uv = varying_uv[0] * bar.x + varying_uv[1] * bar.y + varying_uv[2] * bar.z;
        TGAColor color = model->diffuse(uv);  // 替代原来的 {255,255,255,255}
        Vec3f n = (A *Vec4f(model->normal(uv),0)).toVec3().normalize();
        Vec3f r = (n * (n*l*2.f) - l).normalize();

        //处理阴影
        Vec4f p_view = tri[0] * bar.x + tri[1] * bar.y + tri[2] * bar.z;
        Vec4f p_light = M_shadow * p_view;
        p_light = p_light / p_light.w;

        float shadow_diff = 1.f;
        float shadow_spec = 1.f;
        float bias = 0.01;

        int idx_x = std::max(0, std::min(shadow_width - 1, (int)p_light.x));
        int idx_y = std::max(0, std::min(shadow_height - 1, (int)p_light.y));
        int shadow_idx = idx_x + idx_y * shadow_width;

        if (p_light.z + bias < shadowBuffer[shadow_idx]) {
            shadow_diff = 0.2f;
            shadow_spec = 0.f;
        }

        double amb = 0.3; //环境光
        double diff = std::max(0.f,n*l);//漫反射
        double spec = std::pow(std::max(0.f,r.z),40);//高光

        for (int i= 0; i < 3; i++) {
            color[i] *= std::min(1., amb + shadow_diff * .4 * diff + shadow_spec * .9 * spec);
        }
        return {false, color};
    }


};

    int main() {

        int model_count;
        std::cout << "Enter model count: ";
        std::cin >> model_count;

        std::vector<std::string> paths(model_count);
        for (int i = 0; i < model_count; i++) {
            std::cout << "Enter model path " << i + 1 << ": ";
            std::cin >> paths[i];
        }

        std::vector<Model> models;
        for (int i = 0; i < model_count; i++) {
            models.emplace_back(paths[i]);
        }

        int width = 800;
        int height = 800;

        Vec3f light(1,1,1);
        Vec3f eye(-1,0,2);
        Vec3f center(0,0,0);
        Vec3f up(0,1,0);

        ModelView(light, center, up);
        Perspective((light - center).norm());
        Viewport(width/16,height/16,width * 7/8,height * 7/8);

        Mat4 M_Light = viewport * perspective * modelview;
        zbufferf.assign(width * height, -1e10f);
        TGAImage shadowImg(width, height, TGAImage::RGB);

        for (int m = 0; m < model_count; m++) {
            MapShader shadowShader(&models[m]);
            for (int i = 0; i < models[m].nfaces(); i++) {
                Vec4f face[3] = {shadowShader.Vertex(i,0), shadowShader.Vertex(i,1), shadowShader.Vertex(i,2)};
                Rasterize(shadowImg, face, shadowShader);
            }
        }

        std::vector<float> shadowBuffer = zbufferf;

        TGAImage img(width, height, TGAImage::RGB,{177, 195, 209, 255});

        ModelView(eye, center, up);
        Perspective((eye - center).norm());
        Viewport(width/16,height/16,width * 7/8,height * 7/8);
        Mat4 M_Shadow = M_Light * inverse(modelview);
        zbufferf.assign(width * height, -1e10f);

        for (int m = 0; m < model_count; m++) {
            PhongShader shader(&models[m], light, shadowBuffer, M_Shadow, width, height);
            for (int i = 0; i < models[m].nfaces(); i++) {
                Vec4f face[3] = {shader.Vertex(i,0), shader.Vertex(i,1), shader.Vertex(i,2)};
                Rasterize(img, face, shader);
            }
        }

        img.write_tga_file("output.tga");
        CreateZBufferImage(zbufferf, width, height).write_tga_file("zbuffer.tga");

        return 0;
}
