#ifndef SKYLINE_GRIDCELL_H
#define SKYLINE_GRIDCELL_H

#include <string>
#include <memory>
#include <functional> // Callback fonksiyonları için
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

        // **Yeni eklenenler**
        // Callback fonksiyonları için
        std::function<void()> onKeyCallback;
        std::function<void()> onMouseCallback;

        // Hangi tuşla tetikleneceğini belirlemek için
        int key; // GLFW key kodu

        GridCell(float x, float y, float width, float height);

        void setStrategy(CellStrategy* strat);
        void update();
        void draw(Renderer& renderer);
        void onDataUpdated(int newData) override;

        // **Yeni fonksiyonlar**
        void setKeyCallback(int key, std::function<void()> callback);
        void setMouseCallback(std::function<void()> callback);
        bool containsPoint(float px, float py); // Mouse tıklamasında kullanmak için
    };

} // namespace SkyLine

#endif // SKYLINE_GRIDCELL_H
