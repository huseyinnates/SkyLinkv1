#ifndef SKYLINE_OBSERVER_H
#define SKYLINE_OBSERVER_H

namespace SkyLink {

    class Observer {
    public:
        virtual void onDataUpdated(int newData) = 0;
        virtual ~Observer() = default;
    };

} // namespace SkyLine

#endif // SKYLINE_OBSERVER_H
