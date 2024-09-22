#ifndef SKYLINK_MODEL_SHADER_H
#define SKYLINK_MODEL_SHADER_H

#include <string>
#include <glm/glm.hpp>

namespace SkyLink {
    namespace Model {

        class Shader {
        public:
            unsigned int ID;

            Shader(const char* vertexSource, const char* fragmentSource);
            ~Shader();

            void use();

            // Uniform değişken ayarlama fonksiyonları
            void setBool(const std::string& name, bool value) const;
            void setInt(const std::string& name, int value) const;
            void setFloat(const std::string& name, float value) const;
            void setVec3(const std::string& name, const glm::vec3& value) const;
            void setMat4(const std::string& name, const glm::mat4& mat) const;

        private:
            void checkCompileErrors(unsigned int shader, const std::string& type);
        };

    } // namespace Model
} // namespace SkyLink

#endif // SKYLINK_MODEL_SHADER_H
