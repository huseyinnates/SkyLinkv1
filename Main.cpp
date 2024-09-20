#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <map>
#include <vector>
#include <algorithm>

// Ekran boyutları
const GLuint WIDTH = 1920, HEIGHT = 1080;

// Grid Sistemi için Sınıf
class GridSystem {
public:
    struct Character {
        GLuint TextureID;   // Karakterin texture ID'si
        GLuint Width;       // Karakterin genişliği
        GLuint Height;      // Karakterin yüksekliği
        GLint BearingX;     // Karakterin sol/üst ofseti
        GLint BearingY;     // Karakterin sol/üst ofseti
        GLuint Advance;     // Sonraki karaktere geçiş mesafesi
    };

    struct GridCell {
        float x, y;         // Hücre koordinatları
        float width, height; // Hücre boyutları
        std::string text;   // Hücredeki metin
        bool active;        // Hücre aktif mi?

        GridCell(float x, float y, float width, float height)
            : x(x), y(y), width(width), height(height), active(false) {}
    };

    std::map<GLchar, Character> Characters; // Karakterleri tutan harita
    std::vector<GridCell> cells;  // Tüm hücreleri tutan vektör
    GLuint textShaderProgram;     // Metin için shader programı
    GLuint triangleShaderProgram; // Üçgen için shader programı
    GLuint triangleVAO, triangleVBO; // Üçgen için VAO ve VBO
    glm::mat4 projection;          // Projeksiyon matrisi
    int rows, cols;               // Satır ve sütun sayısı
    float cellWidth, cellHeight;  // Her bir hücrenin boyutları

    GridSystem(int rows, int cols, std::string fontPath)
        : rows(rows), cols(cols) {
        cellWidth = WIDTH / cols;
        cellHeight = HEIGHT / rows;

        // Shader programlarını oluştur
        textShaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);
        triangleShaderProgram = createShaderProgram(triangleVertexShaderSource, triangleFragmentShaderSource);

        // FreeType başlat ve karakterleri yükle
        loadCharacters(fontPath);

        // Hücreleri oluştur
        createCells();

        // Projeksiyon matrisi ayarla
        projection = glm::ortho(0.0f, static_cast<GLfloat>(WIDTH), 0.0f, static_cast<GLfloat>(HEIGHT));

        // Metin shader programı için projeksiyon matrisini ayarla
        glUseProgram(textShaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(textShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        // Üçgen için VAO ve VBO oluştur
        glGenVertexArrays(1, &triangleVAO);
        glGenBuffers(1, &triangleVBO);
    }

    ~GridSystem() {
        // Kaynakları serbest bırak
        glDeleteVertexArrays(1, &triangleVAO);
        glDeleteBuffers(1, &triangleVBO);
        glDeleteProgram(textShaderProgram);
        glDeleteProgram(triangleShaderProgram);
    }

    // Metin için shader kaynak kodları
    const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec4 vertex; // <vec2 pos, vec2 tex>
        out vec2 TexCoords;

        uniform mat4 projection;

        void main() {
            gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);
            TexCoords = vertex.zw;
        }
    )";

    const char* fragmentShaderSource = R"(
        #version 330 core
        in vec2 TexCoords;
        out vec4 color;

        uniform sampler2D text;
        uniform vec3 textColor;

        void main() {
            vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);
            color = vec4(textColor, 1.0) * sampled;
        }
    )";

    // Üçgen için shader kaynak kodları
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

    // Shader programı oluşturma fonksiyonu
    GLuint createShaderProgram(const char* vertexSource, const char* fragmentSource) {
        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexSource, NULL);
        glCompileShader(vertexShader);

        GLint success;
        GLchar infoLog[512];
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
        }

        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
        glCompileShader(fragmentShader);
        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
        }

        GLuint shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);
        glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
        }

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        return shaderProgram;
    }

    // FreeType ile karakterleri yükleme fonksiyonu
    void loadCharacters(std::string fontPath) {
        FT_Library ft;
        if (FT_Init_FreeType(&ft)) {
            std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
            return;
        }

        FT_Face face;
        if (FT_New_Face(ft, fontPath.c_str(), 0, &face)) {
            std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
            return;
        }

        FT_Set_Pixel_Sizes(face, 0, 48);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        for (GLubyte c = 0; c < 128; c++) {
            if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
                std::cout << "ERROR::FREETYPE: Failed to load Glyph" << std::endl;
                continue;
            }

            GLuint texture;
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RED,
                face->glyph->bitmap.width,
                face->glyph->bitmap.rows,
                0,
                GL_RED,
                GL_UNSIGNED_BYTE,
                face->glyph->bitmap.buffer
            );

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

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

    // Hücreleri oluşturma fonksiyonu
    void createCells() {
        for (int i = 0; i < rows; ++i) {
            for (int j = 0; j < cols; ++j) {
                float x = j * cellWidth;
                float y = i * cellHeight;
                cells.push_back(GridCell(x, y, cellWidth, cellHeight));
            }
        }
    }

    // Tüm hücreleri çizme fonksiyonu
    void drawGrid() {
        for (auto& cell : cells) {
            drawCell(cell);
        }
    }

    // Hücreyi çizme fonksiyonu
    void drawCell(GridCell& cell) {
        // Hücreye üçgen çiz
        drawTriangle(cell);

        // Hücrede metin varsa çiz
        if (!cell.text.empty()) {
            renderText(cell.text, cell.x + 10, cell.y + cell.height - 30, 0.5f, glm::vec3(1.0f, 1.0f, 1.0f));
        }
    }

    // Hücreye üçgen çizme fonksiyonu
    void drawTriangle(GridCell& cell) {
        // Üçgenin merkezini ve boyutunu hesapla
        GLfloat x = cell.x;
        GLfloat y = cell.y;
        GLfloat w = cell.width;
        GLfloat h = cell.height;

        GLfloat centerX = x + w / 2.0f;
        GLfloat centerY = y + h / 2.0f;

        GLfloat size = std::min(w, h) * 0.4f; // Üçgenin boyutu

        GLfloat vertices[] = {
            centerX,         centerY + size,  // Üst nokta
            centerX - size,  centerY - size,  // Sol alt
            centerX + size,  centerY - size   // Sağ alt
        };

        glUseProgram(triangleShaderProgram);

        // Projeksiyon matrisini gönder
        GLint projLoc = glGetUniformLocation(triangleShaderProgram, "projection");
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        // Üçgenin rengini ayarla
        GLint colorLoc = glGetUniformLocation(triangleShaderProgram, "triangleColor");
        glUniform3f(colorLoc, 1.0f, 0.0f, 0.0f); // Kırmızı renk

        glBindVertexArray(triangleVAO);

        glBindBuffer(GL_ARRAY_BUFFER, triangleVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (void*)0);

        glDrawArrays(GL_TRIANGLES, 0, 3);

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    // Metni render etme fonksiyonu
    void renderText(std::string text, GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color) {
        glUseProgram(textShaderProgram);
        glUniform3f(glGetUniformLocation(textShaderProgram, "textColor"), color.x, color.y, color.z);
        glActiveTexture(GL_TEXTURE0);

        GLuint VBO, VAO;
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);

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
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

            glDrawArrays(GL_TRIANGLES, 0, 6);
            x += (ch.Advance >> 6) * scale;
        }
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);

        glDeleteBuffers(1, &VBO);
        glDeleteVertexArrays(1, &VAO);
    }

    // Tıklanan hücreyi kontrol etme fonksiyonu
    void checkClick(float mouseX, float mouseY) {
        for (auto& cell : cells) {
            if (isCellClicked(cell, mouseX, mouseY)) {
                cell.active = true;
                cell.text = "Clicked!";
            }
            else {
                cell.active = false;
            }
        }
    }

    // Hücreye tıklanıp tıklanmadığını kontrol etme fonksiyonu
    bool isCellClicked(GridCell& cell, float mouseX, float mouseY) {
        return (mouseX >= cell.x && mouseX <= cell.x + cell.width && mouseY >= cell.y && mouseY <= cell.y + cell.height);
    }

    // Girdi işleme fonksiyonu
    void handleInput(int key) {
        for (auto& cell : cells) {
            if (cell.active) {
                if (key == GLFW_KEY_ENTER) {
                    cell.text = "Enter Pressed";
                }
            }
        }
    }
};

int main() {
    if (!glfwInit()) {
        std::cout << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Grid System", NULL, NULL);
    if (!window) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cout << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Grid sistemi oluştur ve font dosyasını yükle
    GridSystem grid(4, 4, "C:/Company/GroundControl/SkyLink/orange_juice2.ttf");

    // Hücreleri düzenle ve metin ekle
    if (!grid.cells.empty()) {
        grid.cells[0].text = "Cell 0";        // 1. Hücreye metin ekle
        grid.cells[1].text = "Cell 1";        // 2. Hücreye metin ekle
        grid.cells[5].text = "Hello World!";  // 6. Hücreye metin ekle
        grid.cells[10].text = "OpenGL Grid";  // 11. Hücreye metin ekle
        grid.cells[15].text = "GLFW Example"; // 16. Hücreye metin ekle
    }

    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Grid çizimi
        grid.drawGrid();

        // Girdi işleme
        if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS) {
            grid.handleInput(GLFW_KEY_ENTER);
        }

        // Fare tıklaması kontrolü
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            grid.checkClick(static_cast<float>(xpos), HEIGHT - static_cast<float>(ypos));
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
