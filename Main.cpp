// main.cpp

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "Renderer.h"
#include "GridSystem.h"
#include "DataProvider.h"
#include "CellStrategy.h"
#include <iostream>
#include <random>
#include <assimp/scene.h>
// Dear ImGui headers
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_freetype.h"
// Yeni eklenen başlık dosyaları
#include "Mesh.h"
#include "Shader.h"
#include "ModelLoader.h" 
#include "Camera.h"


using namespace SkyLink; // Corrected namespace from 'SkyLink' to 'SkyLine'
using namespace SkyLink::Model;
using namespace glm;
// **Global variables**
GridSystem* gridSystem = nullptr;
Renderer* renderer = nullptr;

// Yeni global değişkenler
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f),glm::vec3(0.0f, 1.0f, 0.0f),-90.0f, 0.0f); 
float lastX = WIDTH / 2.0f;
float lastY = HEIGHT / 2.0f;
bool firstMouse = true;
float deltaTime = 0.0f;
float lastFrame = 0.0f;
Mesh mesh;
Shader* modelShader = nullptr;
// **Application States**
enum class ScreenState {
    Monitoring,
    MissionControl,
    Model,
    VisualScripting
};

ScreenState currentScreen = ScreenState::Monitoring;
ScreenState previousScreen = currentScreen;

// **Shader kaynak kodları**
const char* vertexShaderSource = R"(
    // Vertex Shader kodunuz (3D model için)
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aNormal;

    out vec3 FragPos;
    out vec3 Normal;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    void main()
    {
        FragPos = vec3(model * vec4(aPos, 1.0));
        Normal = mat3(transpose(inverse(model))) * aNormal;
        gl_Position = projection * view * vec4(FragPos, 1.0);
    }
)";

const char* fragmentShaderSource = R"(
    // Fragment Shader kodunuz (3D model için)
    #version 330 core
    out vec4 FragColor;

    in vec3 FragPos;
    in vec3 Normal;

    uniform vec3 lightPos;
    uniform vec3 viewPos;
    uniform vec3 lightColor;
    uniform vec3 objectColor;

    void main()
    {
        // Ambient
        float ambientStrength = 0.1;
        vec3 ambient = ambientStrength * lightColor;

        // Diffuse 
        vec3 norm = normalize(Normal);
        vec3 lightDir = normalize(lightPos - FragPos);
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = diff * lightColor;

        // Specular
        float specularStrength = 0.5;
        vec3 viewDir = normalize(viewPos - FragPos);
        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
        vec3 specular = specularStrength * spec * lightColor;

        vec3 result = (ambient + diffuse + specular) * objectColor;
        FragColor = vec4(result, 1.0);
    }
)";

// **Shader compilation functions**
// Function to compile a shader
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

// Function to create shader program
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

// **Drawing functions**
void drawMonitoringScreen(Renderer& renderer) {
    gridSystem->draw(renderer);
}

void drawColoredTriangle(GLuint shaderProgram, float r, float g, float b) {
    // Set up the vertices for the triangle
    GLfloat vertices[] = {
        // Positions         // Colors
         0.0f,  0.5f, 0.0f,   r, g, b, // Top vertex
        -0.5f, -0.5f, 0.0f,   r, g, b, // Bottom left
         0.5f, -0.5f, 0.0f,   r, g, b  // Bottom right
    };

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    // Bind the Vertex Array Object first, then bind and set vertex buffer(s)
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Position attribute (location = 0 in the vertex shader)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    // Color attribute (location = 1 in the vertex shader)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat),
        (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    // Unbind VBO and VAO
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Draw the triangle
    glUseProgram(shaderProgram);
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);

    // Clean up (optional in this context)
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}

void drawMissionControlScreen(GLuint shaderProgram) {
    // Draw a triangle in red
    drawColoredTriangle(shaderProgram, 1.0f, 0.0f, 0.0f);
}

void drawModelScreen(GLuint shaderProgram) {
    // 3D model çizimi
    glEnable(GL_DEPTH_TEST);

    // Shader kullan
    modelShader->use();

    // Uniform değişkenleri ayarla
    modelShader->setVec3("lightPos", glm::vec3(1.2f, 1.0f, 2.0f));
    modelShader->setVec3("viewPos", camera.position);
    modelShader->setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));
    modelShader->setVec3("objectColor", mesh.diffuseColor);

    // Dönüşümler
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 projection = glm::perspective(glm::radians(camera.fov),
        static_cast<float>(WIDTH) / static_cast<float>(HEIGHT), 0.1f, 100.0f);

    modelShader->setMat4("model", model);
    modelShader->setMat4("view", view);
    modelShader->setMat4("projection", projection);

    // Mesh çizimi
    mesh.draw();

    glDisable(GL_DEPTH_TEST);
}

void drawVisualScriptingScreen(GLuint shaderProgram) {
    // Draw a triangle in blue
    drawColoredTriangle(shaderProgram, 0.0f, 0.0f, 1.0f);
}

// **Key callback function**
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        // If 'A' key is pressed
        if (key == GLFW_KEY_A) {
            // Change strategies of all cells in the 2nd row
            int row = 1; // Indexing starts from 0
            for (int col = 0; col < gridSystem->cols; ++col) {
                auto cell = gridSystem->getCell(row, col);
                if (cell) {
                    cell->setStrategy(new TriangleBlueCellStrategy());
                }
            }
            std::cout << "'A' key pressed, cells in the 2nd row turned blue." << std::endl;
        }

        // Check all cells
        for (auto& cell : gridSystem->cells) {
            if (cell->key == key && cell->onKeyCallback) {
                cell->onKeyCallback();
            }
        }
    }
}
void processInput(GLFWwindow* window) {
    // ESC tuşu ile pencereyi kapat
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (currentScreen == ScreenState::Model) {
        // Kamera hareketi
        float cameraSpeed = 2.5f * deltaTime; // Hız faktörü

        // İleri hareket
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            // Kamera pozisyonunu güncelle
            camera.distance -= cameraSpeed;
            if (camera.distance < 1.0f)
                camera.distance = 1.0f;
        }

        // Geri hareket
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            camera.distance += cameraSpeed;
            if (camera.distance > 10.0f)
                camera.distance = 10.0f;
        }

        // Sol ve sağ hareket için kamera açısını güncelle
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            camera.yaw -= cameraSpeed * 50.0f; // Açı değişimi
            camera.updateCameraVectors();
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            camera.yaw += cameraSpeed * 50.0f; // Açı değişimi
            camera.updateCameraVectors();
        }
    }
}
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    // Pencere boyutu değiştiğinde viewport'u güncelle
    glViewport(0, 0, width, height);
}
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (currentScreen != ScreenState::Model)
        return;

    if (firstMouse) {
        lastX = static_cast<float>(xpos);
        lastY = static_cast<float>(ypos);
        firstMouse = false;
    }
    float xoffset = static_cast<float>(xpos) - lastX;
    float yoffset = lastY - static_cast<float>(ypos); // Y koordinatları ters

    lastX = static_cast<float>(xpos);
    lastY = static_cast<float>(ypos);

    camera.processMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    if (currentScreen != ScreenState::Model)
        return;

    camera.processMouseScroll(static_cast<float>(yoffset));
}

// **Mouse button callback function**
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_LEFT) {
        // Get mouse position
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        // Adjust Y coordinate (according to OpenGL coordinate system)
        ypos = HEIGHT - ypos;

        // Find which cell was clicked
        for (auto& cell : gridSystem->cells) {
            if (cell->containsPoint(static_cast<float>(xpos), static_cast<float>(ypos)) && cell->onMouseCallback) {
                cell->onMouseCallback();
            }
        }
    }
}

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cout << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create window
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Grid System", NULL, NULL);
    if (!window) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Initialize GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cout << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Create Shader Program
    GLuint shaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);
    // Model Shader'ı oluşturma
    modelShader = new Shader(vertexShaderSource, fragmentShaderSource);

    // Model yükleme
    ModelLoader loader;
    if (!loader.loadModel("C:/Company/GroundControl/rocket.fbx", mesh)) {
        std::cout << "Failed to load model!" << std::endl;
        return -1;
    }
    // Create Renderer object after GLEW initialization
    Renderer renderObj;
    renderer = &renderObj;

    // Create grid system
    GridSystem grid(4, 4);
    gridSystem = &grid;

    // Create data provider
    DataProvider dataProvider;

    // Attach data provider to cells
    for (auto& cell : grid.cells) {
        cell->dataProvider = &dataProvider;
        dataProvider.attach(cell.get());
    }

    // Random number generator
    std::default_random_engine generator;
    std::uniform_int_distribution<int> distribution(0, 100);

    // **Set GLFW callback functions**
    glfwSetKeyCallback(window, keyCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);

    // **Assign callback functions to some cells as examples**

    // Assign key callback to cell (e.g., 'A' key)
    grid.getCell(0, 0)->setKeyCallback(GLFW_KEY_A, []() {
        std::cout << "Cell 0,0 'A' key pressed!" << std::endl;
        });

    // Assign mouse callback to cell
    grid.getCell(0, 0)->setMouseCallback([]() {
        std::cout << "Cell 0,0 clicked!" << std::endl;
        });

    // **Dear ImGui Initialization**
    // Create ImGui Context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // **Load Custom Font**
    ImFont* customFont = io.Fonts->AddFontFromFileTTF("C:/Company/GroundControl/SkyLink/orange_juice2.ttf", 16.0f);
    if (!customFont) {
        std::cout << "Failed to load custom font!" << std::endl;
    }
    else {
        io.FontDefault = customFont; // Set custom font as default
    }

    // **Set Button Colors to Orange**
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_Button] = ImVec4(1.0f, 0.5f, 0.0f, 1.0f); // Orange
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(1.0f, 0.6f, 0.1f, 1.0f); // Light orange
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(1.0f, 0.4f, 0.0f, 1.0f); // Dark orange

    // Initialize Platform/Renderer backends
    const char* glsl_version = "#version 330";
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // **Set up ImGui style for the toolbar**
    // Remove window background
    style.Colors[ImGuiCol_WindowBg] = ImVec4(1.0f, 0.4f, 0.0f, 1.0f);

    // Main loop
   // Ana döngü
    while (!glfwWindowShouldClose(window)) {
        // Zaman hesaplamaları
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Kullanıcı girişlerini işleme
        processInput(window);

        // Ekran durum değişikliğini kontrol et
        if (currentScreen != previousScreen) {
            if (currentScreen == ScreenState::Model) {
                // Model ekranına geçildiğinde
                glfwSetCursorPosCallback(window, mouse_callback);
                glfwSetScrollCallback(window, scroll_callback);
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }
            else if (previousScreen == ScreenState::Model) {
                // Model ekranından çıkıldığında
                glfwSetCursorPosCallback(window, NULL);
                glfwSetScrollCallback(window, NULL);
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                firstMouse = true; // İlk mouse hareketini sıfırla
            }
            previousScreen = currentScreen;
        }

        // Veri güncelleme
        int randomData = distribution(generator);
        dataProvider.setData(randomData);

        // Grid güncelleme
        grid.update();

        // **ImGui Yeni Frame Başlat**
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // **Sabit Araç Çubuğu**
        // Araç çubuğu penceresi ayarları
        ImGuiWindowFlags toolbarFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

        // Araç çubuğu penceresini başlat
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(static_cast<float>(WIDTH), TOOLBAR_HEIGHT));
        ImGui::Begin("##Toolbar", nullptr, toolbarFlags);

        // Butonları dikey olarak ortala
        ImGui::SetCursorPosY((TOOLBAR_HEIGHT - ImGui::GetFontSize()) / 2);

        // Monitoring Butonu
        if (ImGui::Button("Monitoring")) {
            currentScreen = ScreenState::Monitoring;
        }
        ImGui::SameLine();

        // Mission Control Butonu
        if (ImGui::Button("Mission Control")) {
            currentScreen = ScreenState::MissionControl;
        }
        ImGui::SameLine();

        // Model Butonu
        if (ImGui::Button("Model")) {
            currentScreen = ScreenState::Model;
        }
        ImGui::SameLine();

        // Visual Scripting Butonu
        if (ImGui::Button("Visual Scripting")) {
            currentScreen = ScreenState::VisualScripting;
        }

        ImGui::End(); // Araç çubuğu penceresini sonlandır

        // ImGui render işlemi
        ImGui::Render();

        // **Seçilen Ekrana Göre İçerik Render Etme**
        // Ekranı temizle
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Ekran durumuna göre içerik çizimi
        switch (currentScreen) {
        case ScreenState::Monitoring:
            drawMonitoringScreen(*renderer);
            break;
        case ScreenState::MissionControl:
            drawMissionControlScreen(shaderProgram);
            break;
        case ScreenState::Model:
            drawModelScreen(shaderProgram);
            break;
        case ScreenState::VisualScripting:
            drawVisualScriptingScreen(shaderProgram);
            break;
        }

        // ImGui verilerini OpenGL ile render et
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Bufferları değiştirme ve olayları çekme
        glfwSwapBuffers(window);
        glfwPollEvents();
    }


    // **Cleanup**
    glDeleteProgram(shaderProgram);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
