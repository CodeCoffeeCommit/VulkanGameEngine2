#pragma once

#include "../world/World.h"
#include "../components/CoreComponents.h"
#include "Camera.h"
#include <glm/glm.hpp>
#include <limits>

namespace libre {

    // Ray for raycasting
    struct Ray {
        glm::vec3 origin;
        glm::vec3 direction;
    };

    // Hit result from raycasting
    struct HitResult {
        EntityID entity = INVALID_ENTITY;
        float distance = std::numeric_limits<float>::max();
        glm::vec3 point = glm::vec3(0.0f);
        glm::vec3 normal = glm::vec3(0.0f);

        bool hit() const { return entity != INVALID_ENTITY; }
    };

    // Selection utilities
    class SelectionSystem {
    public:
        // Convert screen coordinates to world ray
        static Ray screenToRay(const Camera& camera, float screenX, float screenY,
            int viewportWidth, int viewportHeight) {
            // Normalize screen coordinates to [-1, 1]
            float x = (2.0f * screenX) / viewportWidth - 1.0f;
            float y = 1.0f - (2.0f * screenY) / viewportHeight;  // Flip Y

            // Create clip space coordinates
            glm::vec4 rayClip(x, y, -1.0f, 1.0f);

            // Transform to eye space
            glm::mat4 invProj = glm::inverse(camera.getProjectionMatrix());
            glm::vec4 rayEye = invProj * rayClip;
            rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);

            // Transform to world space
            glm::mat4 invView = glm::inverse(camera.getViewMatrix());
            glm::vec4 rayWorld = invView * rayEye;

            Ray ray;
            ray.origin = camera.getPosition();
            ray.direction = glm::normalize(glm::vec3(rayWorld));

            return ray;
        }

        // Raycast against all entities with BoundsComponent
        static HitResult raycast(World& world, const Ray& ray) {
            HitResult closest;

            world.forEach<BoundsComponent>([&](EntityID id, BoundsComponent& bounds) {
                // Skip if no transform or not visible
                auto* meta = world.getMetadata(id);
                if (meta && !meta->isVisible()) return;
                if (meta && !meta->isSelectable()) return;

                float tMin, tMax;
                if (bounds.intersectsRay(ray.origin, ray.direction, tMin, tMax)) {
                    if (tMin > 0 && tMin < closest.distance) {
                        closest.entity = id;
                        closest.distance = tMin;
                        closest.point = ray.origin + ray.direction * tMin;
                        // Approximate normal (pointing toward camera)
                        closest.normal = -ray.direction;
                    }
                }
                });

            return closest;
        }

        // Raycast against specific entity
        static bool raycastEntity(World& world, EntityID entity, const Ray& ray, HitResult& result) {
            auto* bounds = world.getComponent<BoundsComponent>(entity);
            if (!bounds) return false;

            float tMin, tMax;
            if (bounds->intersectsRay(ray.origin, ray.direction, tMin, tMax)) {
                if (tMin > 0) {
                    result.entity = entity;
                    result.distance = tMin;
                    result.point = ray.origin + ray.direction * tMin;
                    result.normal = -ray.direction;
                    return true;
                }
            }

            return false;
        }

        // Frustum culling helper (for selection boxes, etc.)
        static bool isInFrustum(const Camera& camera, const BoundsComponent& bounds) {
            // Simple sphere check against camera frustum
            // TODO: Implement proper frustum planes check
            glm::vec3 toCenter = bounds.worldCenter - camera.getPosition();
            float dist = glm::length(toCenter);

            // Very basic check - is it roughly in front of camera and not too far?
            glm::vec3 forward = glm::normalize(camera.getTarget() - camera.getPosition());
            float dotProduct = glm::dot(glm::normalize(toCenter), forward);

            return dotProduct > -0.2f && dist < 1000.0f;
        }

        // Box selection (marquee selection)
        static std::vector<EntityID> boxSelect(World& world, const Camera& camera,
            float x1, float y1, float x2, float y2,
            int viewportWidth, int viewportHeight) {
            std::vector<EntityID> selected;

            // Normalize coordinates
            float minX = std::min(x1, x2);
            float maxX = std::max(x1, x2);
            float minY = std::min(y1, y2);
            float maxY = std::max(y1, y2);

            glm::mat4 viewProj = camera.getProjectionMatrix() * camera.getViewMatrix();

            world.forEach<BoundsComponent>([&](EntityID id, BoundsComponent& bounds) {
                auto* meta = world.getMetadata(id);
                if (meta && !meta->isVisible()) return;
                if (meta && !meta->isSelectable()) return;

                // Project world center to screen
                glm::vec4 clipPos = viewProj * glm::vec4(bounds.worldCenter, 1.0f);

                if (clipPos.w > 0) {  // In front of camera
                    glm::vec3 ndc = glm::vec3(clipPos) / clipPos.w;

                    // Convert to screen coordinates
                    float screenX = (ndc.x + 1.0f) * 0.5f * viewportWidth;
                    float screenY = (1.0f - ndc.y) * 0.5f * viewportHeight;

                    // Check if in selection box
                    if (screenX >= minX && screenX <= maxX &&
                        screenY >= minY && screenY <= maxY) {
                        selected.push_back(id);
                    }
                }
                });

            return selected;
        }
    };

} // namespace libre