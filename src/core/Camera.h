// src/core/Camera.h

#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
    Camera();
    ~Camera() = default;

    // Update matrices (from orbit parameters)
    void update();

    // Blender-style orbit controls
    void orbit(float deltaX, float deltaY);
    void pan(float deltaX, float deltaY);
    void zoom(float delta);

    // Reset to default view
    void reset();
    void setFront();
    void setRight();
    void setTop();

    // Getters
    glm::mat4 getViewMatrix() const { return viewMatrix; }
    glm::mat4 getProjectionMatrix() const { return projectionMatrix; }
    glm::vec3 getPosition() const { return position; }
    glm::vec3 getTarget() const { return target; }

    // Set aspect ratio (call on resize)
    void setAspectRatio(float aspect);

    // ========================================================================
    // SETTERS for Render Thread (to use matrices from FrameData)
    // ========================================================================
    void setViewMatrix(const glm::mat4& view) { viewMatrix = view; }
    void setProjectionMatrix(const glm::mat4& proj) { projectionMatrix = proj; }
    void setPosition(const glm::vec3& pos) { position = pos; }

    // Camera settings (public for easy access)
    float fov = 45.0f;
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;

private:
    void updatePosition();

    // Orbit parameters (spherical coordinates)
    float distance = 10.0f;
    float azimuth = 45.0f;    // Horizontal angle (degrees)
    float elevation = 30.0f;   // Vertical angle (degrees)

    // Target point (what camera looks at)
    glm::vec3 target = glm::vec3(0.0f, 0.0f, 0.0f);

    // Computed values
    glm::vec3 position;
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

    // Matrices
    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;

    float aspectRatio = 16.0f / 9.0f;

    // Sensitivity
    float orbitSensitivity = 0.5f;
    float panSensitivity = 0.01f;
    float zoomSensitivity = 1.0f;
};