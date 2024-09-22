// Model.h
#pragma once

#include <vector>
#include <string>
#include <GL/glew.h>
#include <glm/glm.hpp>

namespace SkyLink {
    namespace Model {

        class Model {
        public:
            Model(const std::string& path);
            ~Model();

            void draw(GLuint shaderProgram);

        private:
            // Model data
            std::vector<glm::vec3> vertices;
            std::vector<glm::vec2> uvs;
            std::vector<glm::vec3> normals;

            // OpenGL data
            GLuint VAO, VBO_Vertices, VBO_UVs, VBO_Normals;

            bool loadOBJ(const std::string& path);
            void setupMesh();
        };

    } // namespace Model
} // namespace SkyLink
