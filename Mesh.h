#ifndef SKYLINK_MODEL_MESH_H
#define SKYLINK_MODEL_MESH_H

#include <vector>
#include <glm/glm.hpp>

namespace SkyLink {
    namespace Model {

        class Mesh {
        public:
            // Mesh verileri
            std::vector<float> vertices;
            std::vector<unsigned int> indices;
            glm::vec3 diffuseColor;

            Mesh();
            ~Mesh();

            void setupMesh();
            void draw();

        private:
            unsigned int VAO, VBO, EBO;
        };

    } // namespace Model
} // namespace SkyLink

#endif // SKYLINK_MODEL_MESH_H
