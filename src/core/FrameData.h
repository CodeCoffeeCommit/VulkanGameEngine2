// src/core/FrameData.h
// 
// FrameData is the communication structure between Main Thread and Render Thread.
// Main thread WRITES to this, Render thread READS from it.
// This structure is COPIED, never shared by pointer.
//
// Design principles:
// - No pointers to mutable data (only IDs/handles)
// - POD types or types with trivial copy
// - Everything render thread needs for ONE frame
//

#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <cstdint>

namespace libre {

    // ============================================================================
    // GPU RESOURCE HANDLES
    // ============================================================================
    // These are IDs that reference GPU resources, NOT pointers.
    // The render thread uses these to look up actual Vulkan objects.

    using MeshHandle = uint64_t;
    using TextureHandle = uint64_t;
    using MaterialHandle = uint64_t;
    using BufferHandle = uint64_t;

    constexpr MeshHandle INVALID_MESH_HANDLE = 0;
    constexpr TextureHandle INVALID_TEXTURE_HANDLE = 0;
    constexpr MaterialHandle INVALID_MATERIAL_HANDLE = 0;
    constexpr BufferHandle INVALID_BUFFER_HANDLE = 0;

    // ============================================================================
    // RENDERABLE OBJECT
    // ============================================================================
    // Represents one object to be drawn. Uses handles, not pointers.

    struct RenderableMesh {
        MeshHandle meshHandle = INVALID_MESH_HANDLE;    // GPU mesh to draw
        glm::mat4 modelMatrix = glm::mat4(1.0f);        // World transform
        glm::vec4 color = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);  // Base color/tint
        MaterialHandle material = INVALID_MATERIAL_HANDLE;     // Material (future)

        // Selection/highlight state
        bool isSelected = false;
        bool isHovered = false;

        // Entity ID for picking (maps back to ECS)
        uint64_t entityId = 0;
    };

    // ============================================================================
    // DIRTY REGION (For future sculpting - incremental GPU updates)
    // ============================================================================
    // When sculpting, we don't re-upload entire mesh, just changed vertices.

    struct MeshDirtyRegion {
        MeshHandle meshHandle = INVALID_MESH_HANDLE;
        uint32_t startVertex = 0;       // First vertex that changed
        uint32_t vertexCount = 0;       // Number of vertices changed
        const void* vertexData = nullptr;  // Pointer to NEW vertex data (valid only during submission)

        // Note: vertexData pointer is only valid during submitFrameData() call.
        // Render thread must copy or upload immediately.
    };

    // ============================================================================
    // CAMERA DATA
    // ============================================================================

    struct CameraData {
        glm::mat4 viewMatrix = glm::mat4(1.0f);
        glm::mat4 projectionMatrix = glm::mat4(1.0f);
        glm::vec3 position = glm::vec3(0.0f, 0.0f, 5.0f);
        glm::vec3 forward = glm::vec3(0.0f, 0.0f, -1.0f);
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

        float nearPlane = 0.1f;
        float farPlane = 1000.0f;
        float fov = 45.0f;              // Degrees
        float aspectRatio = 16.0f / 9.0f;
    };

    // ============================================================================
    // LIGHTING DATA
    // ============================================================================

    struct LightData {
        glm::vec3 direction = glm::normalize(glm::vec3(0.5f, 0.7f, 0.5f));
        glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f);
        float intensity = 1.0f;
        float ambientStrength = 0.15f;
    };

    // ============================================================================
    // VIEWPORT DATA
    // ============================================================================

    struct ViewportData {
        uint32_t width = 1280;
        uint32_t height = 720;
        uint32_t x = 0;                 // Viewport offset (for multi-viewport future)
        uint32_t y = 0;

        // Clear color
        glm::vec4 clearColor = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);

        // Grid settings
        bool showGrid = true;
        float gridSize = 10.0f;
        float gridSpacing = 1.0f;
    };

    // ============================================================================
    // UI RENDER DATA
    // ============================================================================
    // Minimal data needed to render UI. Actual UI draw commands come via callback.

    struct UIRenderData {
        float screenWidth = 1280.0f;
        float screenHeight = 720.0f;
        float dpiScale = 1.0f;
        bool uiNeedsRedraw = true;      // Optimization: skip UI render if unchanged
    };

    // ============================================================================
    // DEBUG/GIZMO DATA (Future)
    // ============================================================================

    struct GizmoData {
        bool showTransformGizmo = false;
        glm::mat4 gizmoTransform = glm::mat4(1.0f);
        int activeAxis = -1;            // -1 = none, 0 = X, 1 = Y, 2 = Z

        // Brush cursor (for sculpting)
        bool showBrushCursor = false;
        glm::vec3 brushPosition = glm::vec3(0.0f);
        float brushRadius = 1.0f;
        glm::vec3 brushNormal = glm::vec3(0.0f, 1.0f, 0.0f);
    };

    // ============================================================================
    // FRAME DATA - The main structure passed to render thread
    // ============================================================================

    struct FrameData {
        // Frame identification
        uint64_t frameNumber = 0;
        float deltaTime = 0.016f;       // Time since last frame
        float totalTime = 0.0f;         // Total elapsed time

        // Camera
        CameraData camera;

        // Lighting
        LightData light;

        // Viewport
        ViewportData viewport;

        // Objects to render (copied each frame)
        std::vector<RenderableMesh> meshes;

        // Incremental mesh updates (for sculpting - future)
        std::vector<MeshDirtyRegion> dirtyRegions;

        // UI
        UIRenderData ui;

        // Gizmos/Debug visualization
        GizmoData gizmo;

        // Render flags
        bool wireframeMode = false;
        bool showNormals = false;       // Debug: show vertex normals
        bool enableShadows = false;     // Future

        // ========================================================================
        // Helper methods
        // ========================================================================

        void clear() {
            meshes.clear();
            dirtyRegions.clear();
            frameNumber = 0;
        }

        void addMesh(MeshHandle handle, const glm::mat4& transform,
            uint64_t entityId = 0, bool selected = false) {
            RenderableMesh rm;
            rm.meshHandle = handle;
            rm.modelMatrix = transform;
            rm.entityId = entityId;
            rm.isSelected = selected;
            meshes.push_back(rm);
        }

        void addMesh(const RenderableMesh& mesh) {
            meshes.push_back(mesh);
        }
    };

    // ============================================================================
    // RENDER THREAD COMMANDS (Future - for more complex operations)
    // ============================================================================
    // These would be used for things like "create new GPU resource" or 
    // "dispatch compute shader" that need to happen on render thread.

    enum class RenderCommandType {
        None,
        CreateMesh,
        DestroyMesh,
        UpdateMeshRegion,
        CreateTexture,
        DestroyTexture,
        DispatchCompute,        // For simulations
    };

    struct RenderCommand {
        RenderCommandType type = RenderCommandType::None;
        uint64_t resourceId = 0;
        // Additional data would be in a variant or union
    };

} // namespace libre