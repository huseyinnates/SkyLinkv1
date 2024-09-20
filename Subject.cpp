#include "Subject.h"

namespace SkyLink {

    void Subject::attach(Observer* observer) {
        observers.push_back(observer);
    }

    void Subject::detach(Observer* observer) {
        observers.erase(
            std::remove(observers.begin(), observers.end(), observer),
            observers.end());
    }

    void Subject::notify(int newData) {
        for (auto observer : observers) {
            observer->onDataUpdated(newData);
        }
    }

} // namespace SkyLine
