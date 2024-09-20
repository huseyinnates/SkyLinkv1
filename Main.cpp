#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "Renderer.h"
#include "GridSystem.h"
#include "DataProvider.h"
#include "CellStrategy.h"
#include <iostream>
#include <random>

using namespace SkyLink;

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

        // Hücre stratejisini değiştirme (örnek)
        grid.cells[0]->setStrategy(new TriangleBlueCellStrategy());

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
