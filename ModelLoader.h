#ifndef SKYLINK_MODEL_MODELLOADER_H
#define SKYLINK_MODEL_MODELLOADER_H

#include "Mesh.h"
#include <string>

namespace SkyLink {
    namespace Model {

        class ModelLoader {
        public:
            ModelLoader();
            ~ModelLoader();

            bool loadModel(const std::string& path, Mesh& mesh);
        };

    } // namespace Model
} // namespace SkyLink

#endif // SKYLINK_MODEL_MODELLOADER_H
