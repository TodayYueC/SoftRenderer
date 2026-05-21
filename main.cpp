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
        Vec3f t = {T[0][0], T[0][1], T[0][2]};
        Vec3f b = {T[1][0], T[1][1], T[1][2]};
        Vec3f a2 = varying_normal[0] * bar.x + varying_normal[1] * bar.y + varying_normal[2] * bar.z;
        a2.normalize();
        t = t - a2 * (t * a2);
        t.normalize();
        b = b - a2 * (b * a2);
        b = b - t * (b * t);
        b.normalize();
        A[0] = {t.x, t.y, t.z, 0};
        A[1] = {b.x, b.y, b.z, 0};
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
        float bias = 0.05f;

        if (p_light.x >= 0 && p_light.x < shadow_width && p_light.y >= 0 && p_light.y < shadow_height) {
            int idx_x = (int)p_light.x;
            int idx_y = (int)p_light.y;
            int shadow_idx = idx_x + idx_y * shadow_width;
            if (p_light.z + bias < shadowBuffer[shadow_idx]) {
                shadow_diff = 0.05f;
                shadow_spec = 0.f;
            }
        }

        double amb = 0.35; //环境光
        double diff = std::max(0.f,n*l);//漫反射
        Vec3f v_dir = -p_view.toVec3().normalize();
        double spec = std::pow(std::max(0.f, r * v_dir), 40);
        for (int i= 0; i < 3; i++) {
            color[i] *= std::min(1., amb + 2 * shadow_diff * diff + shadow_spec * 2 * spec);
        }
        return {false, color};
    }


};

void ApplySSAO(TGAImage &img, const std::vector<float> &zbuffer, int width, int height) {
    // 准备一个随机数发生器，用来向四周撒“侦察兵”
    std::default_random_engine generator(2026); // 固定随机种子保证渲染稳定
    std::uniform_real_distribution<float> random_floats(0.0, 1.0);

    // 1. 遍历屏幕上的每一个像素 (x, y)
    #pragma omp parallel for // 如果你开了 OpenMP 加速，这行可以让后处理飞快
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {

            // 拿到当前中心像素的深度值
            float center_z = zbuffer[x + y * width];

            // 如果踩到了背景（-1e10f），说明这里是虚空，直接跳过不需要遮蔽
            if (center_z < -1e9f) continue;

            float total_occlusion = 0.0f;
            int sample_count = 16;  // 每一个像素周围撒 16 个采样点（侦察兵）
            float radius = 15.0f;   // 采样半径（单位：像素），可以决定缝隙阴影的粗细

            // 2. 开始向周围随机撒点
            for (int i = 0; i < sample_count; i++) {
                // 随机生成一个圆周方向的夹角
                float angle = random_floats(generator) * 2.0f * M_PI;
                // 随机生成一个半径距离
                float r = random_floats(generator) * radius;

                // 算出侦察兵在屏幕上的 2D 坐标
                int sample_x = x + (int)(r * cos(angle));
                int sample_y = y + (int)(r * sin(angle));

                // 越界保护：如果侦察兵跑出了屏幕，就不算数
                if (sample_x < 0 || sample_x >= width || sample_y < 0 || sample_y >= height) continue;

                // 3. 查一查侦察兵踩到的几何体表面深度
                float neighbor_z = zbuffer[sample_x + sample_y * width];

                // 【核心几何大比拼】：
                // 因为你的 zbuffer 越大越近。如果邻居的 neighbor_z 明显大于（更靠近相机）你的 center_z，
                // 说明邻居是一堵“高墙”，把你围在里面了！
                if (neighbor_z > center_z) {
                    // 距离判定：防止隔得很远处的物体（比如前面飞过的一根手指）误遮蔽了远处的墙面
                    float depth_diff = neighbor_z - center_z;
                    if (depth_diff < 15.0f) { // 深度差异阈值，超过这个距离说明不是同一个转折缝隙
                        total_occlusion += 1.0f;
                    }
                }
            }

            // 4. 计算最终的遮蔽因子 (0.0 代表完全黑死，1.0 代表完全开阔)
            float occlusion_factor = total_occlusion / (float)sample_count;

            // 这里的 0.65f 是遮蔽强度，值越大，夹缝处黑得越狠，画面越硬朗
            float shadow_factor = 1.0f - occlusion_factor * 0.65f;

            // 5. 物理涂黑：读取原本的像素颜色，乘以遮蔽因子
            TGAColor color = img.get(x, y);
            for (int c = 0; c < 3; c++) {
                color[c] = (int)(color[c] * shadow_factor);
            }
            // 写回画布
            img.set(x, y, color);
        }
    }
}

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

        int width = 1080;
        int height = 1080;

        Vec3f light(0.5,1,1);
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
        ApplySSAO(img, zbufferf, width, height);
        img.write_tga_file("output.tga");
        CreateZBufferImage(zbufferf, width, height).write_tga_file("zbuffer.tga");

        return 0;
}
