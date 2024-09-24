#include "Renderer.h"
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H

namespace SkyLink {

    // Shader kaynak kodları
    const char* textVertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec4 vertex; // <vec2 pos, vec2 tex>
    out vec2 TexCoords;

    uniform mat4 projection;

    void main() {
        gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);
        TexCoords = vertex.zw;
    }
)";

    const char* textFragmentShaderSource = R"(
    #version 330 core
    in vec2 TexCoords;
    out vec4 color;

    uniform sampler2D text;
    uniform vec3 textColor;

    void main() {
        vec4 sampled = vec4(1.0, 1.0, 1.0,
                            texture(text, TexCoords).r);
        color = vec4(textColor, 1.0) * sampled;
    }
)";

    const char* triangleVertexShaderSource = R"(
    #version 330 core
    layout(location = 0) in vec2 aPos;

    uniform mat4 projection;

    void main() {
        gl_Position = projection * vec4(aPos, 0.0, 1.0);
    }
)";

    const char* triangleFragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;

    uniform vec3 triangleColor;

    void main() {
        FragColor = vec4(triangleColor, 1.0);
    }
)";

    Renderer::Renderer() {
        // Shader programlarını oluştur
        textShaderProgram = createShaderProgram(textVertexShaderSource, textFragmentShaderSource);
        triangleShaderProgram = createShaderProgram(triangleVertexShaderSource, triangleFragmentShaderSource);

        // Projeksiyon matrisi ayarla
        projection = glm::ortho(0.0f, static_cast<GLfloat>(WIDTH),
            0.0f, static_cast<GLfloat>(HEIGHT));

        // Metin shader programı için projeksiyon matrisini ayarla
        glUseProgram(textShaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(textShaderProgram, "projection"),
            1, GL_FALSE, glm::value_ptr(projection));

        // Üçgen için VAO ve VBO oluştur
        glGenVertexArrays(1, &triangleVAO);
        glGenBuffers(1, &triangleVBO);

        // FreeType ile karakterleri yükle
        loadCharacters("C:/Company/GroundControl/SkyLinkv1/orange_juice2.ttf");
    }

    Renderer::~Renderer() {
        // Kaynakları serbest bırak
        glDeleteVertexArrays(1, &triangleVAO);
        glDeleteBuffers(1, &triangleVBO);
        glDeleteProgram(textShaderProgram);
        glDeleteProgram(triangleShaderProgram);
    }

    void Renderer::clear() {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    // renderText fonksiyonunun implementasyonu
    void Renderer::renderText(const std::string& text, GLfloat x, GLfloat y,
        GLfloat scale, glm::vec3 color) {
        glUseProgram(textShaderProgram);
        glUniform3f(glGetUniformLocation(textShaderProgram, "textColor"), color.x, color.y, color.z);
        glActiveTexture(GL_TEXTURE0);

        GLuint VBO, VAO;
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4,
            NULL, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE,
            4 * sizeof(GLfloat), 0);

        std::string::const_iterator c;
        for (c = text.begin(); c != text.end(); c++) {
            Character ch = Characters[*c];

            GLfloat xpos = x + ch.BearingX * scale;
            GLfloat ypos = y - (ch.Height - ch.BearingY) * scale;

            GLfloat w = ch.Width * scale;
            GLfloat h = ch.Height * scale;

            GLfloat vertices[6][4] = {
                { xpos,     ypos + h,   0.0, 0.0 },
                { xpos,     ypos,       0.0, 1.0 },
                { xpos + w, ypos,       1.0, 1.0 },

                { xpos,     ypos + h,   0.0, 0.0 },
                { xpos + w, ypos,       1.0, 1.0 },
                { xpos + w, ypos + h,   1.0, 0.0 }
            };

            glBindTexture(GL_TEXTURE_2D, ch.TextureID);
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0,
                sizeof(vertices), vertices);

            glDrawArrays(GL_TRIANGLES, 0, 6);
            x += (ch.Advance >> 6) * scale;
        }
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);

        glDeleteBuffers(1, &VBO);
        glDeleteVertexArrays(1, &VAO);
    }

    // drawTriangle fonksiyonunun implementasyonu
    void Renderer::drawTriangle(float x, float y, float size, glm::vec3 color) {
        GLfloat vertices[] = {
            x,         y + size,  // Üst nokta
            x - size,  y - size,  // Sol alt
            x + size,  y - size   // Sağ alt
        };

        glUseProgram(triangleShaderProgram);

        // Projeksiyon matrisini gönder
        GLint projLoc = glGetUniformLocation(triangleShaderProgram, "projection");
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        // Üçgenin rengini ayarla
        GLint colorLoc = glGetUniformLocation(triangleShaderProgram, "triangleColor");
        glUniform3f(colorLoc, color.r, color.g, color.b);

        glBindVertexArray(triangleVAO);

        glBindBuffer(GL_ARRAY_BUFFER, triangleVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
            2 * sizeof(GLfloat), (void*)0);

        glDrawArrays(GL_TRIANGLES, 0, 3);

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    // createShaderProgram fonksiyonunun implementasyonu
    GLuint Renderer::createShaderProgram(const char* vertexSource, const char* fragmentSource) {
        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexSource, NULL);
        glCompileShader(vertexShader);

        GLint success;
        GLchar infoLog[512];
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
            std::cout << "Vertex Shader Hatası:\n" << infoLog << std::endl;
        }

        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
        glCompileShader(fragmentShader);
        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
            std::cout << "Fragment Shader Hatası:\n" << infoLog << std::endl;
        }

        GLuint shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);
        glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
            std::cout << "Shader Program Linkleme Hatası:\n" << infoLog << std::endl;
        }

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        return shaderProgram;
    }

    // loadCharacters fonksiyonunun implementasyonu
    void Renderer::loadCharacters(const std::string& fontPath) {
        FT_Library ft;
        if (FT_Init_FreeType(&ft)) {
            std::cout << "ERROR::FREETYPE: Could not init FreeType Library"
                << std::endl;
            return;
        }

        FT_Face face;
        if (FT_New_Face(ft, fontPath.c_str(), 0, &face)) {
            std::cout << "ERROR::FREETYPE: Failed to load font"
                << std::endl;
            return;
        }

        FT_Set_Pixel_Sizes(face, 0, 48);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        for (GLubyte c = 0; c < 128; c++) {
            if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
                std::cout << "ERROR::FREETYPE: Failed to load Glyph"
                    << std::endl;
                continue;
            }

            GLuint texture;
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexImage2D(
                GL_TEXTURE_2D, 0, GL_RED,
                face->glyph->bitmap.width, face->glyph->bitmap.rows,
                0, GL_RED, GL_UNSIGNED_BYTE,
                face->glyph->bitmap.buffer
            );

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                GL_LINEAR);

            Character character = {
                texture,
                face->glyph->bitmap.width,
                face->glyph->bitmap.rows,
                face->glyph->bitmap_left,
                face->glyph->bitmap_top,
                face->glyph->advance.x
            };
            Characters.insert(std::pair<GLchar, Character>(c, character));
        }

        FT_Done_Face(face);
        FT_Done_FreeType(ft);
    }

} // namespace SkyLine
