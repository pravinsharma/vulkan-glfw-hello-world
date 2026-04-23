#pragma once

#include <string>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

class Window {
public:
    Window(int width = 800, int height = 600, const std::string& title = "Vulkan Hello World");
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool shouldClose() const { return glfwWindowShouldClose(m_window) != 0; }
    void pollEvents() { glfwPollEvents(); }
    void* getGlfwWindow() const { return m_window; }
    VkSurfaceKHR createVulkanSurface(VkInstance instance) const;

private:
    GLFWwindow* m_window = nullptr;
};

