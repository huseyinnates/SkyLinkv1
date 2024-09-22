// Model.cpp
#include "Model.h"
#include <iostream>
#include <fstream>
#include <sstream>

namespace SkyLink {
    namespace Model {

        Model::Model(const std::string& path) : VAO(0), VBO_Vertices(0), VBO_UVs(0), VBO_Normals(0) {
            if (loadOBJ(path)) {
                setupMesh();
            }
            else {
                std::cerr << "Failed to load model: " << path << std::endl;
            }
        }

        Model::~Model() {
            glDeleteVertexArrays(1, &VAO);
            glDeleteBuffers(1, &VBO_Vertices);
            glDeleteBuffers(1, &VBO_UVs);
            glDeleteBuffers(1, &VBO_Normals);
        }

        bool Model::loadOBJ(const std::string& path) {
            std::vector<unsigned int> vertexIndices, uvIndices, normalIndices;
            std::vector<glm::vec3> temp_vertices;
            std::vector<glm::vec2> temp_uvs;
            std::vector<glm::vec3> temp_normals;

            std::ifstream file(path);
            if (!file.is_open()) {
                std::cerr << "Cannot open file: " << path << std::endl;
                return false;
            }

            std::string line;
            while (std::getline(file, line)) {
                std::istringstream ss(line);
                std::string prefix;
                ss >> prefix;

                if (prefix == "v") {
                    glm::vec3 vertex;
                    ss >> vertex.x >> vertex.y >> vertex.z;
                    temp_vertices.push_back(vertex);
                }
                else if (prefix == "vt") {
                    glm::vec2 uv;
                    ss >> uv.x >> uv.y;
                    uv.y = 1.0f - uv.y; // Flip UV coordinate
                    temp_uvs.push_back(uv);
                }
                else if (prefix == "vn") {
                    glm::vec3 normal;
                    ss >> normal.x >> normal.y >> normal.z;
                    temp_normals.push_back(normal);
                }
                else if (prefix == "f") {
                    unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];
                    char slash;

                    for (int i = 0; i < 3; ++i) {
                        ss >> vertexIndex[i] >> slash >> uvIndex[i] >> slash >> normalIndex[i];
                        vertexIndices.push_back(vertexIndex[i]);
                        uvIndices.push_back(uvIndex[i]);
                        normalIndices.push_back(normalIndex[i]);
                    }
                }
            }

            // Reconstruct the data
            for (unsigned int i = 0; i < vertexIndices.size(); ++i) {
                unsigned int vertexIndex = vertexIndices[i];
                glm::vec3 vertex = temp_vertices[vertexIndex - 1];
                vertices.push_back(vertex);
            }

            for (unsigned int i = 0; i < uvIndices.size(); ++i) {
                unsigned int uvIndex = uvIndices[i];
                glm::vec2 uv = temp_uvs[uvIndex - 1];
                uvs.push_back(uv);
            }

            for (unsigned int i = 0; i < normalIndices.size(); ++i) {
                unsigned int normalIndex = normalIndices[i];
                glm::vec3 normal = temp_normals[normalIndex - 1];
                normals.push_back(normal);
            }

            return true;
        }

        void Model::setupMesh() {
            glGenVertexArrays(1, &VAO);
            glGenBuffers(1, &VBO_Vertices);
            glGenBuffers(1, &VBO_UVs);
            glGenBuffers(1, &VBO_Normals);

            glBindVertexArray(VAO);

            // Vertices
            glBindBuffer(GL_ARRAY_BUFFER, VBO_Vertices);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);
            glEnableVertexAttribArray(0); // Layout location 0 in shader
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

            // UVs
            if (!uvs.empty()) {
                glBindBuffer(GL_ARRAY_BUFFER, VBO_UVs);
                glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(glm::vec2), &uvs[0], GL_STATIC_DRAW);
                glEnableVertexAttribArray(1); // Layout location 1 in shader
                glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
            }

            // Normals
            if (!normals.empty()) {
                glBindBuffer(GL_ARRAY_BUFFER, VBO_Normals);
                glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0], GL_STATIC_DRAW);
                glEnableVertexAttribArray(2); // Layout location 2 in shader
                glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
            }

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
        }

        void Model::draw(GLuint shaderProgram) {
            glUseProgram(shaderProgram);
            glBindVertexArray(VAO);

            // Draw the model
            glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()));

            glBindVertexArray(0);
            glUseProgram(0);
        }

    } // namespace Model
} // namespace SkyLink
