#ifndef UTILITY_H
#define UTILITY_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <string>

namespace SkyLink {
    namespace Utility {
        GLuint compileShader(GLenum type, const GLchar* source);
        GLuint createShaderProgram(const GLchar* vertexSource, const GLchar* fragmentSource);
        bool initializeGLFWandGLEW(GLFWwindow*& window, const int WIDTH, const int HEIGHT);
        std::string readFile(const std::string& filePath);
    }
}

#endif // UTILITY_H
