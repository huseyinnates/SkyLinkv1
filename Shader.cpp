#include "Shader.h"
#include <GL/glew.h>
#include <iostream>

namespace SkyLink {
    namespace Model {

        Shader::Shader(const char* vertexSource, const char* fragmentSource) {
            // Shader derleme ve program oluşturma
            unsigned int vertex, fragment;

            // Vertex Shader
            vertex = glCreateShader(GL_VERTEX_SHADER);
            glShaderSource(vertex, 1, &vertexSource, NULL);
            glCompileShader(vertex);
            checkCompileErrors(vertex, "VERTEX");

            // Fragment Shader
            fragment = glCreateShader(GL_FRAGMENT_SHADER);
            glShaderSource(fragment, 1, &fragmentSource, NULL);
            glCompileShader(fragment);
            checkCompileErrors(fragment, "FRAGMENT");

            // Shader Program
            ID = glCreateProgram();
            glAttachShader(ID, vertex);
            glAttachShader(ID, fragment);
            glLinkProgram(ID);
            checkCompileErrors(ID, "PROGRAM");

            // Shaderları sil
            glDeleteShader(vertex);
            glDeleteShader(fragment);
        }

        Shader::~Shader() {
            glDeleteProgram(ID);
        }

        void Shader::use() {
            glUseProgram(ID);
        }

        // Uniform değişken ayarlama fonksiyonları
        void Shader::setBool(const std::string& name, bool value) const {
            glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
        }
        void Shader::setInt(const std::string& name, int value) const {
            glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
        }
        void Shader::setFloat(const std::string& name, float value) const {
            glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
        }
        void Shader::setVec3(const std::string& name, const glm::vec3& value) const {
            glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
        }
        void Shader::setMat4(const std::string& name, const glm::mat4& mat) const {
            glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
        }

        void Shader::checkCompileErrors(unsigned int shader, const std::string& type) {
            // Derleme hatalarını kontrol et
            int success;
            char infoLog[1024];
            if (type != "PROGRAM") {
                glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
                if (!success) {
                    glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                    std::cerr << "| ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n"
                        << infoLog << "\n -- --------------------------------------------------- -- "
                        << std::endl;
                }
            }
            else {
                glGetProgramiv(shader, GL_LINK_STATUS, &success);
                if (!success) {
                    glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                    std::cerr << "| ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n"
                        << infoLog << "\n -- --------------------------------------------------- -- "
                        << std::endl;
                }
            }
        }

    } // namespace Model
} // namespace SkyLink
