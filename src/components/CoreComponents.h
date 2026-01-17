#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <vector>
#include <cstdint>
#include <string>

namespace libre {

    // ============================================================================
    // TRANSFORM COMPONENT
    // ============================================================================

    struct TransformComponent {
        glm::vec3 position = glm::vec3(0.0f);
        glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        glm::vec3 scale = glm::vec3(1.0f);

        // Cached world matrix
        glm::mat4 worldMatrix = glm::mat4(1.0f);
        bool dirty = true;

        // Compute local transform matrix
        glm::mat4 getLocalMatrix() const {
            glm::mat4 t = glm::translate(glm::mat4(1.0f), position);
            glm::mat4 r = glm::mat4_cast(rotation);
            glm::mat4 s = glm::scale(glm::mat4(1.0f), scale);
            return t * r * s;
        }

        // Helper setters
        void setPosition(float x, float y, float z) {
            position = glm::vec3(x, y, z);
            dirty = true;
        }

        void setRotationEuler(float pitch, float yaw, float roll) {
            rotation = glm::quat(glm::vec3(
                glm::radians(pitch),
                glm::radians(yaw),
                glm::radians(roll)
            ));
            dirty = true;
        }

        void setScale(float uniform) {
            scale = glm::vec3(uniform);
            dirty = true;
        }

        void setScale(float x, float y, float z) {
            scale = glm::vec3(x, y, z);
            dirty = true;
        }
    };

    // ============================================================================
    // MESH COMPONENT - Geometry data for rendering
    // ============================================================================

    struct MeshVertex {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec3 color;
        glm::vec2 uv;
    };

    struct MeshComponent {
        std::vector<MeshVertex> vertices;
        std::vector<uint32_t> indices;

        // Bounding box for culling/selection
        glm::vec3 boundsMin = glm::vec3(0.0f);
        glm::vec3 boundsMax = glm::vec3(0.0f);

        // GPU buffer handles (set by renderer)
        uint64_t vertexBufferHandle = 0;
        uint64_t indexBufferHandle = 0;
        bool gpuDirty = true;

        // Calculate bounds from vertices
        void calculateBounds() {
            if (vertices.empty()) {
                boundsMin = boundsMax = glm::vec3(0.0f);
                return;
            }

            boundsMin = boundsMax = vertices[0].position;
            for (const auto& v : vertices) {
                boundsMin = glm::min(boundsMin, v.position);
                boundsMax = glm::max(boundsMax, v.position);
            }
        }

        // Get center of bounding box
        glm::vec3 getCenter() const {
            return (boundsMin + boundsMax) * 0.5f;
        }

        // Get size of bounding box
        glm::vec3 getSize() const {
            return boundsMax - boundsMin;
        }

        size_t getVertexCount() const { return vertices.size(); }
        size_t getIndexCount() const { return indices.size(); }
        size_t getTriangleCount() const { return indices.size() / 3; }
    };

    // ============================================================================
    // RENDER COMPONENT - Visual properties
    // ============================================================================

    struct RenderComponent {
        glm::vec3 baseColor = glm::vec3(0.8f, 0.8f, 0.8f);
        float metallic = 0.0f;
        float roughness = 0.5f;
        float opacity = 1.0f;

        bool visible = true;
        bool castShadows = true;
        bool receiveShadows = true;

        // Display mode
        enum class DisplayMode : uint8_t {
            Solid,
            Wireframe,
            SolidWireframe,
            Textured,
            MaterialPreview
        };
        DisplayMode displayMode = DisplayMode::Solid;

        // Selection highlight
        bool isSelected = false;
        bool isHovered = false;
        glm::vec3 selectionColor = glm::vec3(1.0f, 0.5f, 0.0f);
    };

    // ============================================================================
    // BOUNDS COMPONENT - For selection and culling
    // ============================================================================

    struct BoundsComponent {
        // Object-space bounding box
        glm::vec3 localMin = glm::vec3(-0.5f);
        glm::vec3 localMax = glm::vec3(0.5f);

        // World-space (cached, updated from transform)
        glm::vec3 worldMin = glm::vec3(-0.5f);
        glm::vec3 worldMax = glm::vec3(0.5f);
        glm::vec3 worldCenter = glm::vec3(0.0f);
        float worldRadius = 1.0f;  // Bounding sphere

        bool dirty = true;

        // Update world bounds from transform
        void updateWorldBounds(const glm::mat4& worldMatrix) {
            // Transform all 8 corners of local AABB
            glm::vec3 corners[8] = {
                glm::vec3(localMin.x, localMin.y, localMin.z),
                glm::vec3(localMax.x, localMin.y, localMin.z),
                glm::vec3(localMin.x, localMax.y, localMin.z),
                glm::vec3(localMax.x, localMax.y, localMin.z),
                glm::vec3(localMin.x, localMin.y, localMax.z),
                glm::vec3(localMax.x, localMin.y, localMax.z),
                glm::vec3(localMin.x, localMax.y, localMax.z),
                glm::vec3(localMax.x, localMax.y, localMax.z),
            };

            worldMin = glm::vec3(std::numeric_limits<float>::max());
            worldMax = glm::vec3(std::numeric_limits<float>::lowest());

            for (int i = 0; i < 8; ++i) {
                glm::vec4 transformed = worldMatrix * glm::vec4(corners[i], 1.0f);
                glm::vec3 p = glm::vec3(transformed);
                worldMin = glm::min(worldMin, p);
                worldMax = glm::max(worldMax, p);
            }

            worldCenter = (worldMin + worldMax) * 0.5f;
            worldRadius = glm::length(worldMax - worldCenter);
            dirty = false;
        }

        // Ray intersection test
        bool intersectsRay(const glm::vec3& origin, const glm::vec3& direction, float& tMin, float& tMax) const {
            glm::vec3 invDir = 1.0f / direction;

            glm::vec3 t0 = (worldMin - origin) * invDir;
            glm::vec3 t1 = (worldMax - origin) * invDir;

            glm::vec3 tSmall = glm::min(t0, t1);
            glm::vec3 tBig = glm::max(t0, t1);

            tMin = glm::max(glm::max(tSmall.x, tSmall.y), tSmall.z);
            tMax = glm::min(glm::min(tBig.x, tBig.y), tBig.z);

            return tMax >= tMin && tMax > 0.0f;
        }
    };

    // ============================================================================
    // HIERARCHY COMPONENT - Parent/child relationships
    // ============================================================================

    struct HierarchyComponent {
        uint64_t parent = 0;  // INVALID_ENTITY = 0
        std::vector<uint64_t> children;

        int depth = 0;  // Depth in hierarchy (root = 0)

        bool hasParent() const { return parent != 0; }
        bool hasChildren() const { return !children.empty(); }

        void addChild(uint64_t child) {
            if (std::find(children.begin(), children.end(), child) == children.end()) {
                children.push_back(child);
            }
        }

        void removeChild(uint64_t child) {
            children.erase(
                std::remove(children.begin(), children.end(), child),
                children.end()
            );
        }
    };

    // ============================================================================
    // NAME COMPONENT - Simple name storage
    // ============================================================================

    struct NameComponent {
        std::string name;
        std::string type;  // Entity type for filtering

        NameComponent() = default;
        NameComponent(const std::string& n, const std::string& t = "")
            : name(n), type(t) {
        }
    };

} // namespace libre