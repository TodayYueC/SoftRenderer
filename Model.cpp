#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "model.h"

Model::Model(const char *filename) : verts_(), norms_(), uvs_(), faces_(), face_norms_(), face_uvs_() {
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

        if (!line.compare(0, 3, "vn ")) {
            iss >> trash >> trash;
            Vec3f n;
            for (int i = 0; i < 3; i++) iss >> n[i];
            norms_.push_back(n);
        }
        else if (!line.compare(0, 3, "vt ")) {
            iss >> trash >> trash;
            Vec2f uv;
            iss >> uv.x >> uv.y;
            uvs_.push_back(uv);
        }
        else if (!line.compare(0, 2, "v ")) {
            iss >> trash;
            Vec3f v;
            for (int i = 0; i < 3; i++) iss >> v[i];
            verts_.push_back(v);
        }
        else if (!line.compare(0, 2, "f ")) {
            std::vector<int> f, fn, fuv;
            iss >> trash;
            for (int i = 0; i < 3; i++) {
                int vidx, vtidx = 0, vnidx = 0;
                iss >> vidx;
                if (iss.peek() == '/') {
                    iss >> trash;
                    if (iss.peek() == '/') {
                        iss >> trash >> vnidx;
                    } else {
                        iss >> vtidx;
                        if (iss.peek() == '/') {
                            iss >> trash >> vnidx;
                        }
                    }
                }
                f.push_back(vidx - 1);
                fn.push_back(vnidx - 1);
                fuv.push_back(vtidx - 1);
            }
            faces_.push_back(f);
            face_norms_.push_back(fn);
            face_uvs_.push_back(fuv);
        }
    }
    std::cerr << "# v# " << nverts() << " vt# " << uvs_.size()
              << " vn# " << nnorms() << " f# " << nfaces() << std::endl;

    std::string texfile(filename);
    int dot = texfile.find_last_of('.');
    if (dot != std::string::npos) {
        texfile.erase(dot);
        normalmap_.read_tga_file(texfile + "_nm_tangent.tga");
        normalmap_.flip_vertically();
        diffusemap_.read_tga_file(texfile + "_diffuse.tga");
        diffusemap_.flip_vertically();
    }
}

Model::Model(const std::string &filename) : Model(filename.c_str()) {
}

Model::~Model() {
}

int Model::nverts() const {
    return (int)verts_.size();
}

int Model::nnorms() const {
    return (int)norms_.size();
}

int Model::nfaces() const {
    return (int)faces_.size();
}

Vec3f Model::vert(int i) const {
    return verts_[i];
}

Vec3f Model::normal(int face, int vert) const {
    int idx = face_norms_[face][vert];
    return norms_[idx];
}

Vec2f Model::uv(int face, int vert) const {
    int idx = face_uvs_[face][vert];
    return uvs_[idx];
}

Vec3f Model::normal(Vec2f uv) const {
    TGAColor c = normalmap_.get(uv.x * normalmap_.width(), uv.y * normalmap_.height());
    Vec3f res;
    for (int i = 0; i < 3; i++)
        res[2 - i] = c[i] / 255.f * 2.f - 1.f;
    return res;
}

TGAColor Model::diffuse(Vec2f uv) const {
    return diffusemap_.get(uv.x * diffusemap_.width(), uv.y * diffusemap_.height());
}

std::vector<int> Model::face(int idx) const {
    return faces_[idx];
}

void Model::load_normal_map(const char *filename) {
    normalmap_.read_tga_file(filename);
    normalmap_.flip_vertically();
}

void Model::load_diffuse_map(const char *filename) {
    diffusemap_.read_tga_file(filename);
    diffusemap_.flip_vertically();
}
