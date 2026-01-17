#pragma once

#include "../world/World.h"
#include <string>
#include <memory>
#include <vector>
#include <deque>
#include <queue>
#include <mutex>

namespace libre {

    // ============================================================================
    // COMMAND BASE CLASS
    // ============================================================================

    class Command {
    public:
        virtual ~Command() = default;

        virtual void execute(World& world) = 0;
        virtual void undo(World& world) = 0;

        virtual bool canMergeWith(const Command& other) const { return false; }
        virtual void mergeWith(const Command& other) {}

        virtual std::string getName() const = 0;
        virtual std::string getDescription() const { return getName(); }
        virtual bool isUndoable() const { return true; }
    };

    // ============================================================================
    // CREATE ENTITY COMMAND
    // ============================================================================

    class CreateEntityCommand : public Command {
    public:
        CreateEntityCommand(const std::string& name, const std::string& type = "")
            : entityName_(name), entityType_(type) {
        }

        void execute(World& world) override {
            auto handle = world.createEntity(entityName_, entityType_);
            createdId_ = handle.getID();
        }

        void undo(World& world) override {
            world.destroyEntity(createdId_);
            createdId_ = INVALID_ENTITY;
        }

        std::string getName() const override { return "Create Entity"; }
        EntityID getCreatedEntity() const { return createdId_; }

    private:
        std::string entityName_;
        std::string entityType_;
        EntityID createdId_ = INVALID_ENTITY;
    };

    // ============================================================================
    // DELETE ENTITY COMMAND
    // ============================================================================

    class DeleteEntityCommand : public Command {
    public:
        DeleteEntityCommand(EntityID entity) : entityId_(entity) {}

        void execute(World& world) override {
            if (auto* meta = world.getMetadata(entityId_)) {
                savedName_ = meta->name;
                savedType_ = meta->type;
                savedFlags_ = meta->flags;
            }
            savedParent_ = world.getParent(entityId_);

            if (auto* t = world.getComponent<TransformComponent>(entityId_)) {
                savedTransform_ = *t;
                hasTransform_ = true;
            }

            world.destroyEntity(entityId_);
        }

        void undo(World& world) override {
            auto handle = world.createEntity(savedName_, savedType_);
            entityId_ = handle.getID();

            if (auto* meta = world.getMetadata(entityId_)) {
                meta->flags = savedFlags_;
            }

            if (savedParent_ != INVALID_ENTITY) {
                world.setParent(entityId_, savedParent_);
            }

            if (hasTransform_) {
                world.addComponent<TransformComponent>(entityId_, savedTransform_);
            }
        }

        std::string getName() const override { return "Delete Entity"; }

    private:
        EntityID entityId_;
        std::string savedName_;
        std::string savedType_;
        EntityFlags savedFlags_ = EntityFlags::Default;
        EntityID savedParent_ = INVALID_ENTITY;
        TransformComponent savedTransform_;
        bool hasTransform_ = false;
    };

    // ============================================================================
    // TRANSFORM COMMAND
    // ============================================================================

    class TransformCommand : public Command {
    public:
        TransformCommand(EntityID entity,
            const glm::vec3& pos,
            const glm::quat& rot,
            const glm::vec3& scl)
            : entityId_(entity), position_(pos), rotation_(rot), scale_(scl) {
        }

        void execute(World& world) override {
            auto* t = world.getComponent<TransformComponent>(entityId_);
            if (!t) return;

            oldPosition_ = t->position;
            oldRotation_ = t->rotation;
            oldScale_ = t->scale;

            t->position = position_;
            t->rotation = rotation_;
            t->scale = scale_;
            t->dirty = true;
        }

        void undo(World& world) override {
            auto* t = world.getComponent<TransformComponent>(entityId_);
            if (!t) return;

            t->position = oldPosition_;
            t->rotation = oldRotation_;
            t->scale = oldScale_;
            t->dirty = true;
        }

        bool canMergeWith(const Command& other) const override {
            auto* o = dynamic_cast<const TransformCommand*>(&other);
            return o && o->entityId_ == entityId_;
        }

        void mergeWith(const Command& other) override {
            auto* o = dynamic_cast<const TransformCommand*>(&other);
            if (o) {
                position_ = o->position_;
                rotation_ = o->rotation_;
                scale_ = o->scale_;
            }
        }

        std::string getName() const override { return "Transform"; }

    private:
        EntityID entityId_;
        glm::vec3 position_;
        glm::quat rotation_;
        glm::vec3 scale_;
        glm::vec3 oldPosition_;
        glm::quat oldRotation_;
        glm::vec3 oldScale_;
    };

    // ============================================================================
    // SET SELECTION COMMAND
    // ============================================================================

    class SetSelectionCommand : public Command {
    public:
        SetSelectionCommand(const std::vector<EntityID>& selection)
            : selection_(selection) {
        }

        void execute(World& world) override {
            oldSelection_ = world.getSelection();
            world.setSelection(selection_);
        }

        void undo(World& world) override {
            world.setSelection(oldSelection_);
        }

        std::string getName() const override { return "Select"; }
        bool isUndoable() const override { return false; }

    private:
        std::vector<EntityID> selection_;
        std::vector<EntityID> oldSelection_;
    };

    // ============================================================================
    // SET PARENT COMMAND
    // ============================================================================

    class SetParentCommand : public Command {
    public:
        SetParentCommand(EntityID child, EntityID newParent)
            : childId_(child), parentId_(newParent) {
        }

        void execute(World& world) override {
            oldParent_ = world.getParent(childId_);
            world.setParent(childId_, parentId_);
        }

        void undo(World& world) override {
            world.setParent(childId_, oldParent_);
        }

        std::string getName() const override { return "Set Parent"; }

    private:
        EntityID childId_;
        EntityID parentId_;
        EntityID oldParent_ = INVALID_ENTITY;
    };

    // ============================================================================
    // RENAME ENTITY COMMAND
    // ============================================================================

    class RenameEntityCommand : public Command {
    public:
        RenameEntityCommand(EntityID entity, const std::string& newName)
            : entityId_(entity), name_(newName) {
        }

        void execute(World& world) override {
            if (auto* meta = world.getMetadata(entityId_)) {
                oldName_ = meta->name;
                meta->name = name_;
            }
        }

        void undo(World& world) override {
            if (auto* meta = world.getMetadata(entityId_)) {
                meta->name = oldName_;
            }
        }

        std::string getName() const override { return "Rename"; }

    private:
        EntityID entityId_;
        std::string name_;
        std::string oldName_;
    };

    // ============================================================================
    // COMPOUND COMMAND
    // ============================================================================

    class CompoundCommand : public Command {
    public:
        CompoundCommand(const std::string& name) : commandName_(name) {}

        void addCommand(std::unique_ptr<Command> cmd) {
            commands_.push_back(std::move(cmd));
        }

        void execute(World& world) override {
            for (auto& cmd : commands_) {
                cmd->execute(world);
            }
        }

        void undo(World& world) override {
            for (auto it = commands_.rbegin(); it != commands_.rend(); ++it) {
                (*it)->undo(world);
            }
        }

        std::string getName() const override { return commandName_; }

    private:
        std::string commandName_;
        std::vector<std::unique_ptr<Command>> commands_;
    };

    // ============================================================================
    // COMMAND HISTORY
    // ============================================================================

    class CommandHistory {
    public:
        CommandHistory(size_t maxUndoLevels = 100)
            : maxLevels_(maxUndoLevels) {
        }

        void execute(std::unique_ptr<Command> cmd, World& world) {
            cmd->execute(world);

            if (cmd->isUndoable()) {
                if (!undoStack_.empty() && undoStack_.back()->canMergeWith(*cmd)) {
                    undoStack_.back()->mergeWith(*cmd);
                }
                else {
                    undoStack_.push_back(std::move(cmd));
                    while (undoStack_.size() > maxLevels_) {
                        undoStack_.pop_front();
                    }
                }
                redoStack_.clear();
            }
        }

        bool undo(World& world) {
            if (undoStack_.empty()) return false;

            auto cmd = std::move(undoStack_.back());
            undoStack_.pop_back();
            cmd->undo(world);
            redoStack_.push_back(std::move(cmd));
            return true;
        }

        bool redo(World& world) {
            if (redoStack_.empty()) return false;

            auto cmd = std::move(redoStack_.back());
            redoStack_.pop_back();
            cmd->execute(world);
            undoStack_.push_back(std::move(cmd));
            return true;
        }

        bool canUndo() const { return !undoStack_.empty(); }
        bool canRedo() const { return !redoStack_.empty(); }

        std::string getUndoName() const {
            return undoStack_.empty() ? "" : undoStack_.back()->getName();
        }

        std::string getRedoName() const {
            return redoStack_.empty() ? "" : redoStack_.back()->getName();
        }

        void clear() {
            undoStack_.clear();
            redoStack_.clear();
        }

    private:
        size_t maxLevels_;
        std::deque<std::unique_ptr<Command>> undoStack_;
        std::vector<std::unique_ptr<Command>> redoStack_;
    };

    // ============================================================================
    // COMMAND QUEUE
    // ============================================================================

    class CommandQueue {
    public:
        void submit(std::unique_ptr<Command> cmd) {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push(std::move(cmd));
        }

        void process(World& world, CommandHistory& history) {
            std::queue<std::unique_ptr<Command>> toProcess;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                std::swap(toProcess, queue_);
            }

            while (!toProcess.empty()) {
                history.execute(std::move(toProcess.front()), world);
                toProcess.pop();
            }
        }

        bool isEmpty() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return queue_.empty();
        }

    private:
        mutable std::mutex mutex_;
        std::queue<std::unique_ptr<Command>> queue_;
    };

} // namespace libre