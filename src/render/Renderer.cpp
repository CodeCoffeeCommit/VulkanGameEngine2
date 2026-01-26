// src/render/Renderer.cpp
// FIXED VERSION - Ensures font atlas is flushed BEFORE render pass

#include "Renderer.h"
#include "VulkanContext.h"
#include "SwapChain.h"
#include "GraphicsPipeline.h"
#include "UniformBuffer.h"
#include "Grid.h"
#include "Mesh.h"
#include "../core/Camera.h"
#include "../core/FrameData.h"
#include "../ui/FontSystem.h"  // For font atlas flushing before render pass
#include <iostream>
#include <stdexcept>
#include <array>

Renderer::Renderer() {}

Renderer::~Renderer() {}

void Renderer::init(VulkanContext* ctx, SwapChain* swap) {
    this->context = ctx;
    this->swapChain = swap;

    createCommandPool();

    uniformBuffer = new UniformBuffer();
    uniformBuffer->create(context, MAX_FRAMES_IN_FLIGHT);

    createPipeline();
    createCommandBuffers();
    createSyncObjects();
    createSceneObjects();

    std::cout << "[OK] Renderer initialized" << std::endl;
}

void Renderer::createPipeline() {
    pipeline = new GraphicsPipeline();
    pipeline->init(context, swapChain, uniformBuffer);
}

void Renderer::cleanupPipeline() {
    if (pipeline) {
        pipeline->cleanup();
        delete pipeline;
        pipeline = nullptr;
    }
}

void Renderer::onSwapChainRecreated(SwapChain* newSwapChain) {
    std::cout << "[Renderer] Updating for new swap chain..." << std::endl;

    this->swapChain = newSwapChain;

    cleanupPipeline();
    createPipeline();

    std::cout << "[Renderer] Updated for new swap chain" << std::endl;
}

void Renderer::setUIRenderCallback(std::function<void(VkCommandBuffer)> callback) {
    uiRenderCallback_ = callback;
}

void Renderer::cleanup() {
    if (!context) return;

    vkDeviceWaitIdle(context->getDevice());

    for (auto& [id, mesh] : meshCache) {
        if (mesh) {
            mesh->cleanup();
            delete mesh;
        }
    }
    meshCache.clear();

    if (grid) {
        grid->cleanup();
        delete grid;
        grid = nullptr;
    }

    cleanupPipeline();

    if (uniformBuffer) {
        uniformBuffer->cleanup();
        delete uniformBuffer;
        uniformBuffer = nullptr;
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (i < imageAvailableSemaphores.size() && imageAvailableSemaphores[i] != VK_NULL_HANDLE) {
            vkDestroySemaphore(context->getDevice(), imageAvailableSemaphores[i], nullptr);
        }
        if (i < renderFinishedSemaphores.size() && renderFinishedSemaphores[i] != VK_NULL_HANDLE) {
            vkDestroySemaphore(context->getDevice(), renderFinishedSemaphores[i], nullptr);
        }
        if (i < inFlightFences.size() && inFlightFences[i] != VK_NULL_HANDLE) {
            vkDestroyFence(context->getDevice(), inFlightFences[i], nullptr);
        }
    }
    imageAvailableSemaphores.clear();
    renderFinishedSemaphores.clear();
    inFlightFences.clear();

    if (commandPool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(context->getDevice(), commandPool, nullptr);
        commandPool = VK_NULL_HANDLE;
    }

    std::cout << "[OK] Renderer cleaned up" << std::endl;
}

void Renderer::createCommandPool() {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = context->getGraphicsQueueFamily();

    if (vkCreateCommandPool(context->getDevice(), &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create command pool!");
    }
}

void Renderer::createCommandBuffers() {
    commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

    if (vkAllocateCommandBuffers(context->getDevice(), &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffers!");
    }
}

void Renderer::createSyncObjects() {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(context->getDevice(), &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(context->getDevice(), &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(context->getDevice(), &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create synchronization objects!");
        }
    }
}

void Renderer::createSceneObjects() {
    grid = new Grid();
    grid->create(context, 10.0f, 20);
    std::cout << "[OK] Scene objects created" << std::endl;
}

void Renderer::submitMesh(Mesh* mesh, const glm::mat4& transform, const glm::vec3& color, bool selected) {
    if (!mesh) {
        std::cerr << "[Renderer] WARNING: submitMesh called with null mesh!" << std::endl;
        return;
    }

    RenderObject obj;
    obj.mesh = mesh;
    obj.transform = transform;
    obj.color = color;
    obj.selected = selected;
    renderQueue.push_back(obj);
}

void Renderer::clearSubmissions() {
    renderQueue.clear();
}

Mesh* Renderer::getOrCreateMesh(uint64_t entityId, const void* vertexData, size_t vertexCount,
    const uint32_t* indexData, size_t indexCount) {

    // Check cache first
    auto it = meshCache.find(entityId);
    if (it != meshCache.end()) {
        return it->second;
    }

    // Don't create mesh if no vertex data provided
    if (vertexData == nullptr || vertexCount == 0) {
        std::cerr << "[Renderer] Cannot create mesh " << entityId << ": no vertex data" << std::endl;
        return nullptr;
    }

    if (indexData == nullptr || indexCount == 0) {
        std::cerr << "[Renderer] Cannot create mesh " << entityId << ": no index data" << std::endl;
        return nullptr;
    }

    std::cout << "[Renderer] Creating mesh for entity " << entityId
        << " with " << vertexCount << " vertices and "
        << indexCount << " indices" << std::endl;

    Mesh* mesh = new Mesh();

    // Convert UploadVertex to Vertex
    const libre::UploadVertex* uploadVerts = static_cast<const libre::UploadVertex*>(vertexData);

    std::vector<Vertex> vertices;
    vertices.reserve(vertexCount);

    for (size_t i = 0; i < vertexCount; ++i) {
        Vertex v;
        v.position = uploadVerts[i].position;
        v.normal = uploadVerts[i].normal;
        v.color = uploadVerts[i].color;
        vertices.push_back(v);
    }

    mesh->setVertices(vertices);
    mesh->setIndices(std::vector<uint32_t>(indexData, indexData + indexCount));
    mesh->create(context);

    meshCache[entityId] = mesh;

    std::cout << "[Renderer] Mesh " << entityId << " created successfully. "
        << "Cache size: " << meshCache.size() << std::endl;

    return mesh;
}

Mesh* Renderer::getMeshFromCache(uint64_t entityId) {
    auto it = meshCache.find(entityId);
    if (it != meshCache.end()) {
        return it->second;
    }
    return nullptr;
}

void Renderer::removeMesh(uint64_t entityId) {
    auto it = meshCache.find(entityId);
    if (it != meshCache.end()) {
        if (it->second) {
            it->second->cleanup();
            delete it->second;
        }
        meshCache.erase(it);
        std::cout << "[Renderer] Removed mesh " << entityId << std::endl;
    }
}

void Renderer::waitIdle() {
    if (context && context->getDevice()) {
        vkDeviceWaitIdle(context->getDevice());
    }
}

bool Renderer::drawFrame(Camera* camera) {
    // Wait for previous frame with this index to complete
    vkWaitForFences(context->getDevice(), 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    // ========================================================================
    // FLUSH FONT ATLAS BEFORE ACQUIRING IMAGE (outside of any render pass)
    // This is the FIX - font atlas upload must happen before render pass
    // flushAtlas() returns early if not initialized or not dirty
    // ========================================================================
    libre::ui::FontSystem::instance().flushAtlas(VK_NULL_HANDLE);

    // Acquire next image
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(
        context->getDevice(),
        swapChain->getSwapChain(),
        UINT64_MAX,
        imageAvailableSemaphores[currentFrame],
        VK_NULL_HANDLE,
        &imageIndex
    );

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        std::cout << "[Renderer] Swap chain out of date" << std::endl;
        return false;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Failed to acquire swap chain image!");
    }

    vkResetFences(context->getDevice(), 1, &inFlightFences[currentFrame]);

    updateUniformBuffer(currentFrame, camera);

    vkResetCommandBuffer(commandBuffers[currentFrame], 0);
    recordCommandBuffer(commandBuffers[currentFrame], imageIndex, camera);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

    VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(context->getGraphicsQueue(), 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("Failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { swapChain->getSwapChain() };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(context->getPresentQueue(), &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        std::cout << "[Renderer] Swap chain suboptimal/out of date after present" << std::endl;
        return false;
    }
    else if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to present swap chain image!");
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    return true;
}

void Renderer::updateUniformBuffer(uint32_t currentImage, Camera* camera) {
    UniformBufferObject ubo{};
    ubo.view = camera->getViewMatrix();
    ubo.projection = camera->getProjectionMatrix();
    ubo.lightDir = glm::normalize(glm::vec3(0.5f, 0.7f, 0.5f));
    ubo.viewPos = camera->getPosition();

    uniformBuffer->update(currentImage, ubo);
}

void Renderer::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, Camera* camera) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin recording command buffer!");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = swapChain->getRenderPass();
    renderPassInfo.framebuffer = swapChain->getFramebuffers()[imageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = swapChain->getExtent();

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = { {0.22f, 0.22f, 0.22f, 1.0f} };
    clearValues[1].depthStencil = { 1.0f, 0 };

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Set dynamic viewport and scissor
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapChain->getExtent().width);
    viewport.height = static_cast<float>(swapChain->getExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = swapChain->getExtent();
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    VkDescriptorSet descriptorSet = uniformBuffer->getDescriptorSet(currentFrame);

    // ========================================
    // 1. Draw grid
    // ========================================
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->getGridPipeline());
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline->getGridPipelineLayout(), 0, 1, &descriptorSet, 0, nullptr);

    PushConstants gridPush{};
    gridPush.model = glm::mat4(1.0f);
    vkCmdPushConstants(commandBuffer, pipeline->getGridPipelineLayout(),
        VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &gridPush);

    grid->bind(commandBuffer);
    grid->draw(commandBuffer);

    // ========================================
    // 2. Draw meshes
    // ========================================
    if (!renderQueue.empty()) {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->getMeshPipeline());
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipeline->getMeshPipelineLayout(), 0, 1, &descriptorSet, 0, nullptr);

        for (const auto& obj : renderQueue) {
            if (obj.mesh) {
                PushConstants push{};
                push.model = obj.transform;
                vkCmdPushConstants(commandBuffer, pipeline->getMeshPipelineLayout(),
                    VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &push);

                obj.mesh->bind(commandBuffer);
                obj.mesh->draw(commandBuffer);
            }
        }
    }

    // ========================================
    // 3. Render UI (MUST be last - draws on top)
    // Font atlas already flushed before render pass started
    // ========================================
    if (uiRenderCallback_) {
        uiRenderCallback_(commandBuffer);
    }

    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to record command buffer!");
    }
}