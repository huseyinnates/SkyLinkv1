#include "Utility.h"
#include <iostream>
#include <fstream>
#include <sstream>

namespace SkyLink {
    namespace Utility {
        GLuint compileShader(GLenum type, const GLchar* source) {
            GLuint shader = glCreateShader(type);
            glShaderSource(shader, 1, &source, NULL);
            glCompileShader(shader);

            // Check compilation status
            GLint success;
            GLchar infoLog[512];
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetShaderInfoLog(shader, 512, NULL, infoLog);
                std::cout << "Shader compilation error:\n" << infoLog << std::endl;
            }

            return shader;
        }

        GLuint createShaderProgram(const GLchar* vertexSource, const GLchar* fragmentSource) {
            GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
            GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);

            // Create shader program
            GLuint shaderProgram = glCreateProgram();
            glAttachShader(shaderProgram, vertexShader);
            glAttachShader(shaderProgram, fragmentShader);
            glLinkProgram(shaderProgram);

            // Check linking status
            GLint success;
            GLchar infoLog[512];
            glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
                std::cout << "Shader linking error:\n" << infoLog << std::endl;
            }

            // Delete shaders as they are linked into the program now and no longer necessary
            glDeleteShader(vertexShader);
            glDeleteShader(fragmentShader);

            return shaderProgram;
        }

        bool initializeGLFWandGLEW(GLFWwindow*& window, const int WIDTH, const int HEIGHT) {
            // Initialize GLFW
            if (!glfwInit()) {
                std::cout << "Failed to initialize GLFW" << std::endl;
                return false;
            }
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

            // Create window
            window = glfwCreateWindow(WIDTH, HEIGHT, "Grid System", NULL, NULL);
            if (!window) {
                std::cout << "Failed to create GLFW window" << std::endl;
                glfwTerminate();
                return false;
            }
            glfwMakeContextCurrent(window);

            // Initialize GLEW
            glewExperimental = GL_TRUE;
            if (glewInit() != GLEW_OK) {
                std::cout << "Failed to initialize GLEW" << std::endl;
                return false;
            }

            glEnable(GL_CULL_FACE);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            return true;
        }

        std::string readFile(const std::string& filePath) {
            std::ifstream file(filePath);
            if (!file.is_open()) {
                std::cout << "Failed to open file: " << filePath << std::endl;
                return "";
            }
            std::stringstream buffer;
            buffer << file.rdbuf();
            return buffer.str();
        }
    }
}
