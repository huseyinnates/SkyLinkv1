#ifndef SKYLINE_RENDERER_H
#define SKYLINE_RENDERER_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <map>
#include <string>

namespace SkyLink {

    const GLuint WIDTH = 800, HEIGHT = 600;

    class Renderer {
    public:
        struct Character {
            GLuint TextureID;
            GLuint Width;
            GLuint Height;
            GLint BearingX;
            GLint BearingY;
            GLuint Advance;
        };
        std::map<GLchar, Character> Characters;

        glm::mat4 projection;

        GLuint textShaderProgram;
        GLuint triangleShaderProgram;
        GLuint triangleVAO, triangleVBO;

        Renderer();
        ~Renderer();

        void clear();
        void renderText(const std::string& text, GLfloat x, GLfloat y,
            GLfloat scale, glm::vec3 color);
        void drawTriangle(float x, float y, float size, glm::vec3 color);

    private:
        GLuint createShaderProgram(const char* vertexSource, const char* fragmentSource);
        void loadCharacters(const std::string& fontPath);
    };

} // namespace SkyLine

#endif // SKYLINE_RENDERER_H
