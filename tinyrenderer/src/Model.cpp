#include "tinyrenderer/Model.hpp"

#include <array>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

Model::Model(const char *filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file");
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream ss(line);
        char c{};

        if (line.starts_with("v ")) {
            ss >> c; // consume 'v'
            Vec3f v;
            ss >> v.x() >> v.y() >> v.z();
            verts_.push_back(v);
        } else if (line.starts_with("vn ")) {
            ss >> c >> c; // consume 'v' 'n'
            Vec3f n;
            ss >> n.x() >> n.y() >> n.z();
            normals_.push_back(n);
        } else if (line.starts_with("vt ")) {
            ss >> c >> c; // consume 'v' 't'
            Vec2f t;
            ss >> t.x() >> t.y(); // ignore optional 3rd (w) component
            texCoords_.push_back(t);
        } else if (line.starts_with("f ")) {
            ss >> c; // consume 'f'

            std::array<FaceCorner, 3> face{};
            size_t cornerIdx = 0;

            size_t vidx{}; // vertex index
            size_t tidx{}; // texture index
            size_t nidx{}; // normal index
            while (ss >> vidx >> c >> tidx >> c >> nidx) {
                if (cornerIdx >= 3) {
                    throw std::runtime_error("Only triangular faces are supported");
                }
                face[cornerIdx++] = {vidx - 1, tidx - 1, nidx - 1};
            }
            if (cornerIdx != 3) {
                throw std::runtime_error("Face must have exactly 3 corners");
            }

            faces_.push_back(face);
        }
    }
}
