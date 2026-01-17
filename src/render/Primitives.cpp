#include "Primitives.h"
#include "VulkanContext.h"
#include <glm/glm.hpp>

namespace Primitives {

    Mesh* createCube(VulkanContext* context, float size) {
        Mesh* mesh = new Mesh();

        float h = size * 0.5f;

        // Blender workbench style - slightly different colors per face for depth
        glm::vec3 baseColor(0.8f, 0.8f, 0.8f);

        // Define vertices with positions, normals, and colors
        // Each face has its own vertices for proper normals
        std::vector<Vertex> vertices = {
            // Front face (Z+) - Normal: (0, 0, 1)
            {{-h, -h,  h}, {0.0f, 0.0f, 1.0f}, baseColor * 0.9f},
            {{ h, -h,  h}, {0.0f, 0.0f, 1.0f}, baseColor * 0.9f},
            {{ h,  h,  h}, {0.0f, 0.0f, 1.0f}, baseColor * 0.9f},
            {{-h,  h,  h}, {0.0f, 0.0f, 1.0f}, baseColor * 0.9f},

            // Back face (Z-) - Normal: (0, 0, -1)
            {{ h, -h, -h}, {0.0f, 0.0f, -1.0f}, baseColor * 0.7f},
            {{-h, -h, -h}, {0.0f, 0.0f, -1.0f}, baseColor * 0.7f},
            {{-h,  h, -h}, {0.0f, 0.0f, -1.0f}, baseColor * 0.7f},
            {{ h,  h, -h}, {0.0f, 0.0f, -1.0f}, baseColor * 0.7f},

            // Top face (Y+) - Normal: (0, 1, 0)
            {{-h,  h,  h}, {0.0f, 1.0f, 0.0f}, baseColor * 1.0f},
            {{ h,  h,  h}, {0.0f, 1.0f, 0.0f}, baseColor * 1.0f},
            {{ h,  h, -h}, {0.0f, 1.0f, 0.0f}, baseColor * 1.0f},
            {{-h,  h, -h}, {0.0f, 1.0f, 0.0f}, baseColor * 1.0f},

            // Bottom face (Y-) - Normal: (0, -1, 0)
            {{-h, -h, -h}, {0.0f, -1.0f, 0.0f}, baseColor * 0.5f},
            {{ h, -h, -h}, {0.0f, -1.0f, 0.0f}, baseColor * 0.5f},
            {{ h, -h,  h}, {0.0f, -1.0f, 0.0f}, baseColor * 0.5f},
            {{-h, -h,  h}, {0.0f, -1.0f, 0.0f}, baseColor * 0.5f},

            // Right face (X+) - Normal: (1, 0, 0)
            {{ h, -h,  h}, {1.0f, 0.0f, 0.0f}, baseColor * 0.85f},
            {{ h, -h, -h}, {1.0f, 0.0f, 0.0f}, baseColor * 0.85f},
            {{ h,  h, -h}, {1.0f, 0.0f, 0.0f}, baseColor * 0.85f},
            {{ h,  h,  h}, {1.0f, 0.0f, 0.0f}, baseColor * 0.85f},

            // Left face (X-) - Normal: (-1, 0, 0)
            {{-h, -h, -h}, {-1.0f, 0.0f, 0.0f}, baseColor * 0.65f},
            {{-h, -h,  h}, {-1.0f, 0.0f, 0.0f}, baseColor * 0.65f},
            {{-h,  h,  h}, {-1.0f, 0.0f, 0.0f}, baseColor * 0.65f},
            {{-h,  h, -h}, {-1.0f, 0.0f, 0.0f}, baseColor * 0.65f},
        };

        // Indices for triangles (2 triangles per face)
        std::vector<uint32_t> indices = {
            // Front
            0, 1, 2, 2, 3, 0,
            // Back
            4, 5, 6, 6, 7, 4,
            // Top
            8, 9, 10, 10, 11, 8,
            // Bottom
            12, 13, 14, 14, 15, 12,
            // Right
            16, 17, 18, 18, 19, 16,
            // Left
            20, 21, 22, 22, 23, 20
        };

        // Store edges for wireframe
        mesh->edges = {
            // Front face edges
            {0, 1}, {1, 2}, {2, 3}, {3, 0},
            // Back face edges
            {4, 5}, {5, 6}, {6, 7}, {7, 4},
            // Connecting edges
            {0, 5}, {1, 4}, {2, 7}, {3, 6}
        };

        // Store faces
        for (size_t i = 0; i < indices.size(); i += 3) {
            Face face;
            face.v0 = indices[i];
            face.v1 = indices[i + 1];
            face.v2 = indices[i + 2];
            face.normal = vertices[indices[i]].normal;
            mesh->faces.push_back(face);
        }

        mesh->setVertices(vertices);
        mesh->setIndices(indices);
        mesh->create(context);

        return mesh;
    }

    Mesh* createSphere(VulkanContext* context, float radius, int segments, int rings) {
        Mesh* mesh = new Mesh();

        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

        glm::vec3 baseColor(0.8f, 0.8f, 0.8f);

        for (int y = 0; y <= rings; y++) {
            for (int x = 0; x <= segments; x++) {
                float xSegment = (float)x / (float)segments;
                float ySegment = (float)y / (float)rings;
                float xPos = std::cos(xSegment * 2.0f * 3.14159f) * std::sin(ySegment * 3.14159f);
                float yPos = std::cos(ySegment * 3.14159f);
                float zPos = std::sin(xSegment * 2.0f * 3.14159f) * std::sin(ySegment * 3.14159f);

                glm::vec3 pos(xPos * radius, yPos * radius, zPos * radius);
                glm::vec3 normal = glm::normalize(pos);

                vertices.push_back({ pos, normal, baseColor });
            }
        }

        for (int y = 0; y < rings; y++) {
            for (int x = 0; x < segments; x++) {
                uint32_t i0 = y * (segments + 1) + x;
                uint32_t i1 = i0 + 1;
                uint32_t i2 = (y + 1) * (segments + 1) + x;
                uint32_t i3 = i2 + 1;

                indices.push_back(i0);
                indices.push_back(i2);
                indices.push_back(i1);

                indices.push_back(i1);
                indices.push_back(i2);
                indices.push_back(i3);
            }
        }

        mesh->setVertices(vertices);
        mesh->setIndices(indices);
        mesh->create(context);

        return mesh;
    }

    Mesh* createPlane(VulkanContext* context, float size, int subdivisions) {
        Mesh* mesh = new Mesh();

        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

        glm::vec3 baseColor(0.8f, 0.8f, 0.8f);
        glm::vec3 normal(0.0f, 1.0f, 0.0f);

        float h = size * 0.5f;
        float step = size / subdivisions;

        for (int z = 0; z <= subdivisions; z++) {
            for (int x = 0; x <= subdivisions; x++) {
                float xPos = -h + x * step;
                float zPos = -h + z * step;
                vertices.push_back({ {xPos, 0.0f, zPos}, normal, baseColor });
            }
        }

        for (int z = 0; z < subdivisions; z++) {
            for (int x = 0; x < subdivisions; x++) {
                uint32_t i0 = z * (subdivisions + 1) + x;
                uint32_t i1 = i0 + 1;
                uint32_t i2 = (z + 1) * (subdivisions + 1) + x;
                uint32_t i3 = i2 + 1;

                indices.push_back(i0);
                indices.push_back(i2);
                indices.push_back(i1);

                indices.push_back(i1);
                indices.push_back(i2);
                indices.push_back(i3);
            }
        }

        mesh->setVertices(vertices);
        mesh->setIndices(indices);
        mesh->create(context);

        return mesh;
    }

} // namespace Primitives