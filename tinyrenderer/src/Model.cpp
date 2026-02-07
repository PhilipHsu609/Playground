#include "tinyrenderer/Model.hpp"

#include <fstream>
#include <sstream>
#include <string>

Model::Model(const char *filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file");
    }

    std::string line;
    while (!file.eof()) {
        std::getline(file, line);
        std::istringstream ss(line);
        char c{};

        if (line[0] == 'v') {
            if (line[1] == ' ') {
                ss >> c;
                Vec3f v;
                ss >> v[0] >> v[1] >> v[2];
                verts_.push_back(v);
            }
        } else if (line[0] == 'f') {
            std::vector<size_t> f;

            size_t vidx{}; // vertex index
            size_t tidx{}; // texture index
            size_t nidx{}; // normal index

            ss >> c;
            while (ss >> vidx >> c >> tidx >> c >> nidx) {
                f.push_back(vidx - 1);
            }
            faces_.push_back(f);
        }
    }
}