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


using namespace SkyLink; // Corrected namespace from 'SkyLink' to 'SkyLine'

// **Global variables**
GridSystem* gridSystem = nullptr;
Renderer* renderer = nullptr;

// **Application States**
enum class ScreenState {
    Monitoring,
    MissionControl,
    Model,
    VisualScripting
};

ScreenState currentScreen = ScreenState::Monitoring;

// **Shader sources**
const GLchar* vertexShaderSource = R"glsl(
    #version 330 core
    layout(location = 0) in vec3 position;
    layout(location = 1) in vec3 color;

    out vec3 vertexColor;

    void main()
    {
        gl_Position = vec4(position, 1.0);
        vertexColor = color;
    }
)glsl";

const GLchar* fragmentShaderSource = R"glsl(
    #version 330 core
    in vec3 vertexColor;
    out vec4 color;

    void main()
    {
        color = vec4(vertexColor, 1.0);
    }
)glsl";

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
    // Draw a triangle in green
    drawColoredTriangle(shaderProgram, 0.0f, 1.0f, 0.0f);
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
    while (!glfwWindowShouldClose(window)) {
        // Input processing
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        // Data update
        int randomData = distribution(generator);
        dataProvider.setData(randomData);

        // Update
        grid.update();

        // **ImGui Frame Start**
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // **Fixed Toolbar**
        // Set window flags to make the toolbar fixed and non-scrollable
        ImGuiWindowFlags toolbarFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

        // Begin toolbar window
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(static_cast<float>(WIDTH), TOOLBAR_HEIGHT));
        ImGui::Begin("##Toolbar", nullptr, toolbarFlags);

        // Center the buttons vertically
        ImGui::SetCursorPosY((TOOLBAR_HEIGHT - ImGui::GetFontSize()) / 2);

        // Monitoring Button
        if (ImGui::Button("Monitoring")) {
            currentScreen = ScreenState::Monitoring;
        }
        ImGui::SameLine();

        // Mission Control Button
        if (ImGui::Button("Mission Control")) {
            currentScreen = ScreenState::MissionControl;
        }
        ImGui::SameLine();

        // Model Button
        if (ImGui::Button("Model")) {
            currentScreen = ScreenState::Model;
        }
        ImGui::SameLine();

        // Visual Scripting Button
        if (ImGui::Button("Visual Scripting")) {
            currentScreen = ScreenState::VisualScripting;
        }

        ImGui::End();

        // **ImGui Rendering**
        ImGui::Render();

        // **Render Content Based on Current Screen**
        renderer->clear();

        // Adjust the viewport to account for the toolbar height if necessary
        glViewport(0, 0, WIDTH, HEIGHT);

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

        // ImGui Rendering with OpenGL
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // GLFW buffer swap and event polling
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
