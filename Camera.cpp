#include "Camera.h"

namespace SkyLink {
    namespace Model {

        Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch)
            : front(glm::vec3(0.0f, 0.0f, -1.0f)), fov(45.0f), distance(3.0f),
            position(position), up(up), yaw(yaw), pitch(pitch) {
            updateCameraVectors();
        }

        glm::mat4 Camera::getViewMatrix() {
            return glm::lookAt(front * distance, glm::vec3(0.0f, 0.0f, 0.0f), up);
        }

        void Camera::processMouseMovement(float xoffset, float yoffset) {
            float sensitivity = 0.1f;
            xoffset *= sensitivity;
            yoffset *= sensitivity;

            yaw += xoffset;
            pitch += yoffset;

            // Pitch sınırları
            if (pitch > 89.0f)
                pitch = 89.0f;
            if (pitch < -89.0f)
                pitch = -89.0f;

            updateCameraVectors();
        }

        void Camera::processMouseScroll(float yoffset) {
            if (fov >= 1.0f && fov <= 45.0f)
                fov -= yoffset;
            if (fov <= 1.0f)
                fov = 1.0f;
            if (fov >= 45.0f)
                fov = 45.0f;

            distance -= yoffset * 0.1f;
            if (distance < 1.0f)
                distance = 1.0f;
            if (distance > 10.0f)
                distance = 10.0f;
        }

        void Camera::updateCameraVectors() {
            glm::vec3 frontVec;
            frontVec.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
            frontVec.y = sin(glm::radians(pitch));
            frontVec.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
            front = glm::normalize(frontVec);
        }

    } // namespace Model
} // namespace SkyLink
