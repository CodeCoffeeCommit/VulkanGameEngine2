#pragma once

#include "World.h"
#include "../components/CoreComponents.h"
#include <glm/glm.hpp>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace libre {

    class Primitives {
    public:
        static EntityHandle createCube(World& world, float size = 1.0f,
            const std::string& name = "Cube") {
            auto entity = world.createEntity(name, "mesh");
            EntityID id = entity.getID();

            MeshComponent mesh;
            generateCubeMesh(mesh, size);
            world.addComponent<MeshComponent>(id, mesh);

            RenderComponent render;
            render.baseColor = glm::vec3(0.8f, 0.8f, 0.8f);
            world.addComponent<RenderComponent>(id, render);

            BoundsComponent bounds;
            float h = size * 0.5f;
            bounds.localMin = glm::vec3(-h);
            bounds.localMax = glm::vec3(h);
            world.addComponent<BoundsComponent>(id, bounds);

            return entity;
        }

        static EntityHandle createSphere(World& world, float radius = 1.0f,
            int segments = 32, int rings = 16,
            const std::string& name = "Sphere") {
            auto entity = world.createEntity(name, "mesh");
            EntityID id = entity.getID();

            MeshComponent mesh;
            generateSphereMesh(mesh, radius, segments, rings);
            world.addComponent<MeshComponent>(id, mesh);

            RenderComponent render;
            render.baseColor = glm::vec3(0.8f, 0.8f, 0.8f);
            world.addComponent<RenderComponent>(id, render);

            BoundsComponent bounds;
            bounds.localMin = glm::vec3(-radius);
            bounds.localMax = glm::vec3(radius);
            world.addComponent<BoundsComponent>(id, bounds);

            return entity;
        }

        static EntityHandle createCylinder(World& world, float radius = 0.5f,
            float height = 2.0f, int segments = 32,
            const std::string& name = "Cylinder") {
            auto entity = world.createEntity(name, "mesh");
            EntityID id = entity.getID();

            MeshComponent mesh;
            generateCylinderMesh(mesh, radius, height, segments);
            world.addComponent<MeshComponent>(id, mesh);

            RenderComponent render;
            render.baseColor = glm::vec3(0.8f, 0.8f, 0.8f);
            world.addComponent<RenderComponent>(id, render);

            BoundsComponent bounds;
            float halfH = height * 0.5f;
            bounds.localMin = glm::vec3(-radius, -halfH, -radius);
            bounds.localMax = glm::vec3(radius, halfH, radius);
            world.addComponent<BoundsComponent>(id, bounds);

            return entity;
        }

    private:
        static void generateCubeMesh(MeshComponent& mesh, float size) {
            float h = size * 0.5f;
            glm::vec3 baseColor(0.8f);

            mesh.vertices = {
                // Front face (Z+)
                {{-h, -h,  h}, {0, 0, 1}, baseColor, {0, 0}},
                {{ h, -h,  h}, {0, 0, 1}, baseColor, {1, 0}},
                {{ h,  h,  h}, {0, 0, 1}, baseColor, {1, 1}},
                {{-h,  h,  h}, {0, 0, 1}, baseColor, {0, 1}},
                // Back face (Z-)
                {{ h, -h, -h}, {0, 0, -1}, baseColor, {0, 0}},
                {{-h, -h, -h}, {0, 0, -1}, baseColor, {1, 0}},
                {{-h,  h, -h}, {0, 0, -1}, baseColor, {1, 1}},
                {{ h,  h, -h}, {0, 0, -1}, baseColor, {0, 1}},
                // Top face (Y+)
                {{-h,  h,  h}, {0, 1, 0}, baseColor, {0, 0}},
                {{ h,  h,  h}, {0, 1, 0}, baseColor, {1, 0}},
                {{ h,  h, -h}, {0, 1, 0}, baseColor, {1, 1}},
                {{-h,  h, -h}, {0, 1, 0}, baseColor, {0, 1}},
                // Bottom face (Y-)
                {{-h, -h, -h}, {0, -1, 0}, baseColor, {0, 0}},
                {{ h, -h, -h}, {0, -1, 0}, baseColor, {1, 0}},
                {{ h, -h,  h}, {0, -1, 0}, baseColor, {1, 1}},
                {{-h, -h,  h}, {0, -1, 0}, baseColor, {0, 1}},
                // Right face (X+)
                {{ h, -h,  h}, {1, 0, 0}, baseColor, {0, 0}},
                {{ h, -h, -h}, {1, 0, 0}, baseColor, {1, 0}},
                {{ h,  h, -h}, {1, 0, 0}, baseColor, {1, 1}},
                {{ h,  h,  h}, {1, 0, 0}, baseColor, {0, 1}},
                // Left face (X-)
                {{-h, -h, -h}, {-1, 0, 0}, baseColor, {0, 0}},
                {{-h, -h,  h}, {-1, 0, 0}, baseColor, {1, 0}},
                {{-h,  h,  h}, {-1, 0, 0}, baseColor, {1, 1}},
                {{-h,  h, -h}, {-1, 0, 0}, baseColor, {0, 1}},
            };

            mesh.indices = {
                0, 1, 2, 2, 3, 0,       // Front
                4, 5, 6, 6, 7, 4,       // Back
                8, 9, 10, 10, 11, 8,    // Top
                12, 13, 14, 14, 15, 12, // Bottom
                16, 17, 18, 18, 19, 16, // Right
                20, 21, 22, 22, 23, 20  // Left
            };

            mesh.calculateBounds();
        }

        static void generateSphereMesh(MeshComponent& mesh, float radius, int segments, int rings) {
            glm::vec3 baseColor(0.8f);

            for (int y = 0; y <= rings; y++) {
                for (int x = 0; x <= segments; x++) {
                    float xSeg = static_cast<float>(x) / segments;
                    float ySeg = static_cast<float>(y) / rings;
                    float xPos = std::cos(xSeg * 2.0f * static_cast<float>(M_PI)) * std::sin(ySeg * static_cast<float>(M_PI));
                    float yPos = std::cos(ySeg * static_cast<float>(M_PI));
                    float zPos = std::sin(xSeg * 2.0f * static_cast<float>(M_PI)) * std::sin(ySeg * static_cast<float>(M_PI));

                    glm::vec3 pos(xPos * radius, yPos * radius, zPos * radius);
                    glm::vec3 normal = glm::normalize(pos);

                    MeshVertex v;
                    v.position = pos;
                    v.normal = normal;
                    v.color = baseColor;
                    v.uv = glm::vec2(xSeg, ySeg);
                    mesh.vertices.push_back(v);
                }
            }

            for (int y = 0; y < rings; y++) {
                for (int x = 0; x < segments; x++) {
                    uint32_t i0 = y * (segments + 1) + x;
                    uint32_t i1 = i0 + 1;
                    uint32_t i2 = (y + 1) * (segments + 1) + x;
                    uint32_t i3 = i2 + 1;

                    mesh.indices.push_back(i0);
                    mesh.indices.push_back(i2);
                    mesh.indices.push_back(i1);
                    mesh.indices.push_back(i1);
                    mesh.indices.push_back(i2);
                    mesh.indices.push_back(i3);
                }
            }

            mesh.calculateBounds();
        }

        static void generateCylinderMesh(MeshComponent& mesh, float radius, float height, int segments) {
            glm::vec3 baseColor(0.8f);
            float halfH = height * 0.5f;

            // Side vertices
            for (int i = 0; i <= segments; i++) {
                float angle = static_cast<float>(i) / segments * 2.0f * static_cast<float>(M_PI);
                float x = std::cos(angle) * radius;
                float z = std::sin(angle) * radius;
                glm::vec3 normal = glm::normalize(glm::vec3(x, 0, z));

                MeshVertex vTop, vBot;
                vTop.position = glm::vec3(x, halfH, z);
                vTop.normal = normal;
                vTop.color = baseColor;
                vTop.uv = glm::vec2(static_cast<float>(i) / segments, 1.0f);

                vBot.position = glm::vec3(x, -halfH, z);
                vBot.normal = normal;
                vBot.color = baseColor;
                vBot.uv = glm::vec2(static_cast<float>(i) / segments, 0.0f);

                mesh.vertices.push_back(vTop);
                mesh.vertices.push_back(vBot);
            }

            // Side indices
            for (int i = 0; i < segments; i++) {
                uint32_t i0 = i * 2;
                uint32_t i1 = i0 + 1;
                uint32_t i2 = i0 + 2;
                uint32_t i3 = i0 + 3;

                mesh.indices.push_back(i0);
                mesh.indices.push_back(i1);
                mesh.indices.push_back(i2);
                mesh.indices.push_back(i2);
                mesh.indices.push_back(i1);
                mesh.indices.push_back(i3);
            }

            // Top cap center
            uint32_t topCenter = static_cast<uint32_t>(mesh.vertices.size());
            MeshVertex topCenterV;
            topCenterV.position = glm::vec3(0, halfH, 0);
            topCenterV.normal = glm::vec3(0, 1, 0);
            topCenterV.color = baseColor;
            topCenterV.uv = glm::vec2(0.5f, 0.5f);
            mesh.vertices.push_back(topCenterV);

            // Bottom cap center
            uint32_t botCenter = static_cast<uint32_t>(mesh.vertices.size());
            MeshVertex botCenterV;
            botCenterV.position = glm::vec3(0, -halfH, 0);
            botCenterV.normal = glm::vec3(0, -1, 0);
            botCenterV.color = baseColor;
            botCenterV.uv = glm::vec2(0.5f, 0.5f);
            mesh.vertices.push_back(botCenterV);

            // Cap vertices and indices
            uint32_t capStart = static_cast<uint32_t>(mesh.vertices.size());
            for (int i = 0; i <= segments; i++) {
                float angle = static_cast<float>(i) / segments * 2.0f * static_cast<float>(M_PI);
                float x = std::cos(angle) * radius;
                float z = std::sin(angle) * radius;

                MeshVertex vTop, vBot;
                vTop.position = glm::vec3(x, halfH, z);
                vTop.normal = glm::vec3(0, 1, 0);
                vTop.color = baseColor;

                vBot.position = glm::vec3(x, -halfH, z);
                vBot.normal = glm::vec3(0, -1, 0);
                vBot.color = baseColor;

                mesh.vertices.push_back(vTop);
                mesh.vertices.push_back(vBot);
            }

            for (int i = 0; i < segments; i++) {
                // Top cap
                mesh.indices.push_back(topCenter);
                mesh.indices.push_back(capStart + i * 2);
                mesh.indices.push_back(capStart + (i + 1) * 2);

                // Bottom cap
                mesh.indices.push_back(botCenter);
                mesh.indices.push_back(capStart + (i + 1) * 2 + 1);
                mesh.indices.push_back(capStart + i * 2 + 1);
            }

            mesh.calculateBounds();
        }
    };

} // namespace libre