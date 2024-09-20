#pragma once
#ifndef SKYLINE_GRIDCELL_H
#define SKYLINE_GRIDCELL_H

#include <string>
#include <memory>
#include "Observer.h"
#include "CellStrategy.h"

namespace SkyLink {

    class DataProvider;
    class Renderer;

    class GridCell : public Observer {
    public:
        float x, y;
        float width, height;
        bool active;
        std::string text;
        int data;

        std::unique_ptr<CellStrategy> strategy;
        DataProvider* dataProvider;

        GridCell(float x, float y, float width, float height);

        void setStrategy(CellStrategy* strat);
        void update();
        void draw(Renderer& renderer);
        void onDataUpdated(int newData) override;
    };

} // namespace SkyLine

#endif // SKYLINE_GRIDCELL_H
