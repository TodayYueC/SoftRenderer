#ifndef __MODEL_H__
#define __MODEL_H__

#include <vector>
#include <string>
#include "geometry.h"
#include "tgaimage.h"

class Model {
private:
    std::vector<Vec3f> verts_;                              // 顶点坐标数组
    std::vector<Vec3f> norms_;                              // 法线坐标数组
    std::vector<Vec2f> uvs_;                                // 纹理坐标数组
    std::vector<std::vector<int>> faces_;                    // 面数组，每个面存3个顶点索引（三角形）
    std::vector<std::vector<int>> face_norms_;              // 面的法线索引数组
    std::vector<std::vector<int>> face_uvs_;               // 面的纹理坐标索引数组
    TGAImage diffusemap_;                                     // 漫反射贴图
    TGAImage normalmap_;                                     // 法线贴图

public:
    Model(const char *filename);                              // 从 .obj 文件加载模型
    Model(const std::string &filename);                       // 从 std::string 路径加载模型
    ~Model();

    int nverts() const;                                       // 返回顶点数量
    int nnorms() const;                                       // 返回法线数量
    int nfaces() const;                                       // 返回面（三角形）数量

    Vec3f vert(int i) const;                                  // 返回第 i 个顶点的坐标
    Vec3f normal(int face, int vert) const;                    // 返回指定面的第vert个顶点的法线
    Vec2f uv(int face, int vert) const;                         // 返回指定面的第vert个顶点的纹理坐标
    Vec3f normal(Vec2f uv) const;                               // 从法线贴图采样法线（对象空间）
    TGAColor diffuse(Vec2f uv) const;                           // 从漫反射贴图采样颜色
    std::vector<int> face(int idx) const;                     // 返回第 idx 个面的顶点索引数组

    void load_normal_map(const char *filename);                 // 加载法线贴图
    void load_diffuse_map(const char *filename);                // 加载漫反射贴图
};

#endif //__MODEL_H__
