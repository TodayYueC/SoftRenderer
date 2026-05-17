#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "model.h"

Model::Model(const char *filename) : verts_(), faces_() {
    std::ifstream in;
    in.open(filename, std::ifstream::in);
    if (in.fail()) {
        std::cerr << "Failed to open model file: " << filename << std::endl;
        return;
    }

    std::string line;
    while (!in.eof()) {
        std::getline(in, line);
        std::istringstream iss(line.c_str());
        char trash;

        // 解析顶点行："v x y z"
        if (!line.compare(0, 2, "v ")) {
            iss >> trash;           // 读取 'v'
            Vec3f v;
            for (int i = 0; i < 3; i++) iss >> v[i];
            verts_.push_back(v);
        }
        // 解析面行："f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3"
        // 也可以简化为 "f v1 v2 v3" 或 "f v1//vn1 v2//vn2 v3//vn3"
        else if (!line.compare(0, 2, "f ")) {
            std::vector<int> f;
            int itrash, idx;
            iss >> trash;           // 读取 'f'

            // 读取 3 个顶点索引（只取顶点索引，忽略纹理和法线索引）
            for (int i = 0; i < 3; i++) {
                iss >> idx;
                // OBJ 文件的索引是 1-based，转换为 C++ 的 0-based
                idx--;
                f.push_back(idx);

                // 跳过 "/vt/vn" 或 "//vn" 格式中的纹理索引和法线索引
                if (iss.peek() == '/') {
                    iss >> trash;   // 读取 '/'
                    if (iss.peek() == '/') {
                        iss >> trash; // 读取第二个 '/'
                        iss >> itrash; // 跳过法线索引
                    } else {
                        iss >> itrash; // 跳过纹理索引
                        if (iss.peek() == '/') {
                            iss >> trash; // 读取 '/'
                            iss >> itrash; // 跳过法线索引
                        }
                    }
                }
            }
            faces_.push_back(f);
        }
    }
    std::cerr << "# v# " << nverts() << " f# " << nfaces() << std::endl;
}

Model::Model(const std::string &filename) : Model(filename.c_str()) {
}

Model::~Model() {
}

int Model::nverts() const {
    return (int)verts_.size();
}

int Model::nfaces() const {
    return (int)faces_.size();
}

Vec3f Model::vert(int i) const {
    return verts_[i];
}

std::vector<int> Model::face(int idx) const {
    return faces_[idx];
}
