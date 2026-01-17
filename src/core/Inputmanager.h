#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <unordered_map>

class Window;
struct CallbackData;

class InputManager {
public:
    InputManager(Window* window);
    ~InputManager();

    // Update input state
    void update();

    // Keyboard input
    bool isKeyPressed(int key) const;
    bool isKeyJustPressed(int key) const;
    bool isKeyJustReleased(int key) const;

    // Mouse input
    bool isMouseButtonPressed(int button) const;
    bool isMouseButtonJustPressed(int button) const;
    bool isMouseButtonJustReleased(int button) const;

    // Mouse position
    double getMouseX() const { return mouseX; }
    double getMouseY() const { return mouseY; }
    double getMouseDeltaX() const { return mouseDeltaX; }
    double getMouseDeltaY() const { return mouseDeltaY; }

    // Mouse scroll
    double getScrollX() const { return scrollX; }
    double getScrollY() const { return scrollY; }

    // Callbacks (static for GLFW)
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);

private:
    Window* window;
    GLFWwindow* glfwWindow;

    // Keyboard state
    std::unordered_map<int, bool> keysPressed;
    std::unordered_map<int, bool> keysJustPressed;
    std::unordered_map<int, bool> keysJustReleased;

    // Mouse button state
    std::unordered_map<int, bool> mouseButtonsPressed;
    std::unordered_map<int, bool> mouseButtonsJustPressed;
    std::unordered_map<int, bool> mouseButtonsJustReleased;

    // Mouse position
    double mouseX = 0.0;
    double mouseY = 0.0;
    double lastMouseX = 0.0;
    double lastMouseY = 0.0;
    double mouseDeltaX = 0.0;
    double mouseDeltaY = 0.0;

    // Mouse scroll
    double scrollX = 0.0;
    double scrollY = 0.0;

    // First mouse movement flag
    bool firstMouse = true;
};