#include "CellStrategy.h"
#include "GridCell.h"
#include "Renderer.h"
#include <algorithm>

namespace SkyLink {

    // TriangleCellStrategy için fonksiyonlar
    void TriangleCellStrategy::update(GridCell& cell) {
        // Güncelleme işlemleri
    }

    void TriangleCellStrategy::draw(GridCell& cell, Renderer& renderer) {
        GLfloat centerX = cell.x + cell.width / 2.0f;
        GLfloat centerY = cell.y + cell.height / 2.0f;
        GLfloat size = std::min(cell.width, cell.height) * 0.4f;

        renderer.drawTriangle(centerX, centerY, size, glm::vec3(1.0f, 0.0f, 0.0f));

        if (!cell.text.empty()) {
            renderer.renderText(cell.text, cell.x + 10,
                cell.y + cell.height - 30, 0.5f, glm::vec3(1.0f, 1.0f, 1.0f));
        }
    }

    // TriangleBlueCellStrategy için fonksiyonlar
    void TriangleBlueCellStrategy::update(GridCell& cell) {
        // Güncelleme işlemleri
    }

    void TriangleBlueCellStrategy::draw(GridCell& cell, Renderer& renderer) {
        GLfloat centerX = cell.x + cell.width / 2.0f;
        GLfloat centerY = cell.y + cell.height / 2.0f;
        GLfloat size = std::min(cell.width, cell.height) * 0.4f;

        renderer.drawTriangle(centerX, centerY, size, glm::vec3(0.0f, 0.0f, 1.0f));

        if (!cell.text.empty()) {
            renderer.renderText(cell.text, cell.x + 10,
                cell.y + cell.height - 30, 0.5f, glm::vec3(1.0f, 1.0f, 1.0f));
        }
    }

} // namespace SkyLine
