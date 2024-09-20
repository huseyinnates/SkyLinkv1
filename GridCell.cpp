#include "GridCell.h"
#include "DataProvider.h"
#include "Renderer.h"

namespace SkyLink {

    GridCell::GridCell(float x, float y, float width, float height)
        : x(x), y(y), width(width), height(height),
        active(false), data(0), dataProvider(nullptr),
        key(-1) {} // key varsayılan olarak -1 (geçersiz tuş)

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

    void GridCell::setKeyCallback(int key, std::function<void()> callback) {
        this->key = key;
        onKeyCallback = callback;
    }

    void GridCell::setMouseCallback(std::function<void()> callback) {
        onMouseCallback = callback;
    }

    bool GridCell::containsPoint(float px, float py) {
        return (px >= x && px <= x + width && py >= y && py <= y + height);
    }

} // namespace SkyLine
