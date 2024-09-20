#ifndef SKYLINE_SUBJECT_H
#define SKYLINE_SUBJECT_H

#include <vector>
#include <algorithm>
#include "Observer.h"

namespace SkyLink {

    class Subject {
    private:
        std::vector<Observer*> observers;
    public:
        void attach(Observer* observer);
        void detach(Observer* observer);
        void notify(int newData);
    };

} // namespace SkyLine

#endif // SKYLINE_SUBJECT_H
