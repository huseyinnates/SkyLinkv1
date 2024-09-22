#ifndef SKYLINK_MODEL_CAMERA_H
#define SKYLINK_MODEL_CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace SkyLink {
    namespace Model {

        class Camera {
        public:
            // Kamera özellikleri
            glm::vec3 position;
            glm::vec3 front;
            glm::vec3 up;

            float yaw;
            float pitch;
            float fov;
            float distance;

            Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch);

            glm::mat4 getViewMatrix();

            void processMouseMovement(float xoffset, float yoffset);
            void processMouseScroll(float yoffset);

        private:
            void updateCameraVectors();
        };

    } // namespace Model
} // namespace SkyLink

#endif // SKYLINK_MODEL_CAMERA_H
