#ifndef USERINTERFACE_H
#define USERINTERFACE_H

#include "Renderer.h"
#include "GridSystem.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>

namespace SkyLink {
    namespace UserInterface {
        enum class ScreenState {
            Monitoring,
            MissionControl,
            Model,
            VisualScripting
        };

        extern ScreenState currentScreen;

        void drawMonitoringScreen(Renderer& renderer, GridSystem* gridSystem);
        void drawColoredTriangle(GLuint shaderProgram, float r, float g, float b);
        void drawMissionControlScreen(GLuint shaderProgram);
        void drawModelScreen(GLuint shaderProgram);
        void drawVisualScriptingScreen(GLuint shaderProgram);
        void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
        void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

        // ImGui ile ilgili fonksiyonlar
        void setupImGui(GLFWwindow* window, const int WIDTH, const int TOOLBAR_HEIGHT);
        void renderToolbar();
        void cleanupImGui();
    }
}

#endif // USERINTERFACE_H
