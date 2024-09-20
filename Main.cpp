#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "Renderer.h"
#include "GridSystem.h"
#include "DataProvider.h"
#include "CellStrategy.h"
#include <iostream>
#include <random>

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

        // Çizim
        renderer->clear();
        grid.draw(*renderer);

        // GLFW buffer swap ve event polling
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Temizlik
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
