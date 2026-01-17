#include "Camera.h"
#include <algorithm>
#include <cmath>

Camera::Camera() {
    reset();
}

void Camera::reset() {
    distance = 10.0f;
    azimuth = 45.0f;
    elevation = 30.0f;
    target = glm::vec3(0.0f, 0.0f, 0.0f);
    update();
}

void Camera::setFront() {
    azimuth = 0.0f;
    elevation = 0.0f;
    update();
}

void Camera::setRight() {
    azimuth = 90.0f;
    elevation = 0.0f;
    update();
}

void Camera::setTop() {
    azimuth = 0.0f;
    elevation = 89.9f;
    update();
}

void Camera::orbit(float deltaX, float deltaY) {
    azimuth -= deltaX * orbitSensitivity;
    elevation += deltaY * orbitSensitivity;

    // Clamp elevation to prevent flipping
    elevation = std::clamp(elevation, -89.0f, 89.0f);

    // Wrap azimuth
    if (azimuth > 360.0f) azimuth -= 360.0f;
    if (azimuth < 0.0f) azimuth += 360.0f;

    update();
}

void Camera::pan(float deltaX, float deltaY) {
    // Calculate right and up vectors relative to camera
    glm::vec3 forward = glm::normalize(target - position);
    glm::vec3 right = glm::normalize(glm::cross(forward, up));
    glm::vec3 camUp = glm::normalize(glm::cross(right, forward));

    // Pan the target
    target -= right * deltaX * panSensitivity * distance;
    target += camUp * deltaY * panSensitivity * distance;

    update();
}

void Camera::zoom(float delta) {
    distance -= delta * zoomSensitivity;
    distance = std::clamp(distance, 0.5f, 500.0f);
    update();
}

void Camera::setAspectRatio(float aspect) {
    aspectRatio = aspect;
    update();
}

void Camera::updatePosition() {
    // Convert spherical to Cartesian coordinates
    float azimuthRad = glm::radians(azimuth);
    float elevationRad = glm::radians(elevation);

    position.x = target.x + distance * cos(elevationRad) * sin(azimuthRad);
    position.y = target.y + distance * sin(elevationRad);
    position.z = target.z + distance * cos(elevationRad) * cos(azimuthRad);
}

void Camera::update() {
    updatePosition();

    // View matrix
    viewMatrix = glm::lookAt(position, target, up);

    // Projection matrix (Vulkan clip space)
    projectionMatrix = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
    projectionMatrix[1][1] *= -1; // Flip Y for Vulkan
}