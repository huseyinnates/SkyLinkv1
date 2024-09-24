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
// Newly added header files
#include "Mesh.h"
#include "Shader.h"
#include "ModelLoader.h"
#include "Camera.h"

using namespace SkyLink;
using namespace SkyLink::Model;
using namespace glm;

// **Global Variables**
GridSystem* gridSystem = nullptr;
Renderer* renderer = nullptr;

// New global variables
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, 0.0f);
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

const GLchar* simpleFragmentShaderSource = R"glsl(
    #version 330 core
    in vec3 vertexColor;
    out vec4 color;

    void main()
    {
        color = vec4(vertexColor, 1.0);
    }
)glsl";
const GLchar* simpleVertexShaderSource = R"glsl(
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
// **Shader Source Codes**
const char* vertexShaderSource = R"(
    // Vertex Shader for 3D model
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
    // Fragment Shader
    #version 330 core
    out vec4 FragColor;

    in vec3 FragPos;
    in vec3 Normal;

    #define NR_LIGHTS 4

    struct Light {
        vec3 position;
        vec3 color;
    };

    uniform vec3 viewPos;
    uniform Light lights[NR_LIGHTS];
    uniform vec3 objectColor;

    void main()
    {
        vec3 norm = normalize(Normal);
        vec3 viewDir = normalize(viewPos - FragPos);
        vec3 result = vec3(0.0);

        for(int i = 0; i < NR_LIGHTS; i++)
        {
            // Ambient
            float ambientStrength = 0.1;
            vec3 ambient = ambientStrength * lights[i].color;

            // Diffuse
            vec3 lightDir = normalize(lights[i].position - FragPos);
            float diff = max(dot(norm, lightDir), 0.0);
            vec3 diffuse = diff * lights[i].color;

            // Specular
            float specularStrength = 0.5;
            vec3 reflectDir = reflect(-lightDir, norm);
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
            vec3 specular = specularStrength * spec * lights[i].color;

            result += (ambient + diffuse + specular);
        }

        result *= objectColor;
        FragColor = vec4(result, 1.0);
    }
)";

// **Shader Compilation Functions**
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

// **Drawing Functions**
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
    // 3D model drawing
    glEnable(GL_DEPTH_TEST);

    // Use shader
    modelShader->use();

    // Set uniform variables
    modelShader->setVec3("lightPos", glm::vec3(1.2f, 1.0f, 2.0f));
    modelShader->setVec3("viewPos", camera.position);
    modelShader->setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));
    modelShader->setVec3("objectColor", mesh.diffuseColor);

    // Transformations
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 projection = glm::perspective(glm::radians(camera.fov),
        static_cast<float>(WIDTH) / static_cast<float>(HEIGHT), 0.1f, 100.0f);

    modelShader->setMat4("model", model);
    modelShader->setMat4("view", view);
    modelShader->setMat4("projection", projection);

    // Draw mesh
    mesh.draw();

    glDisable(GL_DEPTH_TEST);
}

void drawVisualScriptingScreen(GLuint shaderProgram) {
    // Draw a triangle in blue
    drawColoredTriangle(shaderProgram, 0.0f, 0.0f, 1.0f);
}

// **Key Callback Function**
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
    static bool escPressedLastFrame = false;

    bool escPressed = glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS;

    if (escPressed && !escPressedLastFrame) {
        // ESC key was pressed
        if (currentScreen == ScreenState::Model) {
            currentScreen = ScreenState::Monitoring; // Switch to desired screen
        }
    }

    escPressedLastFrame = escPressed;

    if (currentScreen == ScreenState::Model) {
        // Camera movement
        float cameraSpeed = 2.5f * deltaTime; // Speed factor

        // Move forward
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            camera.distance -= cameraSpeed;
            if (camera.distance < 1.0f)
                camera.distance = 1.0f;
        }

        // Move backward
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            camera.distance += cameraSpeed;
            if (camera.distance > 10.0f)
                camera.distance = 10.0f;
        }

        // Update camera yaw for left and right movement
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            camera.yaw -= cameraSpeed * 50.0f; // Angle change
            camera.updateCameraVectors();
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            camera.yaw += cameraSpeed * 50.0f; // Angle change
            camera.updateCameraVectors();
        }
    }
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
    float yoffset = lastY - static_cast<float>(ypos); // Reversed since y-coordinates range from bottom to top

    lastX = static_cast<float>(xpos);
    lastY = static_cast<float>(ypos);

    camera.processMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    if (currentScreen != ScreenState::Model)
        return;

    camera.processMouseScroll(static_cast<float>(yoffset));
}

// **Mouse Button Callback Function**
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

    // Define light sources
    std::vector<glm::vec3> lightPositions;
    std::vector<glm::vec3> lightColors;

    lightPositions.push_back(glm::vec3(3.0f, 3.0f, 3.0f));
    lightColors.push_back(glm::vec3(1.0f, 0.0f, 0.0f)); // Red

    lightPositions.push_back(glm::vec3(-3.0f, 3.0f, 3.0f));
    lightColors.push_back(glm::vec3(0.0f, 1.0f, 0.0f)); // Green

    lightPositions.push_back(glm::vec3(3.0f, -3.0f, 3.0f));
    lightColors.push_back(glm::vec3(0.0f, 0.0f, 1.0f)); // Blue

    lightPositions.push_back(glm::vec3(-3.0f, -3.0f, -3.0f));
    lightColors.push_back(glm::vec3(1.0f, 1.0f, 1.0f));

    // Create Shader Program
    GLuint shaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);
    GLuint simpleShaderProgram = createShaderProgram(simpleVertexShaderSource, simpleFragmentShaderSource);

    // Create Model Shader
    modelShader = new Shader(vertexShaderSource, fragmentShaderSource);
    modelShader->use();

    // Set camera position
    modelShader->setVec3("viewPos", camera.position);

    // Set object color
    modelShader->setVec3("objectColor", mesh.diffuseColor);

    // Send light sources to shader
    for (int i = 0; i < lightPositions.size(); i++)
    {
        std::string number = std::to_string(i);
        modelShader->setVec3("lights[" + number + "].position", lightPositions[i]);
        modelShader->setVec3("lights[" + number + "].color", lightColors[i]);
    }

    // Load model
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

    // **Set GLFW Callback Functions**
    glfwSetKeyCallback(window, keyCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // **Assign Callback Functions to Some Cells as Examples**

    // Assign key callback to cell (e.g., 'A' key)
    grid.getCell(0, 0)->setKeyCallback(GLFW_KEY_A, []() {
        std::cout << "Cell 0,0 'A' key pressed!" << std::endl;
        });

    // Assign mouse callback to cell
    grid.getCell(0, 0)->setMouseCallback([]() {
        std::cout << "Cell 0,0 clicked!" << std::endl;
        });

    // **Dear ImGui Initialization**
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

    // **Load Custom Font**
    ImFont* customFont = io.Fonts->AddFontFromFileTTF("C:/Company/GroundControl/SkyLinkv1/orange_juice2.ttf", 16.0f);
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

    // **Set up ImGui Style for the Toolbar**
    // Remove window background
    style.Colors[ImGuiCol_WindowBg] = ImVec4(1.0f, 0.4f, 0.0f, 1.0f);

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Time calculations
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Process user input
        processInput(window);

        // Check for screen state change
        if (currentScreen != previousScreen) {
            if (currentScreen == ScreenState::Model) {
                // When entering the Model screen
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // Disable the cursor
                firstMouse = true; // Reset the first mouse movement
            }
            else if (previousScreen == ScreenState::Model) {
                // When exiting the Model screen
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); // Re-enable the cursor
            }
            previousScreen = currentScreen;
        }

        // Update data
        int randomData = distribution(generator);
        dataProvider.setData(randomData);

        // Update grid
        grid.update();

        // **Start New ImGui Frame**
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // **Fixed Toolbar**
        ImGuiWindowFlags toolbarFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(static_cast<float>(WIDTH), TOOLBAR_HEIGHT));
        ImGui::Begin("##Toolbar", nullptr, toolbarFlags);

        // Center buttons vertically
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

        ImGui::End(); // End toolbar window

        // Render ImGui
        ImGui::Render();

        // **Render Content Based on Selected Screen**
        // Clear screen
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Draw content based on screen state
        switch (currentScreen) {
        case ScreenState::Monitoring:
            drawMonitoringScreen(*renderer);
            break;
        case ScreenState::MissionControl:
            drawMissionControlScreen(simpleShaderProgram);
            break;
        case ScreenState::Model:
            drawModelScreen(shaderProgram);
            break;
        case ScreenState::VisualScripting:
            drawVisualScriptingScreen(simpleShaderProgram);
            break;
        }

        // Render ImGui data with OpenGL
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // **Cleanup**
    glDeleteProgram(shaderProgram);
    glDeleteProgram(simpleShaderProgram);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
