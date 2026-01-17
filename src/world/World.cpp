#include "World.h"
#include <iostream>
#include <algorithm>

namespace libre {

    // ============================================================================
    // ENTITY HANDLE IMPLEMENTATIONS
    // ============================================================================

    bool EntityHandle::isValid() const {
        return world_ && id_ != INVALID_ENTITY && world_->entityExists(id_);
    }

    EntityMetadata* EntityHandle::getMetadata() {
        return world_ ? world_->getMetadata(id_) : nullptr;
    }

    const EntityMetadata* EntityHandle::getMetadata() const {
        return world_ ? world_->getMetadata(id_) : nullptr;
    }

    EntityHandle EntityHandle::getParent() const {
        if (!world_) return EntityHandle();
        return EntityHandle(world_, world_->getParent(id_));
    }

    std::vector<EntityHandle> EntityHandle::getChildren() const {
        std::vector<EntityHandle> result;
        if (world_) {
            for (EntityID childId : world_->getChildren(id_)) {
                result.emplace_back(world_, childId);
            }
        }
        return result;
    }

    void EntityHandle::setParent(EntityHandle parent) {
        if (world_) {
            world_->setParent(id_, parent.getID());
        }
    }

    // ============================================================================
    // WORLD IMPLEMENTATION
    // ============================================================================

    World::World() {
        std::cout << "[World] Created" << std::endl;
    }

    World::~World() {
        clear();
        std::cout << "[World] Destroyed" << std::endl;
    }

    // ========================================================================
    // ENTITY MANAGEMENT
    // ========================================================================

    EntityID World::generateEntityID() {
        uint32_t index = nextIndex_++;
        uint32_t generation = generations_[index]++;
        return makeEntityID(index, generation);
    }

    EntityHandle World::createEntity(const std::string& name, const std::string& type) {
        EntityID id = generateEntityID();
        entities_.insert(id);

        // Create metadata
        EntityMetadata meta;
        meta.name = name;
        meta.type = type;
        meta.flags = EntityFlags::Default;
        entityMetadata_[id] = meta;

        // Always add TransformComponent
        addComponent<TransformComponent>(id);

        return EntityHandle(this, id);
    }

    void World::destroyEntity(EntityID id) {
        if (!entityExists(id)) return;

        // Remove from selection
        deselect(id);

        // Remove all children first (cascade delete)
        auto children = getChildren(id);
        for (EntityID childId : children) {
            destroyEntity(childId);
        }

        // Remove relationships
        relationships_.removeEntity(id);

        // Remove all components
        for (auto& [typeIndex, storage] : componentStorages_) {
            storage->remove(id);
        }

        // Remove metadata and entity
        entityMetadata_.erase(id);
        entities_.erase(id);
    }

    bool World::entityExists(EntityID id) const {
        return entities_.find(id) != entities_.end();
    }

    EntityHandle World::getEntity(EntityID id) {
        if (entityExists(id)) {
            return EntityHandle(this, id);
        }
        return EntityHandle();
    }

    std::vector<EntityHandle> World::getAllEntities() {
        std::vector<EntityHandle> result;
        result.reserve(entities_.size());
        for (EntityID id : entities_) {
            result.emplace_back(this, id);
        }
        return result;
    }

    EntityMetadata* World::getMetadata(EntityID id) {
        auto it = entityMetadata_.find(id);
        return (it != entityMetadata_.end()) ? &it->second : nullptr;
    }

    const EntityMetadata* World::getMetadata(EntityID id) const {
        auto it = entityMetadata_.find(id);
        return (it != entityMetadata_.end()) ? &it->second : nullptr;
    }

    // ========================================================================
    // RELATIONSHIPS / HIERARCHY
    // ========================================================================

    void World::setParent(EntityID child, EntityID parent) {
        if (!entityExists(child)) return;
        if (parent != INVALID_ENTITY && !entityExists(parent)) return;

        // Prevent circular relationships
        if (parent != INVALID_ENTITY && relationships_.isAncestorOf(child, parent)) {
            std::cerr << "[World] Cannot set parent: would create circular hierarchy" << std::endl;
            return;
        }

        // Use RelationshipStore's setParent which handles removal of old parent
        relationships_.setParent(child, parent);

        // Mark transform as dirty
        if (auto* transform = getComponent<TransformComponent>(child)) {
            transform->dirty = true;
        }
    }

    EntityID World::getParent(EntityID child) const {
        return relationships_.getParent(child);
    }

    std::vector<EntityID> World::getChildren(EntityID parent) const {
        return relationships_.getChildren(parent);
    }

    std::vector<EntityID> World::getRootEntities() const {
        return relationships_.getRoots(entities_);
    }

    // ========================================================================
    // SELECTION
    // ========================================================================

    void World::select(EntityID entity) {
        if (!entityExists(entity)) return;
        if (isSelected(entity)) return;

        selection_.push_back(entity);
        activeEntity_ = entity;
    }

    void World::deselect(EntityID entity) {
        auto it = std::find(selection_.begin(), selection_.end(), entity);
        if (it != selection_.end()) {
            selection_.erase(it);

            // Update active entity
            if (activeEntity_ == entity) {
                activeEntity_ = selection_.empty() ? INVALID_ENTITY : selection_.back();
            }
        }
    }

    void World::setSelection(const std::vector<EntityID>& entities) {
        selection_.clear();
        for (EntityID id : entities) {
            if (entityExists(id)) {
                selection_.push_back(id);
            }
        }
        activeEntity_ = selection_.empty() ? INVALID_ENTITY : selection_.back();
    }

    void World::clearSelection() {
        selection_.clear();
        activeEntity_ = INVALID_ENTITY;
    }

    bool World::isSelected(EntityID entity) const {
        return std::find(selection_.begin(), selection_.end(), entity) != selection_.end();
    }

    // ========================================================================
    // UTILITY
    // ========================================================================

    void World::clear() {
        selection_.clear();
        activeEntity_ = INVALID_ENTITY;

        relationships_.clear();

        for (auto& [typeIndex, storage] : componentStorages_) {
            storage->clear();
        }

        entityMetadata_.clear();
        entities_.clear();

        // Reset ID generation but keep generations for safety
        nextIndex_ = 1;
    }

    std::vector<EntityHandle> World::findByName(const std::string& name) {
        std::vector<EntityHandle> result;
        for (EntityID id : entities_) {
            auto* meta = getMetadata(id);
            if (meta && meta->name == name) {
                result.emplace_back(this, id);
            }
        }
        return result;
    }

    std::vector<EntityHandle> World::findByType(const std::string& type) {
        std::vector<EntityHandle> result;
        for (EntityID id : entities_) {
            auto* meta = getMetadata(id);
            if (meta && meta->type == type) {
                result.emplace_back(this, id);
            }
        }
        return result;
    }

} // namespace libre