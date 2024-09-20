#ifndef SKYLINE_CELLSTRATEGY_H
#define SKYLINE_CELLSTRATEGY_H

namespace SkyLink {

    class GridCell;
    class Renderer;

    class CellStrategy {
    public:
        virtual void update(GridCell& cell) = 0;
        virtual void draw(GridCell& cell, Renderer& renderer) = 0;
        virtual ~CellStrategy() = default;
    };

    class TriangleCellStrategy : public CellStrategy {
    public:
        void update(GridCell& cell) override;
        void draw(GridCell& cell, Renderer& renderer) override;
    };

    class TriangleBlueCellStrategy : public CellStrategy {
    public:
        void update(GridCell& cell) override;
        void draw(GridCell& cell, Renderer& renderer) override;
    };

} // namespace SkyLine

#endif // SKYLINE_CELLSTRATEGY_H
