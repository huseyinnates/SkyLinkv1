#include "GridSystem.h"
#include "Renderer.h"
#include "CellStrategy.h"
#include <algorithm>

namespace SkyLink {

    GridSystem::GridSystem(int rows, int cols)
        : rows(rows), cols(cols) {
        cellWidth = WIDTH / cols;
        cellHeight = HEIGHT / rows;
        createCells();
    }

    void GridSystem::createCells() {
        for (int i = 0; i < rows; ++i) {
            for (int j = 0; j < cols; ++j) {
                float x = j * cellWidth;
                float y = i * cellHeight;
                auto cell = std::make_shared<GridCell>(x, y, cellWidth, cellHeight);
                cell->setStrategy(new TriangleCellStrategy());
                cells.push_back(cell);
            }
        }
    }

    void GridSystem::update() {
        for (auto& cell : cells) {
            cell->update();
        }
    }

    void GridSystem::draw(Renderer& renderer) {
        for (auto& cell : cells) {
            cell->draw(renderer);
        }
    }

    void GridSystem::addCell(std::shared_ptr<GridCell> cell) {
        cells.push_back(cell);
    }

    void GridSystem::removeCell(std::shared_ptr<GridCell> cell) {
        cells.erase(std::remove(cells.begin(), cells.end(), cell), cells.end());
    }

    std::shared_ptr<GridCell> GridSystem::getCell(int row, int col) {
        if (row < 0 || row >= rows || col < 0 || col >= cols)
            return nullptr;
        int index = row * cols + col;
        return cells[index];
    }

} // namespace SkyLine
