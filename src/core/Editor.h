#pragma once

#include "Event.h"
#include "Command.h"
#include "../world/World.h"
#include <memory>
#include <string>

namespace libre {

    // ============================================================================
    // EDITOR - Central coordinator
    // ============================================================================

    class Editor {
    public:
        Editor();
        ~Editor();

        void initialize();
        void shutdown();
        void update(float deltaTime);

        // World access
        World& getWorld() { return *world_; }
        const World& getWorld() const { return *world_; }

        // Command execution
        void executeCommand(std::unique_ptr<Command> cmd);
        void queueCommand(std::unique_ptr<Command> cmd);

        // Undo/Redo
        void undo();
        void redo();
        bool canUndo() const;
        bool canRedo() const;
        std::string getUndoDescription() const;
        std::string getRedoDescription() const;

        // Selection helpers
        void select(EntityID entity, bool addToSelection = false);
        void deselect(EntityID entity);
        void selectAll();
        void deselectAll();
        void invertSelection();

        const std::vector<EntityID>& getSelection() const;
        EntityID getActiveEntity() const;
        bool isSelected(EntityID entity) const;

        // Entity operations (create commands internally)
        EntityID createEntity(const std::string& name, const std::string& type = "");
        void deleteEntity(EntityID entity);
        void deleteSelected();
        void duplicateSelected();

        void setEntityName(EntityID entity, const std::string& name);
        void setEntityParent(EntityID child, EntityID parent);

        // Scene operations
        void newScene();
        bool loadScene(const std::string& path);
        bool saveScene(const std::string& path);

        const std::string& getCurrentScenePath() const { return scenePath_; }
        bool isSceneModified() const { return sceneModified_; }

        // Tool management
        enum class Tool { Select, Move, Rotate, Scale };
        void setTool(Tool tool);
        Tool getCurrentTool() const { return currentTool_; }

        // Event bus
        EventBus& getEventBus() { return EventBus::instance(); }

        // Singleton
        static Editor& instance() {
            static Editor editor;
            return editor;
        }

    private:
        void setupEventHandlers();
        void markSceneModified();

        std::unique_ptr<World> world_;
        std::unique_ptr<CommandHistory> commandHistory_;
        std::unique_ptr<CommandQueue> commandQueue_;

        std::string scenePath_;
        bool sceneModified_ = false;
        Tool currentTool_ = Tool::Select;

        std::vector<EventBus::SubscriptionId> subscriptions_;
    };

} // namespace libre