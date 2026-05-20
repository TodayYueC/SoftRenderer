#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "model.h"

Model::Model(const char *filename) : verts_(), norms_(), faces_(), face_norms_() {
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
        else if (!line.compare(0, 2, "v ")) {
            iss >> trash;
            Vec3f v;
            for (int i = 0; i < 3; i++) iss >> v[i];
            verts_.push_back(v);
        }
        else if (!line.compare(0, 2, "f ")) {
            std::vector<int> f, fn;
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
            }
            faces_.push_back(f);
            face_norms_.push_back(fn);
        }
    }
    std::cerr << "# v# " << nverts() << " vn# " << nnorms() << " f# " << nfaces() << std::endl;
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

std::vector<int> Model::face(int idx) const {
    return faces_[idx];
}
