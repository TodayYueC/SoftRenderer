#ifndef __MODEL_H__
#define __MODEL_H__

#include <vector>
#include <string>
#include "geometry.h"

class Model {
private:
    std::vector<Vec3f> verts_;                              // 顶点坐标数组
    std::vector<Vec3f> norms_;                              // 法线坐标数组
    std::vector<std::vector<int>> faces_;                    // 面数组，每个面存3个顶点索引（三角形）
    std::vector<std::vector<int>> face_norms_;              // 面的法线索引数组

public:
    Model(const char *filename);                              // 从 .obj 文件加载模型
    Model(const std::string &filename);                       // 从 std::string 路径加载模型
    ~Model();

    int nverts() const;                                       // 返回顶点数量
    int nnorms() const;                                       // 返回法线数量
    int nfaces() const;                                       // 返回面（三角形）数量

    Vec3f vert(int i) const;                                  // 返回第 i 个顶点的坐标
    Vec3f normal(int face, int vert) const;                    // 返回指定面的第vert个顶点的法线
    std::vector<int> face(int idx) const;                     // 返回第 idx 个面的顶点索引数组
};

#endif //__MODEL_H__
