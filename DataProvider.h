#ifndef SKYLINE_DATAPROVIDER_H
#define SKYLINE_DATAPROVIDER_H

#include "Subject.h"

namespace SkyLink {

    class DataProvider : public Subject {
    public:
        int data;

        void setData(int newData);
    };

} // namespace SkyLine

#endif // SKYLINE_DATAPROVIDER_H
