#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "Renderer.h"
#include "GridSystem.h"
#include "DataProvider.h"
#include "CellStrategy.h"
#include <iostream>
#include <random>

// Dear ImGui başlık dosyalarını ekleyin
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_freetype.h"

using namespace SkyLink; // Düzeltme: 'SkyLink' yerine 'SkyLine' olmalı

// **Global değişkenler**
GridSystem* gridSystem = nullptr;
Renderer* renderer = nullptr;

// Tuş callback fonksiyonu
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        // Eğer 'A' tuşuna basıldıysa
        if (key == GLFW_KEY_A) {
            // 2. satırdaki tüm hücrelerin stratejilerini değiştir
            int row = 1; // Satır indeksleri 0'dan başlar, bu yüzden 1 ikinci satırdır
            for (int col = 0; col < gridSystem->cols; ++col) {
                auto cell = gridSystem->getCell(row, col);
                if (cell) {
                    cell->setStrategy(new TriangleBlueCellStrategy());
                }
            }
            std::cout << "'A' tuşuna basıldı, 2. satırdaki hücreler maviye döndü." << std::endl;
        }

        // Tüm hücreleri kontrol et
        for (auto& cell : gridSystem->cells) {
            if (cell->key == key && cell->onKeyCallback) {
                cell->onKeyCallback();
            }
        }
    }
}

// Mouse buton callback fonksiyonu
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_LEFT) {
        // Mouse konumunu al
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        // Y eksenini düzelt (OpenGL koordinat sistemine göre)
        ypos = HEIGHT - ypos;

        // Hangi hücreye tıklandığını bul
        for (auto& cell : gridSystem->cells) {
            if (cell->containsPoint(static_cast<float>(xpos), static_cast<float>(ypos)) && cell->onMouseCallback) {
                cell->onMouseCallback();
            }
        }
    }
}

int main() {
    // GLFW'yi başlat
    if (!glfwInit()) {
        std::cout << "GLFW başlatılamadı" << std::endl;
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Pencere oluştur
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Grid System", NULL, NULL);
    if (!window) {
        std::cout << "GLFW pencere oluşturulamadı" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // GLEW'i başlat
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cout << "GLEW başlatılamadı" << std::endl;
        return -1;
    }

    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Renderer nesnesini GLEW başlatıldıktan sonra oluşturun
    Renderer renderObj;
    renderer = &renderObj;

    // Grid sistemi oluştur
    GridSystem grid(4, 4);
    gridSystem = &grid;

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

    // **GLFW callback fonksiyonlarını ayarla**
    glfwSetKeyCallback(window, keyCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);

    // **Örnek olarak bazı hücrelere callback fonksiyonları atayalım**

    // Hücreye tuş callback'i atama (Örneğin, 'A' tuşu)
    grid.getCell(0, 0)->setKeyCallback(GLFW_KEY_A, []() {
        std::cout << "Hücre 0,0 'A' tuşuna basıldı!" << std::endl;
        });

    // Hücreye mouse callback'i atama
    grid.getCell(0, 0)->setMouseCallback([]() {
        std::cout << "Hücre 0,0 tıklandı!" << std::endl;
        });

    // **Dear ImGui Başlatma**
    // ImGui Context oluştur
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // **Özel Yazı Tipini Yükleme**
    ImFont* customFont = io.Fonts->AddFontFromFileTTF("C:/Company/GroundControl/SkyLink/orange_juice2.ttf", 16.0f);
    if (!customFont) {
        std::cout << "Özel yazı tipi yüklenemedi!" << std::endl;
    }
    else {
        io.FontDefault = customFont; // Özel yazı tipini varsayılan olarak ayarla
    }

    // **Buton Renklerini Turuncuya Ayarlama**
    ImGui::StyleColorsDark(); // Mevcut stilinizi koruyabilirsiniz
    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_Button] = ImVec4(1.0f, 0.5f, 0.0f, 1.0f); // Turuncu
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(1.0f, 0.6f, 0.1f, 1.0f); // Açık turuncu
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(1.0f, 0.4f, 0.0f, 1.0f); // Koyu turuncu

    // Platform/Renderer backend'lerini başlat
    const char* glsl_version = "#version 330";
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Ana döngü
    while (!glfwWindowShouldClose(window)) {
        // Girdi işlemleri
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        // Veri güncelleme
        int randomData = distribution(generator);
        dataProvider.setData(randomData);

        // Hücre stratejisini değiştirme (örnek)
        // grid.cells[0]->setStrategy(new TriangleBlueCellStrategy());

        // Güncelleme
        grid.update();

        // **ImGui Frame Başlatma**
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // **ImGui Arayüzünü Oluşturma**
        // Ekranın üst kısmına sabit bir ImGui penceresi ekleyelim
        ImGui::Begin("Toolbar", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
        ImGui::SetWindowPos(ImVec2(0, 0));
        ImGui::SetWindowSize(ImVec2(static_cast<float>(WIDTH), 50)); // Pencere genişliği ve yüksekliği
        ImGui::Text("Toolbar");

        // Örnek Düğmeler
        if (ImGui::Button("Button 1")) {
            std::cout << "Button 1 clicked!" << std::endl;
            // Buraya Button 1'in yapacağı işlemleri ekleyin
        }
        ImGui::SameLine();
        if (ImGui::Button("Button 2")) {
            std::cout << "Button 2 clicked!" << std::endl;
            // Buraya Button 2'nin yapacağı işlemleri ekleyin
        }
        ImGui::SameLine();
        if (ImGui::Button("Button 3")) {
            std::cout << "Button 3 clicked!" << std::endl;
            // Buraya Button 3'ün yapacağı işlemleri ekleyin
        }

        ImGui::End();

        // **ImGui Rendering**
        ImGui::Render();

        // Çizim
        renderer->clear();
        grid.draw(*renderer);

        // ImGui'yi OpenGL ile render et
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // GLFW buffer swap ve event polling
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // **Temizlik İşlemleri**
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}