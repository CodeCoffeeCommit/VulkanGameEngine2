#pragma once

#include <cstdint>
#include <limits>
#include <string>

namespace libre {

    // ============================================================================
    // ENTITY ID
    // ============================================================================

    using EntityID = uint64_t;
    constexpr EntityID INVALID_ENTITY = 0;

    // Entity ID structure (for version tracking to detect stale handles)
    // High 32 bits: generation/version
    // Low 32 bits: index
    inline uint32_t getEntityIndex(EntityID id) {
        return static_cast<uint32_t>(id & 0xFFFFFFFF);
    }

    inline uint32_t getEntityGeneration(EntityID id) {
        return static_cast<uint32_t>(id >> 32);
    }

    inline EntityID makeEntityID(uint32_t index, uint32_t generation) {
        return (static_cast<uint64_t>(generation) << 32) | index;
    }

    // ============================================================================
    // ENTITY FLAGS
    // ============================================================================

    enum class EntityFlags : uint32_t {
        None = 0,
        Visible = 1 << 0,
        Selectable = 1 << 1,
        Locked = 1 << 2,
        Hidden = 1 << 3,
        Frozen = 1 << 4,

        Default = Visible | Selectable
    };

    inline EntityFlags operator|(EntityFlags a, EntityFlags b) {
        return static_cast<EntityFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }

    inline EntityFlags operator&(EntityFlags a, EntityFlags b) {
        return static_cast<EntityFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
    }

    inline EntityFlags& operator|=(EntityFlags& a, EntityFlags b) {
        a = a | b;
        return a;
    }

    inline EntityFlags& operator&=(EntityFlags& a, EntityFlags b) {
        a = a & b;
        return a;
    }

    inline bool hasFlag(EntityFlags flags, EntityFlags flag) {
        return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(flag)) != 0;
    }

    // ============================================================================
    // ENTITY METADATA
    // ============================================================================

    struct EntityMetadata {
        std::string name;
        std::string type;           // "mesh", "light", "camera", etc.
        EntityFlags flags = EntityFlags::Default;
        uint32_t layer = 0;         // Layer for organization

        bool isVisible() const { return hasFlag(flags, EntityFlags::Visible); }
        bool isSelectable() const { return hasFlag(flags, EntityFlags::Selectable); }
        bool isLocked() const { return hasFlag(flags, EntityFlags::Locked); }
    };

    // ============================================================================
    // RELATIONSHIP TYPES
    // ============================================================================

    enum class RelationType : uint8_t {
        ParentChild,        // Scene hierarchy
        LayerStack,         // 2D layer ordering
        NodeConnection,     // Node graph edges
        GroupMember,        // Grouping
        Reference,          // Asset references
        Constraint,         // Constraints (IK, etc.)
    };

    // ============================================================================
    // COMPONENT TYPE IDS
    // ============================================================================

    using ComponentTypeID = size_t;

    template<typename T>
    ComponentTypeID getComponentTypeID() {
        static ComponentTypeID id = typeid(T).hash_code();
        return id;
    }

} // namespace libre