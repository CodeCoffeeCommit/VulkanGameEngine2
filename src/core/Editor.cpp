#include "Editor.h"
#include <iostream>

namespace libre {

    Editor::Editor() {}

    Editor::~Editor() {
        shutdown();
    }

    void Editor::initialize() {
        std::cout << "[Editor] Initializing..." << std::endl;

        world_ = std::make_unique<World>();
        commandHistory_ = std::make_unique<CommandHistory>(100);
        commandQueue_ = std::make_unique<CommandQueue>();

        setupEventHandlers();

        std::cout << "[Editor] Initialized" << std::endl;
    }

    void Editor::shutdown() {
        for (auto id : subscriptions_) {
            EventBus::instance().unsubscribe(id);
        }
        subscriptions_.clear();

        commandQueue_.reset();
        commandHistory_.reset();
        world_.reset();
    }

    void Editor::setupEventHandlers() {
        auto sub = EventBus::instance().subscribe<ComponentModifiedEvent>(
            [this](const ComponentModifiedEvent&) {
                markSceneModified();
            }
        );
        subscriptions_.push_back(sub);
    }

    void Editor::update(float deltaTime) {
        commandQueue_->process(*world_, *commandHistory_);
        EventBus::instance().processQueue();
    }

    void Editor::executeCommand(std::unique_ptr<Command> cmd) {
        commandHistory_->execute(std::move(cmd), *world_);
        markSceneModified();
    }

    void Editor::queueCommand(std::unique_ptr<Command> cmd) {
        commandQueue_->submit(std::move(cmd));
    }

    void Editor::undo() {
        if (commandHistory_->undo(*world_)) {
            UndoEvent event;
            event.commandName = commandHistory_->getRedoName();
            EventBus::instance().publish(event);
        }
    }

    void Editor::redo() {
        if (commandHistory_->redo(*world_)) {
            RedoEvent event;
            event.commandName = commandHistory_->getUndoName();
            EventBus::instance().publish(event);
        }
    }

    bool Editor::canUndo() const { return commandHistory_->canUndo(); }
    bool Editor::canRedo() const { return commandHistory_->canRedo(); }
    std::string Editor::getUndoDescription() const { return commandHistory_->getUndoName(); }
    std::string Editor::getRedoDescription() const { return commandHistory_->getRedoName(); }

    // Selection
    void Editor::select(EntityID entity, bool addToSelection) {
        if (!addToSelection) world_->clearSelection();
        world_->select(entity);

        SelectionChangedEvent event;
        event.selectedEntities = world_->getSelection();
        event.activeEntity = world_->getActiveEntity();
        EventBus::instance().publish(event);
    }

    void Editor::deselect(EntityID entity) {
        world_->deselect(entity);

        SelectionChangedEvent event;
        event.selectedEntities = world_->getSelection();
        event.activeEntity = world_->getActiveEntity();
        EventBus::instance().publish(event);
    }

    void Editor::selectAll() {
        for (auto& handle : world_->getAllEntities()) {
            world_->select(handle.getID());
        }

        SelectionChangedEvent event;
        event.selectedEntities = world_->getSelection();
        event.activeEntity = world_->getActiveEntity();
        EventBus::instance().publish(event);
    }

    void Editor::deselectAll() {
        world_->clearSelection();

        SelectionChangedEvent event;
        EventBus::instance().publish(event);
    }

    void Editor::invertSelection() {
        std::vector<EntityID> newSelection;
        for (auto& handle : world_->getAllEntities()) {
            if (!world_->isSelected(handle.getID())) {
                newSelection.push_back(handle.getID());
            }
        }
        world_->setSelection(newSelection);

        SelectionChangedEvent event;
        event.selectedEntities = newSelection;
        event.activeEntity = world_->getActiveEntity();
        EventBus::instance().publish(event);
    }

    const std::vector<EntityID>& Editor::getSelection() const {
        return world_->getSelection();
    }

    EntityID Editor::getActiveEntity() const {
        return world_->getActiveEntity();
    }

    bool Editor::isSelected(EntityID entity) const {
        return world_->isSelected(entity);
    }

    // Entity operations
    EntityID Editor::createEntity(const std::string& name, const std::string& type) {
        auto cmd = std::make_unique<CreateEntityCommand>(name, type);
        CreateEntityCommand* cmdPtr = cmd.get();
        executeCommand(std::move(cmd));

        EntityID id = cmdPtr->getCreatedEntity();

        EntityCreatedEvent event;
        event.entityId = id;
        event.name = name;
        EventBus::instance().publish(event);

        return id;
    }

    void Editor::deleteEntity(EntityID entity) {
        executeCommand(std::make_unique<DeleteEntityCommand>(entity));

        EntityDestroyedEvent event;
        event.entityId = entity;
        EventBus::instance().publish(event);
    }

    void Editor::deleteSelected() {
        auto selection = world_->getSelection();

        auto compound = std::make_unique<CompoundCommand>("Delete Selected");
        for (EntityID id : selection) {
            compound->addCommand(std::make_unique<DeleteEntityCommand>(id));
        }
        executeCommand(std::move(compound));
    }

    void Editor::duplicateSelected() {
        std::cout << "[Editor] Duplicate not yet implemented" << std::endl;
    }

    void Editor::setEntityName(EntityID entity, const std::string& name) {
        std::string oldName;
        if (auto* meta = world_->getMetadata(entity)) {
            oldName = meta->name;
        }

        executeCommand(std::make_unique<RenameEntityCommand>(entity, name));

        EntityRenamedEvent event;
        event.entityId = entity;
        event.oldName = oldName;
        event.newName = name;
        EventBus::instance().publish(event);
    }

    void Editor::setEntityParent(EntityID child, EntityID parent) {
        executeCommand(std::make_unique<SetParentCommand>(child, parent));
    }

    // Scene operations
    void Editor::newScene() {
        world_->clear();
        commandHistory_->clear();
        scenePath_.clear();
        sceneModified_ = false;

        SceneClearedEvent event;
        EventBus::instance().publish(event);
    }

    bool Editor::loadScene(const std::string& path) {
        std::cout << "[Editor] Load scene not yet implemented: " << path << std::endl;
        scenePath_ = path;
        sceneModified_ = false;

        SceneLoadedEvent event;
        event.filePath = path;
        EventBus::instance().publish(event);
        return true;
    }

    bool Editor::saveScene(const std::string& path) {
        std::cout << "[Editor] Save scene not yet implemented: " << path << std::endl;
        scenePath_ = path;
        sceneModified_ = false;

        SceneSavedEvent event;
        event.filePath = path;
        EventBus::instance().publish(event);
        return true;
    }

    void Editor::setTool(Tool tool) {
        Tool prev = currentTool_;
        currentTool_ = tool;

        ToolChangedEvent event;
        const char* names[] = { "Select", "Move", "Rotate", "Scale" };
        event.previousTool = names[static_cast<int>(prev)];
        event.newTool = names[static_cast<int>(tool)];
        EventBus::instance().publish(event);
    }

    void Editor::markSceneModified() {
        sceneModified_ = true;
    }

} // namespace libre