#pragma once

#include "Mesh.h"

class VulkanContext;

namespace Primitives {

    // Create primitive meshes for Vulkan rendering
    Mesh* createCube(VulkanContext* context, float size = 1.0f);
    Mesh* createSphere(VulkanContext* context, float radius = 1.0f, int segments = 32, int rings = 16);
    Mesh* createPlane(VulkanContext* context, float size = 10.0f, int subdivisions = 1);

} // namespace Primitives