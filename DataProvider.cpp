#include "DataProvider.h"

namespace SkyLink {

    void DataProvider::setData(int newData) {
        data = newData;
        notify(data);
    }

} // namespace SkyLine
