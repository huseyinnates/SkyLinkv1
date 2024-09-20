#ifndef SKYLINE_GRIDSYSTEM_H
#define SKYLINE_GRIDSYSTEM_H

#include <vector>
#include <memory>
#include "GridCell.h"

namespace SkyLink {

    class Renderer;

    class GridSystem {
    public:
        std::vector<std::shared_ptr<GridCell>> cells;
        int rows, cols;
        float cellWidth, cellHeight;

        GridSystem(int rows, int cols);

        void update();
        void draw(Renderer& renderer);
        void addCell(std::shared_ptr<GridCell> cell);
        void removeCell(std::shared_ptr<GridCell> cell);
        std::shared_ptr<GridCell> getCell(int row, int col);

    private:
        void createCells();
    };

} // namespace SkyLine

#endif // SKYLINE_GRIDSYSTEM_H
