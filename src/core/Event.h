#pragma once

#include <functional>
#include <vector>
#include <unordered_map>
#include <string>
#include <typeindex>
#include <memory>
#include <queue>
#include <mutex>
#include <cstdint>

namespace libre {

    // ============================================================================
    // EVENT BASE
    // ============================================================================

    struct Event {
        virtual ~Event() = default;
        virtual const char* getName() const = 0;
    };

    // ============================================================================
    // ENTITY EVENTS
    // ============================================================================

    struct EntityCreatedEvent : Event {
        uint64_t entityId;
        std::string name;
        const char* getName() const override { return "EntityCreated"; }
    };

    struct EntityDestroyedEvent : Event {
        uint64_t entityId;
        const char* getName() const override { return "EntityDestroyed"; }
    };

    struct EntityRenamedEvent : Event {
        uint64_t entityId;
        std::string oldName;
        std::string newName;
        const char* getName() const override { return "EntityRenamed"; }
    };

    // ============================================================================
    // COMPONENT EVENTS
    // ============================================================================

    struct ComponentAddedEvent : Event {
        uint64_t entityId;
        std::type_index componentType;
        ComponentAddedEvent() : componentType(typeid(void)) {}
        const char* getName() const override { return "ComponentAdded"; }
    };

    struct ComponentRemovedEvent : Event {
        uint64_t entityId;
        std::type_index componentType;
        ComponentRemovedEvent() : componentType(typeid(void)) {}
        const char* getName() const override { return "ComponentRemoved"; }
    };

    struct ComponentModifiedEvent : Event {
        uint64_t entityId;
        std::type_index componentType;
        std::string propertyName;
        ComponentModifiedEvent() : componentType(typeid(void)) {}
        const char* getName() const override { return "ComponentModified"; }
    };

    // ============================================================================
    // SELECTION EVENTS
    // ============================================================================

    struct SelectionChangedEvent : Event {
        std::vector<uint64_t> selectedEntities;
        uint64_t activeEntity = 0;
        const char* getName() const override { return "SelectionChanged"; }
    };

    // ============================================================================
    // SCENE EVENTS
    // ============================================================================

    struct SceneLoadedEvent : Event {
        std::string filePath;
        const char* getName() const override { return "SceneLoaded"; }
    };

    struct SceneSavedEvent : Event {
        std::string filePath;
        const char* getName() const override { return "SceneSaved"; }
    };

    struct SceneClearedEvent : Event {
        const char* getName() const override { return "SceneCleared"; }
    };

    // ============================================================================
    // TRANSFORM & MESH EVENTS
    // ============================================================================

    struct TransformChangedEvent : Event {
        uint64_t entityId;
        const char* getName() const override { return "TransformChanged"; }
    };

    struct MeshModifiedEvent : Event {
        uint64_t entityId;
        bool topologyChanged = false;
        const char* getName() const override { return "MeshModified"; }
    };

    // ============================================================================
    // UNDO/REDO EVENTS
    // ============================================================================

    struct UndoEvent : Event {
        std::string commandName;
        const char* getName() const override { return "Undo"; }
    };

    struct RedoEvent : Event {
        std::string commandName;
        const char* getName() const override { return "Redo"; }
    };

    // ============================================================================
    // TOOL EVENTS
    // ============================================================================

    struct ToolChangedEvent : Event {
        std::string previousTool;
        std::string newTool;
        const char* getName() const override { return "ToolChanged"; }
    };

    // ============================================================================
    // VIEWPORT EVENTS
    // ============================================================================

    struct ViewportResizedEvent : Event {
        int width = 0;
        int height = 0;
        const char* getName() const override { return "ViewportResized"; }
    };

    // ============================================================================
    // EVENT BUS - Central event dispatcher
    // ============================================================================

    class EventBus {
    public:
        using EventCallback = std::function<void(const Event&)>;
        using SubscriptionId = size_t;

        // Subscribe to specific event type
        template<typename T>
        SubscriptionId subscribe(std::function<void(const T&)> callback) {
            static_assert(std::is_base_of<Event, T>::value, "T must derive from Event");

            auto wrapper = [callback](const Event& e) {
                callback(static_cast<const T&>(e));
                };

            std::lock_guard<std::mutex> lock(mutex_);
            SubscriptionId id = nextId_++;
            subscribers_[std::type_index(typeid(T))].push_back({ id, wrapper });
            return id;
        }

        // Subscribe to all events
        SubscriptionId subscribeAll(EventCallback callback) {
            std::lock_guard<std::mutex> lock(mutex_);
            SubscriptionId id = nextId_++;
            globalSubscribers_.push_back({ id, callback });
            return id;
        }

        // Unsubscribe from events
        void unsubscribe(SubscriptionId id) {
            std::lock_guard<std::mutex> lock(mutex_);

            for (auto& [type, subs] : subscribers_) {
                subs.erase(
                    std::remove_if(subs.begin(), subs.end(),
                        [id](const auto& p) { return p.first == id; }),
                    subs.end()
                );
            }

            globalSubscribers_.erase(
                std::remove_if(globalSubscribers_.begin(), globalSubscribers_.end(),
                    [id](const auto& p) { return p.first == id; }),
                globalSubscribers_.end()
            );
        }

        // Publish event immediately
        template<typename T>
        void publish(const T& event) {
            static_assert(std::is_base_of<Event, T>::value, "T must derive from Event");

            std::vector<std::pair<SubscriptionId, EventCallback>> callbacks;

            {
                std::lock_guard<std::mutex> lock(mutex_);

                auto it = subscribers_.find(std::type_index(typeid(T)));
                if (it != subscribers_.end()) {
                    callbacks = it->second;
                }

                for (auto& sub : globalSubscribers_) {
                    callbacks.push_back(sub);
                }
            }

            // Call outside lock to prevent deadlocks
            for (auto& [id, callback] : callbacks) {
                callback(event);
            }
        }

        // Queue event for later processing (thread-safe)
        template<typename T>
        void queue(const T& event) {
            static_assert(std::is_base_of<Event, T>::value, "T must derive from Event");

            std::lock_guard<std::mutex> lock(queueMutex_);
            eventQueue_.push(std::make_unique<T>(event));
        }

        // Process queued events (call from main thread)
        void processQueue() {
            std::queue<std::unique_ptr<Event>> toProcess;

            {
                std::lock_guard<std::mutex> lock(queueMutex_);
                std::swap(toProcess, eventQueue_);
            }

            while (!toProcess.empty()) {
                auto& event = toProcess.front();

                std::vector<std::pair<SubscriptionId, EventCallback>> callbacks;
                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    for (auto& sub : globalSubscribers_) {
                        callbacks.push_back(sub);
                    }
                }

                for (auto& [id, callback] : callbacks) {
                    callback(*event);
                }

                toProcess.pop();
            }
        }

        // Singleton access
        static EventBus& instance() {
            static EventBus bus;
            return bus;
        }

    private:
        EventBus() = default;

        std::mutex mutex_;
        std::mutex queueMutex_;

        std::unordered_map<std::type_index,
            std::vector<std::pair<SubscriptionId, EventCallback>>> subscribers_;
        std::vector<std::pair<SubscriptionId, EventCallback>> globalSubscribers_;
        std::queue<std::unique_ptr<Event>> eventQueue_;

        SubscriptionId nextId_ = 1;
    };

    // Convenience macros
#define LIBRE_SUBSCRIBE(EventType, callback) \
    libre::EventBus::instance().subscribe<EventType>(callback)

#define LIBRE_PUBLISH(event) \
    libre::EventBus::instance().publish(event)

#define LIBRE_QUEUE(event) \
    libre::EventBus::instance().queue(event)

} // namespace libre