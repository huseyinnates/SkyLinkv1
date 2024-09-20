#include "GridCell.h"
#include "DataProvider.h"
#include "Renderer.h"

namespace SkyLink {

    GridCell::GridCell(float x, float y, float width, float height)
        : x(x), y(y), width(width), height(height),
        active(false), data(0), dataProvider(nullptr) {}

    void GridCell::setStrategy(CellStrategy* strat) {
        strategy.reset(strat);
    }

    void GridCell::update() {
        if (strategy) {
            strategy->update(*this);
        }
    }

    void GridCell::draw(Renderer& renderer) {
        if (strategy) {
            strategy->draw(*this, renderer);
        }
    }

    void GridCell::onDataUpdated(int newData) {
        data = newData;
        text = "Data: " + std::to_string(data);
    }

} // namespace SkyLine
