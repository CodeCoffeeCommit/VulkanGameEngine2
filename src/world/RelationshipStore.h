#pragma once

#include "Types.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <functional>

namespace libre {

    // ============================================================================
    // RELATIONSHIP - Connection between two entities
    // ============================================================================

    struct Relationship {
        RelationType type;
        EntityID from;
        EntityID to;
        int32_t order = 0;  // For ordered relationships (layers, etc.)

        // Optional metadata
        std::string label;
        float weight = 1.0f;

        bool operator==(const Relationship& other) const {
            return type == other.type && from == other.from && to == other.to;
        }
    };

    // Hash for Relationship
    struct RelationshipHash {
        size_t operator()(const Relationship& r) const {
            size_t h1 = std::hash<uint8_t>{}(static_cast<uint8_t>(r.type));
            size_t h2 = std::hash<EntityID>{}(r.from);
            size_t h3 = std::hash<EntityID>{}(r.to);
            return h1 ^ (h2 << 1) ^ (h3 << 2);
        }
    };

    // ============================================================================
    // RELATIONSHIP STORE
    // ============================================================================

    class RelationshipStore {
    public:
        // ========================================================================
        // ADD/REMOVE RELATIONSHIPS
        // ========================================================================

        // Add a relationship
        void add(const Relationship& rel) {
            relationships_.insert(rel);

            // Index by 'from' entity
            fromIndex_[rel.from].push_back(rel);

            // Index by 'to' entity
            toIndex_[rel.to].push_back(rel);

            // Index by type
            typeIndex_[rel.type].push_back(rel);
        }

        // Add parent-child relationship (convenience)
        void setParent(EntityID child, EntityID parent) {
            // Remove existing parent relationship
            removeParent(child);

            if (parent != INVALID_ENTITY) {
                Relationship rel;
                rel.type = RelationType::ParentChild;
                rel.from = parent;
                rel.to = child;
                add(rel);
            }
        }

        // Remove a relationship
        void remove(const Relationship& rel) {
            relationships_.erase(rel);

            // Remove from indices
            removeFromVector(fromIndex_[rel.from], rel);
            removeFromVector(toIndex_[rel.to], rel);
            removeFromVector(typeIndex_[rel.type], rel);
        }

        // Remove parent relationship
        void removeParent(EntityID child) {
            auto it = toIndex_.find(child);
            if (it != toIndex_.end()) {
                auto& rels = it->second;
                for (auto rit = rels.begin(); rit != rels.end(); ) {
                    if (rit->type == RelationType::ParentChild) {
                        Relationship rel = *rit;
                        relationships_.erase(rel);
                        removeFromVector(fromIndex_[rel.from], rel);
                        removeFromVector(typeIndex_[rel.type], rel);
                        rit = rels.erase(rit);
                    }
                    else {
                        ++rit;
                    }
                }
            }
        }

        // Remove all relationships involving an entity
        void removeEntity(EntityID entity) {
            // Remove where entity is 'from'
            if (auto it = fromIndex_.find(entity); it != fromIndex_.end()) {
                for (const auto& rel : it->second) {
                    relationships_.erase(rel);
                    removeFromVector(toIndex_[rel.to], rel);
                    removeFromVector(typeIndex_[rel.type], rel);
                }
                fromIndex_.erase(it);
            }

            // Remove where entity is 'to'
            if (auto it = toIndex_.find(entity); it != toIndex_.end()) {
                for (const auto& rel : it->second) {
                    relationships_.erase(rel);
                    removeFromVector(fromIndex_[rel.from], rel);
                    removeFromVector(typeIndex_[rel.type], rel);
                }
                toIndex_.erase(it);
            }
        }

        // ========================================================================
        // QUERIES
        // ========================================================================

        // Get parent of entity
        EntityID getParent(EntityID child) const {
            auto it = toIndex_.find(child);
            if (it != toIndex_.end()) {
                for (const auto& rel : it->second) {
                    if (rel.type == RelationType::ParentChild) {
                        return rel.from;
                    }
                }
            }
            return INVALID_ENTITY;
        }

        // Get children of entity
        std::vector<EntityID> getChildren(EntityID parent) const {
            std::vector<EntityID> children;
            auto it = fromIndex_.find(parent);
            if (it != fromIndex_.end()) {
                for (const auto& rel : it->second) {
                    if (rel.type == RelationType::ParentChild) {
                        children.push_back(rel.to);
                    }
                }
            }
            return children;
        }

        // Get all root entities (no parent)
        std::vector<EntityID> getRoots(const std::unordered_set<EntityID>& allEntities) const {
            std::vector<EntityID> roots;
            for (EntityID id : allEntities) {
                if (getParent(id) == INVALID_ENTITY) {
                    roots.push_back(id);
                }
            }
            return roots;
        }

        // Get relationships from entity
        const std::vector<Relationship>& getFrom(EntityID entity) const {
            static std::vector<Relationship> empty;
            auto it = fromIndex_.find(entity);
            return (it != fromIndex_.end()) ? it->second : empty;
        }

        // Get relationships to entity
        const std::vector<Relationship>& getTo(EntityID entity) const {
            static std::vector<Relationship> empty;
            auto it = toIndex_.find(entity);
            return (it != toIndex_.end()) ? it->second : empty;
        }

        // Get relationships by type
        const std::vector<Relationship>& getByType(RelationType type) const {
            static std::vector<Relationship> empty;
            auto it = typeIndex_.find(type);
            return (it != typeIndex_.end()) ? it->second : empty;
        }

        // Check if relationship exists
        bool exists(EntityID from, EntityID to, RelationType type) const {
            Relationship rel;
            rel.type = type;
            rel.from = from;
            rel.to = to;
            return relationships_.find(rel) != relationships_.end();
        }

        // ========================================================================
        // HIERARCHY TRAVERSAL
        // ========================================================================

        // Traverse hierarchy depth-first
        template<typename Func>
        void traverseDepthFirst(EntityID root, Func&& func, int depth = 0) const {
            func(root, depth);
            for (EntityID child : getChildren(root)) {
                traverseDepthFirst(child, func, depth + 1);
            }
        }

        // Get all descendants
        std::vector<EntityID> getDescendants(EntityID ancestor) const {
            std::vector<EntityID> result;
            traverseDepthFirst(ancestor, [&result, ancestor](EntityID id, int) {
                if (id != ancestor) result.push_back(id);
                });
            return result;
        }

        // Get all ancestors (path to root)
        std::vector<EntityID> getAncestors(EntityID entity) const {
            std::vector<EntityID> ancestors;
            EntityID current = getParent(entity);
            while (current != INVALID_ENTITY) {
                ancestors.push_back(current);
                current = getParent(current);
            }
            return ancestors;
        }

        // Check if 'ancestor' is ancestor of 'descendant'
        bool isAncestorOf(EntityID ancestor, EntityID descendant) const {
            EntityID current = getParent(descendant);
            while (current != INVALID_ENTITY) {
                if (current == ancestor) return true;
                current = getParent(current);
            }
            return false;
        }

        // Clear all relationships
        void clear() {
            relationships_.clear();
            fromIndex_.clear();
            toIndex_.clear();
            typeIndex_.clear();
        }

        size_t size() const { return relationships_.size(); }

    private:
        void removeFromVector(std::vector<Relationship>& vec, const Relationship& rel) {
            vec.erase(
                std::remove(vec.begin(), vec.end(), rel),
                vec.end()
            );
        }

        std::unordered_set<Relationship, RelationshipHash> relationships_;
        std::unordered_map<EntityID, std::vector<Relationship>> fromIndex_;
        std::unordered_map<EntityID, std::vector<Relationship>> toIndex_;
        std::unordered_map<RelationType, std::vector<Relationship>> typeIndex_;
    };

} // namespace libre