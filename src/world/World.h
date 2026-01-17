#pragma once

#include "Types.h"
#include "ComponentStorage.h"
#include "RelationshipStore.h"
#include "../components/CoreComponents.h"

#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <typeindex>
#include <vector>
#include <string>
#include <functional>

namespace libre {

    // Forward declaration
    class World;

    // ============================================================================
    // ENTITY HANDLE - Safe wrapper for entity access
    // ============================================================================

    class EntityHandle {
    public:
        EntityHandle() : world_(nullptr), id_(INVALID_ENTITY) {}
        EntityHandle(World* world, EntityID id) : world_(world), id_(id) {}

        bool isValid() const;
        EntityID getID() const { return id_; }

        // Component access
        template<typename T> T* get();
        template<typename T> const T* get() const;
        template<typename T> T& add(const T& component = T{});
        template<typename T> bool has() const;
        template<typename T> void remove();

        // Metadata
        EntityMetadata* getMetadata();
        const EntityMetadata* getMetadata() const;

        // Hierarchy
        EntityHandle getParent() const;
        std::vector<EntityHandle> getChildren() const;
        void setParent(EntityHandle parent);

        // Operators
        bool operator==(const EntityHandle& other) const { return id_ == other.id_; }
        bool operator!=(const EntityHandle& other) const { return id_ != other.id_; }
        operator bool() const { return isValid(); }

    private:
        World* world_;
        EntityID id_;
    };

    // ============================================================================
    // WORLD - Central ECS container
    // ============================================================================

    class World {
    public:
        World();
        ~World();

        // ========================================================================
        // ENTITY MANAGEMENT
        // ========================================================================

        EntityHandle createEntity(const std::string& name = "Entity",
            const std::string& type = "");
        void destroyEntity(EntityID id);
        bool entityExists(EntityID id) const;
        EntityHandle getEntity(EntityID id);

        // Get all entities
        std::vector<EntityHandle> getAllEntities();
        size_t getEntityCount() const { return entities_.size(); }

        // Entity metadata
        EntityMetadata* getMetadata(EntityID id);
        const EntityMetadata* getMetadata(EntityID id) const;

        // ========================================================================
        // COMPONENT MANAGEMENT
        // ========================================================================

        template<typename T>
        T& addComponent(EntityID entity, const T& component = T{}) {
            auto& storage = getOrCreateStorage<T>();
            return storage.add(entity, component);
        }

        template<typename T>
        T* getComponent(EntityID entity) {
            auto* storage = getStorage<T>();
            return storage ? storage->get(entity) : nullptr;
        }

        template<typename T>
        const T* getComponent(EntityID entity) const {
            auto* storage = getStorage<T>();
            return storage ? storage->get(entity) : nullptr;
        }

        template<typename T>
        bool hasComponent(EntityID entity) const {
            auto* storage = getStorage<T>();
            return storage ? storage->has(entity) : false;
        }

        template<typename T>
        void removeComponent(EntityID entity) {
            auto* storage = getStorage<T>();
            if (storage) storage->remove(entity);
        }

        // Iterate over all entities with component
        template<typename T, typename Func>
        void forEach(Func&& func) {
            auto* storage = getStorage<T>();
            if (storage) {
                storage->forEach(std::forward<Func>(func));
            }
        }

        // Get component storage directly (for tight loops)
        template<typename T>
        ComponentStorage<T>* getStorage() {
            auto it = componentStorages_.find(std::type_index(typeid(T)));
            if (it == componentStorages_.end()) return nullptr;
            return static_cast<ComponentStorage<T>*>(it->second.get());
        }

        template<typename T>
        const ComponentStorage<T>* getStorage() const {
            auto it = componentStorages_.find(std::type_index(typeid(T)));
            if (it == componentStorages_.end()) return nullptr;
            return static_cast<const ComponentStorage<T>*>(it->second.get());
        }

        // ========================================================================
        // RELATIONSHIPS / HIERARCHY
        // ========================================================================

        void setParent(EntityID child, EntityID parent);
        EntityID getParent(EntityID child) const;
        std::vector<EntityID> getChildren(EntityID parent) const;
        std::vector<EntityID> getRootEntities() const;

        RelationshipStore& getRelationships() { return relationships_; }
        const RelationshipStore& getRelationships() const { return relationships_; }

        // ========================================================================
        // SELECTION
        // ========================================================================

        void select(EntityID entity);
        void deselect(EntityID entity);
        void setSelection(const std::vector<EntityID>& entities);
        void clearSelection();
        bool isSelected(EntityID entity) const;

        const std::vector<EntityID>& getSelection() const { return selection_; }
        EntityID getActiveEntity() const { return activeEntity_; }
        void setActiveEntity(EntityID entity) { activeEntity_ = entity; }

        // ========================================================================
        // UTILITY
        // ========================================================================

        void clear();

        // Find entities by name
        std::vector<EntityHandle> findByName(const std::string& name);

        // Find entities by type
        std::vector<EntityHandle> findByType(const std::string& type);

    private:
        template<typename T>
        ComponentStorage<T>& getOrCreateStorage() {
            auto key = std::type_index(typeid(T));
            auto it = componentStorages_.find(key);
            if (it == componentStorages_.end()) {
                auto storage = std::make_unique<ComponentStorage<T>>();
                auto* ptr = storage.get();
                componentStorages_[key] = std::move(storage);
                return *ptr;
            }
            return *static_cast<ComponentStorage<T>*>(it->second.get());
        }

        // Entity storage
        std::unordered_set<EntityID> entities_;
        std::unordered_map<EntityID, EntityMetadata> entityMetadata_;

        // Component storages
        std::unordered_map<std::type_index, std::unique_ptr<IComponentStorage>> componentStorages_;

        // Relationships
        RelationshipStore relationships_;

        // Selection
        std::vector<EntityID> selection_;
        EntityID activeEntity_ = INVALID_ENTITY;

        // ID generation
        uint32_t nextIndex_ = 1;
        std::unordered_map<uint32_t, uint32_t> generations_;

        EntityID generateEntityID();
    };

    // ============================================================================
    // ENTITY HANDLE TEMPLATE IMPLEMENTATIONS
    // ============================================================================

    template<typename T>
    T* EntityHandle::get() {
        return world_ ? world_->getComponent<T>(id_) : nullptr;
    }

    template<typename T>
    const T* EntityHandle::get() const {
        return world_ ? world_->getComponent<T>(id_) : nullptr;
    }

    template<typename T>
    T& EntityHandle::add(const T& component) {
        return world_->addComponent<T>(id_, component);
    }

    template<typename T>
    bool EntityHandle::has() const {
        return world_ ? world_->hasComponent<T>(id_) : false;
    }

    template<typename T>
    void EntityHandle::remove() {
        if (world_) world_->removeComponent<T>(id_);
    }

} // namespace libre