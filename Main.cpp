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
#include <memory>
#include <functional>
#include <algorithm>
#include <random>
#include <string>

 
const GLuint WIDTH = 800, HEIGHT = 600;

 
class GridCell;
class Renderer;
class DataProvider;
 
class Observer {
public:
    virtual void onDataUpdated(int newData) = 0;
};


class Subject {
private:
    std::vector<Observer*> observers;
public:
    void attach(Observer* observer) {
        observers.push_back(observer);
    }
    void detach(Observer* observer) {
        observers.erase(
            std::remove(observers.begin(), observers.end(), observer),
            observers.end());
    }
    void notify(int newData) {
        for (auto observer : observers) {
            observer->onDataUpdated(newData);
        }
    }
};

 
class CellStrategy {
public:
    virtual void update(GridCell& cell) = 0;
    virtual void draw(GridCell& cell, Renderer& renderer) = 0;
    virtual ~CellStrategy() = default;
};

// Basit Üçgen Çizme Stratejisi
class TriangleCellStrategy : public CellStrategy {
public:
    void update(GridCell& cell) override {
        // Canlı veri güncellemeleri burada işlenebilir
    }
    void draw(GridCell& cell, Renderer& renderer) override;
};

// GridCell Sınıfı (Observer ve Strategy Deseni Uygulaması)
class GridCell : public Observer {
public:
    float x, y;         // Hücre koordinatları
    float width, height; // Hücre boyutları
    bool active;        // Hücre aktif mi?
    std::string text;   // Hücredeki metin
    int data;           // Hücreye gelen veri

    // Strategy için pointer
    std::unique_ptr<CellStrategy> strategy;

    // Veri sağlayıcı
    DataProvider* dataProvider;

    GridCell(float x, float y, float width, float height)
        : x(x), y(y), width(width), height(height),
        active(false), data(0), dataProvider(nullptr) {}

    void setStrategy(CellStrategy* strat) {
        strategy.reset(strat);
    }

    void update() {
        if (strategy) {
            strategy->update(*this);
        }
    }

    void draw(Renderer& renderer) {
        if (strategy) {
            strategy->draw(*this, renderer);
        }
    }

    void onDataUpdated(int newData) override {
        // Veri güncellendiğinde yapılacak işlemler
        data = newData;
        text = "Data: " + std::to_string(data);
    }
};

// DataProvider Sınıfı (Subject Deseni)
class DataProvider : public Subject {
public:
    // Verileri saklayabilir
    int data;

    void setData(int newData) {
        data = newData;
        notify(data); // Veri güncellendiğinde gözlemcilere haber ver
    }
};

// Renderer Sınıfı
class Renderer {
public:
    // Shader Programları
    GLuint textShaderProgram;
    GLuint triangleShaderProgram;

    // Font karakterleri
    struct Character {
        GLuint TextureID;   // Karakterin texture ID'si
        GLuint Width;       // Karakterin genişliği
        GLuint Height;      // Karakterin yüksekliği
        GLint BearingX;     // Karakterin sol/üst ofseti
        GLint BearingY;     // Karakterin sol/üst ofseti
        GLuint Advance;     // Sonraki karaktere geçiş mesafesi
    };
    std::map<GLchar, Character> Characters;

    // Projeksiyon matrisi
    glm::mat4 projection;

    // Üçgen için VAO ve VBO
    GLuint triangleVAO, triangleVBO;

    Renderer() {
        // Shader programlarını oluştur
        textShaderProgram = createShaderProgram(
            textVertexShaderSource, textFragmentShaderSource);
        triangleShaderProgram = createShaderProgram(
            triangleVertexShaderSource, triangleFragmentShaderSource);

        // Projeksiyon matrisi ayarla
        projection = glm::ortho(0.0f, static_cast<GLfloat>(WIDTH),
            0.0f, static_cast<GLfloat>(HEIGHT));

        // Metin shader programı için projeksiyon matrisini ayarla
        glUseProgram(textShaderProgram);
        glUniformMatrix4fv(
            glGetUniformLocation(textShaderProgram, "projection"),
            1, GL_FALSE, glm::value_ptr(projection));

        // Üçgen için VAO ve VBO oluştur
        glGenVertexArrays(1, &triangleVAO);
        glGenBuffers(1, &triangleVBO);

        // FreeType ile karakterleri yükle
        loadCharacters("C:/Company/GroundControl/SkyLink/orange_juice2.ttf");
    }

    ~Renderer() {
        // Kaynakları serbest bırak
        glDeleteVertexArrays(1, &triangleVAO);
        glDeleteBuffers(1, &triangleVBO);
        glDeleteProgram(textShaderProgram);
        glDeleteProgram(triangleShaderProgram);
    }

    // Ekranı temizleme fonksiyonu
    void clear() {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    // Metin render fonksiyonu
    void renderText(std::string text, GLfloat x, GLfloat y,
        GLfloat scale, glm::vec3 color);

    // Üçgen çizme fonksiyonu
    void drawTriangle(float x, float y, float size, glm::vec3 color);

    // Shader programı oluşturma fonksiyonu
    GLuint createShaderProgram(const char* vertexSource,
        const char* fragmentSource);

    // FreeType ile karakterleri yükleme fonksiyonu
    void loadCharacters(std::string fontPath);

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
};

// Renderer sınıfının fonksiyonlarının tanımı
void Renderer::renderText(std::string text, GLfloat x, GLfloat y,
    GLfloat scale, glm::vec3 color) {
    // Metin render etme kodu
    glUseProgram(textShaderProgram);
    glUniform3f(glGetUniformLocation(textShaderProgram,
        "textColor"), color.x, color.y, color.z);
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

void Renderer::drawTriangle(float x, float y, float size,
    glm::vec3 color) {
    GLfloat vertices[] = {
        x,         y + size,  // Üst nokta
        x - size,  y - size,  // Sol alt
        x + size,  y - size   // Sağ alt
    };

    glUseProgram(triangleShaderProgram);

    // Projeksiyon matrisini gönder
    GLint projLoc = glGetUniformLocation(
        triangleShaderProgram, "projection");
    glUniformMatrix4fv(projLoc, 1, GL_FALSE,
        glm::value_ptr(projection));

    // Üçgenin rengini ayarla
    GLint colorLoc = glGetUniformLocation(
        triangleShaderProgram, "triangleColor");
    glUniform3f(colorLoc, color.r, color.g, color.b);

    glBindVertexArray(triangleVAO);

    glBindBuffer(GL_ARRAY_BUFFER, triangleVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices,
        GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
        2 * sizeof(GLfloat), (void*)0);

    glDrawArrays(GL_TRIANGLES, 0, 3);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

GLuint Renderer::createShaderProgram(const char* vertexSource,
    const char* fragmentSource) {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);

    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
            << infoLog << std::endl;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n"
            << infoLog << std::endl;
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
            << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

void Renderer::loadCharacters(std::string fontPath) {
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

// TriangleCellStrategy için draw fonksiyonunun tanımı
void TriangleCellStrategy::draw(GridCell& cell, Renderer& renderer) {
    // Hücrenin merkezini ve boyutunu hesapla
    GLfloat centerX = cell.x + cell.width / 2.0f;
    GLfloat centerY = cell.y + cell.height / 2.0f;
    GLfloat size = std::min(cell.width, cell.height) * 0.4f;

    // Üçgeni çiz
    renderer.drawTriangle(centerX, centerY, size,
        glm::vec3(1.0f, 0.0f, 0.0f));

    // Hücrede metin varsa çiz
    if (!cell.text.empty()) {
        renderer.renderText(cell.text, cell.x + 10,
            cell.y + cell.height - 30, 0.5f,
            glm::vec3(1.0f, 1.0f, 1.0f));
    }
}

// GridSystem Sınıfı (Composite Deseni)
class GridSystem {
public:
    std::vector<std::shared_ptr<GridCell>> cells;  // Hücreleri tutan vektör
    int rows, cols;               // Satır ve sütun sayısı
    float cellWidth, cellHeight;  // Her bir hücrenin boyutları

    GridSystem(int rows, int cols)
        : rows(rows), cols(cols) {
        cellWidth = WIDTH / cols;
        cellHeight = HEIGHT / rows;

        // Hücreleri oluştur
        createCells();
    }

    void createCells() {
        for (int i = 0; i < rows; ++i) {
            for (int j = 0; j < cols; ++j) {
                float x = j * cellWidth;
                float y = i * cellHeight;
                auto cell = std::make_shared<GridCell>(
                    x, y, cellWidth, cellHeight);
                // Varsayılan olarak üçgen çizme stratejisi
                cell->setStrategy(new TriangleCellStrategy());
                cells.push_back(cell);
            }
        }
    }

    void update() {
        for (auto& cell : cells) {
            cell->update();
        }
    }

    void draw(Renderer& renderer) {
        for (auto& cell : cells) {
            cell->draw(renderer);
        }
    }

    // Hücre ekleme fonksiyonu
    void addCell(std::shared_ptr<GridCell> cell) {
        cells.push_back(cell);
    }

    // Hücre silme fonksiyonu
    void removeCell(std::shared_ptr<GridCell> cell) {
        cells.erase(std::remove(cells.begin(), cells.end(), cell),
            cells.end());
    }

    // Belirli bir hücreyi alma fonksiyonu
    std::shared_ptr<GridCell> getCell(int row, int col) {
        if (row < 0 || row >= rows || col < 0 || col >= cols)
            return nullptr;
        int index = row * cols + col;
        return cells[index];
    }
};

// Main fonksiyonu
int main() {
    // GLFW'yi başlat
    if (!glfwInit()) {
        std::cout << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Pencere oluştur
    GLFWwindow* window = glfwCreateWindow(
        WIDTH, HEIGHT, "Grid System", NULL, NULL);
    if (!window) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // GLEW'i başlat
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cout << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Renderer oluştur
    Renderer renderer;

    // Grid sistemi oluştur
    GridSystem grid(4, 4);

    // Veri sağlayıcı oluştur
    DataProvider dataProvider;

    // Hücrelere veri sağlayıcıyı bağla
    for (auto& cell : grid.cells) {
        cell->dataProvider = &dataProvider;
        dataProvider.attach(cell.get());
    }

    // Rastgele sayı üretici
    std::default_random_engine generator;
    std::uniform_int_distribution<int> distribution(0, 100);

    // Ana döngü
    while (!glfwWindowShouldClose(window)) {
        // Girdi işlemleri
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        // Veri güncelleme
        int randomData = distribution(generator);
        dataProvider.setData(randomData);

        // Güncelleme
        grid.update();

        // Çizim
        renderer.clear();
        grid.draw(renderer);

        // GLFW buffer swap ve event polling
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Temizlik
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
