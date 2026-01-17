#pragma once

#include "Types.h"
#include <vector>
#include <unordered_map>
#include <optional>
#include <cassert>
#include <algorithm>

namespace libre {

    // ============================================================================
    // COMPONENT STORAGE BASE
    // ============================================================================

    class IComponentStorage {
    public:
        virtual ~IComponentStorage() = default;
        virtual void remove(EntityID entity) = 0;
        virtual bool has(EntityID entity) const = 0;
        virtual void clear() = 0;
        virtual size_t size() const = 0;
    };

    // ============================================================================
    // COMPONENT STORAGE - Dense array with entity mapping
    // ============================================================================
    // Optimized for iteration (cache-friendly) while maintaining O(1) lookup

    template<typename T>
    class ComponentStorage : public IComponentStorage {
    public:
        // Add or replace component
        T& add(EntityID entity, const T& component = T{}) {
            auto it = entityToIndex_.find(entity);

            if (it != entityToIndex_.end()) {
                // Replace existing
                components_[it->second] = component;
                return components_[it->second];
            }

            // Add new
            size_t index = components_.size();
            components_.push_back(component);
            entities_.push_back(entity);
            entityToIndex_[entity] = index;

            return components_.back();
        }

        // Get component (returns nullptr if not found)
        T* get(EntityID entity) {
            auto it = entityToIndex_.find(entity);
            if (it == entityToIndex_.end()) return nullptr;
            return &components_[it->second];
        }

        const T* get(EntityID entity) const {
            auto it = entityToIndex_.find(entity);
            if (it == entityToIndex_.end()) return nullptr;
            return &components_[it->second];
        }

        // Check if entity has component
        bool has(EntityID entity) const override {
            return entityToIndex_.find(entity) != entityToIndex_.end();
        }

        // Remove component
        void remove(EntityID entity) override {
            auto it = entityToIndex_.find(entity);
            if (it == entityToIndex_.end()) return;

            size_t index = it->second;
            size_t lastIndex = components_.size() - 1;

            if (index != lastIndex) {
                // Swap with last element
                components_[index] = std::move(components_[lastIndex]);
                entities_[index] = entities_[lastIndex];
                entityToIndex_[entities_[index]] = index;
            }

            components_.pop_back();
            entities_.pop_back();
            entityToIndex_.erase(entity);
        }

        // Clear all components
        void clear() override {
            components_.clear();
            entities_.clear();
            entityToIndex_.clear();
        }

        // Get count
        size_t size() const override {
            return components_.size();
        }

        // ========================================================================
        // ITERATION - Cache-friendly access to all components
        // ========================================================================

        // Iterate over all components with entity ID
        template<typename Func>
        void forEach(Func&& func) {
            for (size_t i = 0; i < components_.size(); ++i) {
                func(entities_[i], components_[i]);
            }
        }

        template<typename Func>
        void forEach(Func&& func) const {
            for (size_t i = 0; i < components_.size(); ++i) {
                func(entities_[i], components_[i]);
            }
        }

        // Direct access to arrays (for tight loops)
        T* data() { return components_.data(); }
        const T* data() const { return components_.data(); }

        EntityID* entityData() { return entities_.data(); }
        const EntityID* entityData() const { return entities_.data(); }

        // Iterator support
        auto begin() { return components_.begin(); }
        auto end() { return components_.end(); }
        auto begin() const { return components_.begin(); }
        auto end() const { return components_.end(); }

        // Get all entities with this component
        const std::vector<EntityID>& getEntities() const { return entities_; }

    private:
        std::vector<T> components_;                          // Dense array
        std::vector<EntityID> entities_;                     // Parallel entity IDs
        std::unordered_map<EntityID, size_t> entityToIndex_; // Sparse lookup
    };

    // ============================================================================
    // SOA COMPONENT STORAGE - Structure of Arrays for high-performance data
    // ============================================================================
    // For data like positions where you want to iterate X, Y, Z separately

    template<typename T>
    class SoAComponentStorage : public IComponentStorage {
    public:
        // Get raw arrays
        std::vector<float>& getX() { return x_; }
        std::vector<float>& getY() { return y_; }
        std::vector<float>& getZ() { return z_; }

        const std::vector<float>& getX() const { return x_; }
        const std::vector<float>& getY() const { return y_; }
        const std::vector<float>& getZ() const { return z_; }

        // Add position
        void add(EntityID entity, float px, float py, float pz) {
            auto it = entityToIndex_.find(entity);

            if (it != entityToIndex_.end()) {
                size_t idx = it->second;
                x_[idx] = px;
                y_[idx] = py;
                z_[idx] = pz;
                return;
            }

            size_t index = x_.size();
            x_.push_back(px);
            y_.push_back(py);
            z_.push_back(pz);
            entities_.push_back(entity);
            entityToIndex_[entity] = index;
        }

        // Get position
        bool get(EntityID entity, float& px, float& py, float& pz) const {
            auto it = entityToIndex_.find(entity);
            if (it == entityToIndex_.end()) return false;

            size_t idx = it->second;
            px = x_[idx];
            py = y_[idx];
            pz = z_[idx];
            return true;
        }

        bool has(EntityID entity) const override {
            return entityToIndex_.find(entity) != entityToIndex_.end();
        }

        void remove(EntityID entity) override {
            auto it = entityToIndex_.find(entity);
            if (it == entityToIndex_.end()) return;

            size_t index = it->second;
            size_t lastIndex = x_.size() - 1;

            if (index != lastIndex) {
                x_[index] = x_[lastIndex];
                y_[index] = y_[lastIndex];
                z_[index] = z_[lastIndex];
                entities_[index] = entities_[lastIndex];
                entityToIndex_[entities_[index]] = index;
            }

            x_.pop_back();
            y_.pop_back();
            z_.pop_back();
            entities_.pop_back();
            entityToIndex_.erase(entity);
        }

        void clear() override {
            x_.clear();
            y_.clear();
            z_.clear();
            entities_.clear();
            entityToIndex_.clear();
        }

        size_t size() const override { return x_.size(); }

        const std::vector<EntityID>& getEntities() const { return entities_; }

    private:
        std::vector<float> x_, y_, z_;
        std::vector<EntityID> entities_;
        std::unordered_map<EntityID, size_t> entityToIndex_;
    };

} // namespace libre