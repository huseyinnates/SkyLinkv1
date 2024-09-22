#include "Mesh.h"
#include <GL/glew.h>

namespace SkyLink {
    namespace Model {

        Mesh::Mesh() : VAO(0), VBO(0), EBO(0) {}

        Mesh::~Mesh() {
            // Kaynakları serbest bırak
            if (EBO) glDeleteBuffers(1, &EBO);
            if (VBO) glDeleteBuffers(1, &VBO);
            if (VAO) glDeleteVertexArrays(1, &VAO);
        }

        void Mesh::setupMesh() {
            // OpenGL bufferlarını oluştur ve verileri yükle
            glGenVertexArrays(1, &VAO);
            glGenBuffers(1, &VBO);
            glGenBuffers(1, &EBO);

            glBindVertexArray(VAO);

            // Vertex verileri
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);

            // İndeks verileri
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

            // Vertex konumları
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);

            // Normaller
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
            glEnableVertexAttribArray(1);

            glBindVertexArray(0);
        }

        void Mesh::draw() {
            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }

    } // namespace Model
} // namespace SkyLink
