/*


// main.cpp

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H

#include <iostream>
#include <vector>
#include <map>
#include <chrono>
#include <thread>
#include <cmath>
#include <string>
#include <stdexcept>

// Define M_PI if not defined
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Shader sources
const char* vertexShaderSource = R"glsl(
#version 330 core
layout(location = 0) in vec2 aPos;

uniform mat4 projection;

void main()
{
    gl_Position = projection * vec4(aPos, 0.0, 1.0);
}
)glsl";

const char* fragmentShaderSource = R"glsl(
#version 330 core
out vec4 FragColor;

uniform vec3 color;

void main()
{
    FragColor = vec4(color, 1.0);
}
)glsl";

// Text Shader Sources
const char* textVertexShaderSource = R"glsl(
#version 330 core
layout(location = 0) in vec4 vertex; // <vec2 pos, vec2 tex>

out vec2 TexCoords;

uniform mat4 projection;

void main()
{
    gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);
    TexCoords = vertex.zw;
}
)glsl";

const char* textFragmentShaderSource = R"glsl(
#version 330 core
in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D text;
uniform vec3 textColor;

void main()
{
    float alpha = texture(text, TexCoords).r;
    FragColor = vec4(textColor, alpha);
}
)glsl";

// Background Shader Sources (Optional for Gradient Background)
const char* bgVertexShaderSource = R"glsl(
#version 330 core
layout(location = 0) in vec2 aPos;

void main()
{
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)glsl";

const char* bgFragmentShaderSource = R"glsl(
#version 330 core
out vec4 FragColor;

void main()
{
    // Simple vertical gradient
    float gradient = gl_FragCoord.y / 600.0; // Adjust based on actual window height
    vec3 topColor = vec3(0.2f, 0.2f, 0.2f);
    vec3 bottomColor = vec3(0.1f, 0.1f, 0.1f);
    vec3 color = mix(bottomColor, topColor, gradient);
    FragColor = vec4(color, 1.0f);
}
)glsl";

// Character structure for FreeType
struct Character {
    GLuint TextureID;   // Texture ID
    glm::ivec2 Size;    // Size of glyph
    glm::ivec2 Bearing; // Offset from baseline to left/top of glyph
    GLuint Advance;     // Horizontal offset to advance to next glyph
};

// Global variables for characters
std::map<char, Character> Characters;
GLuint textVAO, textVBO;

// Function to compile shaders
GLuint CompileShader(GLenum type, const char* source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    // Check compile status
    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::string shaderType = (type == GL_VERTEX_SHADER) ? "VERTEX" : "FRAGMENT";
        std::cout << "ERROR::SHADER::" << shaderType << "::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    return shader;
}

// Function to render text using FreeType and dedicated text shader
void RenderText(FT_Face face, std::string text, float x, float y, float scale, glm::vec3 color, GLuint textShaderProgram, glm::mat4 projection)
{
    glUseProgram(textShaderProgram);
    glUniform3f(glGetUniformLocation(textShaderProgram, "textColor"), color.x, color.y, color.z);
    glUniformMatrix4fv(glGetUniformLocation(textShaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);

    glActiveTexture(GL_TEXTURE0);
    glUniform1i(glGetUniformLocation(textShaderProgram, "text"), 0);
    glBindVertexArray(textVAO);

    for (auto c : text)
    {
        // Ensure character exists in the map
        if (Characters.find(c) == Characters.end()) continue;

        Character ch = Characters[c];

        float xpos = x + ch.Bearing.x * scale;
        float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;

        // Vertex data
        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },

            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }
        };

        // Bind texture
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);

        // Update VBO
        glBindBuffer(GL_ARRAY_BUFFER, textVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Draw quad
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Advance cursor
        x += (ch.Advance >> 6) * scale; // Advance is in 1/64 pixels
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

// Callback variables for pan and zoom
float zoomLevel = 1.0f;
glm::vec2 panOffset = glm::vec2(0.0f, 0.0f);

// Scroll callback for zoom
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    // Implement exponential zoom for smoother zooming
    float zoomFactor = 1.05f;
    if (yoffset > 0)
        zoomLevel *= zoomFactor;
    else if (yoffset < 0)
        zoomLevel /= zoomFactor;

    // Clamp zoom level
    if (zoomLevel < 0.1f)
        zoomLevel = 0.1f;
    if (zoomLevel > 10.0f)
        zoomLevel = 10.0f;
}

// Mouse drag variables
bool mousePressed = false;
double lastX, lastY;

// Mouse button callback for dragging
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            mousePressed = true;
            glfwGetCursorPos(window, &lastX, &lastY);
        }
        else if (action == GLFW_RELEASE)
        {
            mousePressed = false;
        }
    }
}

// Cursor position callback for panning
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (mousePressed)
    {
        double dx = xpos - lastX;
        double dy = ypos - lastY;
        lastX = xpos;
        lastY = ypos;

        // Adjust panOffset based on mouse movement and zoom level
        panOffset.x -= dx * 0.005f * zoomLevel;
        panOffset.y += dy * 0.005f * zoomLevel;
    }
}

// Compute "nice" grid spacing based on the view range
float computeGridSpacing(float viewRange)
{
    float roughSpacing = viewRange / 10.0f; // Aim for ~10 grid lines
    float exponent = floor(log10(roughSpacing));
    float fraction = roughSpacing / pow(10.0f, exponent);

    float niceFraction;
    if (fraction < 1.5f)
        niceFraction = 1.0f;
    else if (fraction < 3.0f)
        niceFraction = 2.0f;
    else if (fraction < 7.0f)
        niceFraction = 5.0f;
    else
        niceFraction = 10.0f;

    return niceFraction * pow(10.0f, exponent);
}

// Function to generate grid lines based on current view
std::vector<float> generateGridLines(float left, float right, float bottom, float top)
{
    std::vector<float> lines;

    float viewWidth = right - left;
    float viewHeight = top - bottom;

    float gridSpacingX = computeGridSpacing(viewWidth);
    float gridSpacingY = computeGridSpacing(viewHeight);

    // Vertical grid lines
    float startX = ceil(left / gridSpacingX) * gridSpacingX;
    for (float x = startX; x <= right; x += gridSpacingX)
    {
        lines.push_back(x);
        lines.push_back(bottom);
        lines.push_back(x);
        lines.push_back(top);
    }

    // Horizontal grid lines
    float startY = ceil(bottom / gridSpacingY) * gridSpacingY;
    for (float y = startY; y <= top; y += gridSpacingY)
    {
        lines.push_back(left);
        lines.push_back(y);
        lines.push_back(right);
        lines.push_back(y);
    }

    return lines;
}

// Function to generate tick labels based on grid spacing
std::vector<std::pair<float, float>> generateTickLabels(float left, float right, float bottom, float top, float gridSpacingX, float gridSpacingY)
{
    std::vector<std::pair<float, float>> labels;

    // X-axis labels
    float startX = ceil(left / gridSpacingX) * gridSpacingX;
    for (float x = startX; x <= right; x += gridSpacingX)
    {
        labels.emplace_back(std::make_pair(x, bottom)); // Position at bottom
    }

    // Y-axis labels
    float startY = ceil(bottom / gridSpacingY) * gridSpacingY;
    for (float y = startY; y <= top; y += gridSpacingY)
    {
        labels.emplace_back(std::make_pair(left, y)); // Position at left
    }

    return labels;
}

int main()
{
    // Initialize GLFW
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW!" << std::endl;
        return -1;
    }

    // Configure GLFW for OpenGL 3.3 Core with multi-sampling for anti-aliasing
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4); // 4x MSAA

    // Create window
    GLFWwindow* window = glfwCreateWindow(800, 600, "Real-Time Data Visualization", NULL, NULL);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window!" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Initialize GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
    {
        std::cerr << "Failed to initialize GLEW!" << std::endl;
        return -1;
    }

    // Enable multi-sampling and blending
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Set viewport
    int framebufferWidth, framebufferHeight;
    glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
    glViewport(0, 0, framebufferWidth, framebufferHeight);

    // Set input callbacks
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);

    // Compile main shaders
    GLuint vertexShader = CompileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    // Create main shader program
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // Check program link status
    int success;
    char infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    // Delete shaders as they're linked now
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Compile text shaders
    GLuint textVertexShader = CompileShader(GL_VERTEX_SHADER, textVertexShaderSource);
    GLuint textFragmentShader = CompileShader(GL_FRAGMENT_SHADER, textFragmentShaderSource);

    // Create text shader program
    GLuint textShaderProgram = glCreateProgram();
    glAttachShader(textShaderProgram, textVertexShader);
    glAttachShader(textShaderProgram, textFragmentShader);
    glLinkProgram(textShaderProgram);

    // Check text shader program link status
    glGetProgramiv(textShaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(textShaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::TEXT_PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    // Delete text shaders as they're linked now
    glDeleteShader(textVertexShader);
    glDeleteShader(textFragmentShader);

    // Compile background shaders (Optional)
    GLuint bgVertexShader = CompileShader(GL_VERTEX_SHADER, bgVertexShaderSource);
    GLuint bgFragmentShader = CompileShader(GL_FRAGMENT_SHADER, bgFragmentShaderSource);

    // Create background shader program
    GLuint bgShaderProgram = glCreateProgram();
    glAttachShader(bgShaderProgram, bgVertexShader);
    glAttachShader(bgShaderProgram, bgFragmentShader);
    glLinkProgram(bgShaderProgram);

    // Check background shader program link status
    glGetProgramiv(bgShaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(bgShaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::BG_PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    // Delete background shaders as they're linked now
    glDeleteShader(bgVertexShader);
    glDeleteShader(bgFragmentShader);

    // Setup background VAO and VBO (Optional)
    float bgVertices[] = {
        // positions
        -1.0f,  1.0f,
        -1.0f, -1.0f,
         1.0f, -1.0f,

        -1.0f,  1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f
    };
    GLuint bgVAO, bgVBO;
    glGenVertexArrays(1, &bgVAO);
    glGenBuffers(1, &bgVBO);
    glBindVertexArray(bgVAO);
    glBindBuffer(GL_ARRAY_BUFFER, bgVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(bgVertices), bgVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Setup axes with arrowheads VAO and VBO
    float arrowSize = 0.05f;
    float axesWithArrowsVertices[] = {
        // X axis
        -1.0f, 0.0f,
         1.0f, 0.0f,
         // X axis arrowhead
          1.0f, 0.0f,
          1.0f - arrowSize,  arrowSize,
          1.0f, 0.0f,
          1.0f - arrowSize, -arrowSize,
          // Y axis
           0.0f, -1.0f,
           0.0f,  1.0f,
           // Y axis arrowhead
            0.0f, 1.0f,
            arrowSize,  1.0f - arrowSize,
            0.0f, 1.0f,
           -arrowSize, 1.0f - arrowSize
    };
    GLuint axesVAO, axesVBO;
    glGenVertexArrays(1, &axesVAO);
    glGenBuffers(1, &axesVBO);
    glBindVertexArray(axesVAO);
    glBindBuffer(GL_ARRAY_BUFFER, axesVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(axesWithArrowsVertices), axesWithArrowsVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Setup grid lines VAO and VBO
    std::vector<float> gridVerticesData; // Will be dynamically updated

    GLuint gridVAO, gridVBO;
    glGenVertexArrays(1, &gridVAO);
    glGenBuffers(1, &gridVBO);
    glBindVertexArray(gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
    // Initially empty; will be filled each frame
    glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Setup data points VAO and VBO
    GLuint pointsVAO, pointsVBO;
    glGenVertexArrays(1, &pointsVAO);
    glGenBuffers(1, &pointsVBO);
    glBindVertexArray(pointsVAO);
    glBindBuffer(GL_ARRAY_BUFFER, pointsVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * 1000, NULL, GL_DYNAMIC_DRAW); // Max 1000 points
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Setup text VAO and VBO
    glGenVertexArrays(1, &textVAO);
    glGenBuffers(1, &textVBO);
    glBindVertexArray(textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW); // 6 vertices, 4 floats each
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Setup projection matrix (Orthographic)
    glm::mat4 projection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f);

    // Initialize FreeType
    FT_Library ft;
    if (FT_Init_FreeType(&ft))
    {
        std::cerr << "Failed to initialize FreeType!" << std::endl;
        return -1;
    }

    // Load font
    FT_Face face;
    std::string fontPath = "C:/Company/GroundControl/SkyLink/orange_juice2.ttf"; // Font path relative to executable
    if (FT_New_Face(ft, fontPath.c_str(), 0, &face))
    {
        std::cerr << "Failed to load font at path: " << fontPath << std::endl;
        return -1;
    }
    FT_Set_Pixel_Sizes(face, 0, 48);

    // Load characters
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // Disable byte-alignment restriction
    for (unsigned char c = 32; c < 128; c++)
    {
        // Load character glyph
        if (FT_Load_Char(face, c, FT_LOAD_RENDER))
        {
            std::cerr << "Failed to load Glyph: " << c << std::endl;
            continue;
        }
        // Generate texture
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
        // Set texture options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // Store character
        Character character = {
            texture,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            static_cast<GLuint>(face->glyph->advance.x)
        };
        Characters.insert(std::pair<char, Character>(c, character));
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    // Data points storage
    std::vector<glm::vec2> dataPoints;

    // Timing variables
    auto lastTime = std::chrono::high_resolution_clock::now();
    float spawnInterval = 0.05f; // Interval to add data points
    float xx = 1;
    float yy = 3;
    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        // Calculate delta time
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();

        // Add new data points
        if (deltaTime >= spawnInterval)
        {
            lastTime = currentTime;
            xx+=0.1;
            yy+=0.1;
            // Generate data point (sine wave example)
            float x = sin(xx);// Dynamic based on data points
            float y = yy; // Sine wave
            dataPoints.emplace_back(glm::vec2(x, y));

            // Remove points that are out of view
            float left = -1.0f * zoomLevel + panOffset.x;
            if (dataPoints.front().x < left)
            {
                dataPoints.erase(dataPoints.begin());
            }

            // Ensure maximum number of points
            if (dataPoints.size() > 1000)
                dataPoints.erase(dataPoints.begin());
        }

        // Process input/events
        glfwPollEvents();

        // Update projection matrix based on zoom and pan
        projection = glm::ortho(-1.0f * zoomLevel + panOffset.x, 1.0f * zoomLevel + panOffset.x,
            -1.0f * zoomLevel + panOffset.y, 1.0f * zoomLevel + panOffset.y,
            -1.0f, 1.0f);

        // Compute current view boundaries
        float left = -1.0f * zoomLevel + panOffset.x;
        float right = 1.0f * zoomLevel + panOffset.x;
        float bottom = -1.0f * zoomLevel + panOffset.y;
        float top = 1.0f * zoomLevel + panOffset.y;

        // Generate grid lines based on current view
        std::vector<float> newGridLines = generateGridLines(left, right, bottom, top);

        // Update grid VBO
        glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
        glBufferData(GL_ARRAY_BUFFER, newGridLines.size() * sizeof(float), &newGridLines[0], GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Compute grid spacing for labels
        float viewWidth = right - left;
        float viewHeight = top - bottom;
        float gridSpacingX = computeGridSpacing(viewWidth);
        float gridSpacingY = computeGridSpacing(viewHeight);

        // Generate tick labels based on grid spacing
        std::vector<std::pair<float, float>> tickLabels = generateTickLabels(left, right, bottom, top, gridSpacingX, gridSpacingY);

        // Clear the screen with background color
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Draw background (gradient)
        glUseProgram(bgShaderProgram);
        glBindVertexArray(bgVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        // Draw grid lines
        glUseProgram(shaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
        glUniform3f(glGetUniformLocation(shaderProgram, "color"), 0.5f, 0.5f, 0.5f); // Gray color
        glBindVertexArray(gridVAO);
        glDrawArrays(GL_LINES, 0, newGridLines.size() / 2);
        glBindVertexArray(0);

        // Draw axes with arrowheads
        glUseProgram(shaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
        glUniform3f(glGetUniformLocation(shaderProgram, "color"), 1.0f, 1.0f, 1.0f); // White color
        glBindVertexArray(axesVAO);
        glDrawArrays(GL_LINES, 0, 2); // Main X and Y axes
        glDrawArrays(GL_LINES, 2, 4); // X-axis arrowhead
        glDrawArrays(GL_LINES, 6, 4); // Y-axis arrowhead
        glBindVertexArray(0);

        // Draw data lines
        glBindBuffer(GL_ARRAY_BUFFER, pointsVBO);
        if (!dataPoints.empty())
        {
            glBufferSubData(GL_ARRAY_BUFFER, 0, dataPoints.size() * sizeof(glm::vec2), &dataPoints[0]);
        }
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glUseProgram(shaderProgram);
        glUniform3f(glGetUniformLocation(shaderProgram, "color"), 0.0f, 1.0f, 0.0f); // Green color
        glBindVertexArray(pointsVAO);
        glLineWidth(2.0f); // Thicker lines
        glDrawArrays(GL_LINE_STRIP, 0, dataPoints.size());
        glBindVertexArray(0);

        // Render tick labels
        glUseProgram(textShaderProgram);
        for (auto& label : tickLabels)
        {
            // X-axis labels (below the axis)
            if (label.second <= bottom + 0.05f * zoomLevel)
            {
                std::string num = std::to_string(label.first);
                RenderText(face, num, label.first, bottom - 0.05f * zoomLevel, 0.002f * zoomLevel, glm::vec3(1.0f, 1.0f, 1.0f), textShaderProgram, projection);
            }

            // Y-axis labels (left of the axis)
            if (label.first <= left + 0.05f * zoomLevel)
            {
                std::string num = std::to_string(label.second);
                RenderText(face, num, left - 0.05f * zoomLevel, label.second, 0.002f * zoomLevel, glm::vec3(1.0f, 1.0f, 1.0f), textShaderProgram, projection);
            }
        }

        // Render title
        RenderText(face, "Real-Time Data Visualization", left + 0.05f * zoomLevel, top - 0.05f * zoomLevel, 0.002f * zoomLevel, glm::vec3(1.0f, 1.0f, 1.0f), textShaderProgram, projection);

        // Render "X" and "Y" labels
        RenderText(face, "X", right + 0.05f * zoomLevel, -0.05f * zoomLevel, 0.002f * zoomLevel, glm::vec3(1.0f, 1.0f, 1.0f), textShaderProgram, projection);
        RenderText(face, "Y", -0.05f * zoomLevel, top + 0.05f * zoomLevel, 0.002f * zoomLevel, glm::vec3(1.0f, 1.0f, 1.0f), textShaderProgram, projection);

        // Swap buffers
        glfwSwapBuffers(window);

        // Limit to ~60 FPS
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    // Cleanup
    glDeleteVertexArrays(1, &bgVAO);
    glDeleteBuffers(1, &bgVBO);
    glDeleteVertexArrays(1, &gridVAO);
    glDeleteBuffers(1, &gridVBO);
    glDeleteVertexArrays(1, &axesVAO);
    glDeleteBuffers(1, &axesVBO);
    glDeleteVertexArrays(1, &pointsVAO);
    glDeleteBuffers(1, &pointsVBO);
    glDeleteVertexArrays(1, &textVAO);
    glDeleteBuffers(1, &textVBO);
    glDeleteProgram(shaderProgram);
    glDeleteProgram(textShaderProgram);
    glDeleteProgram(bgShaderProgram);

    glfwTerminate();
    return 0;
}
*/