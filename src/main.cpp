#include <iostream>
#include "Window.hpp"
#include "VulkanContext.hpp"
#include "FontRenderer.hpp"

int main() {
    try {
        Window window(800, 600, "Vulkan Hello World with TrueType Font");
        if (!window.getGlfwWindow()) {
            std::cerr << "Failed to create window\n";
            return -1;
        }

        VulkanContext vkCtx;
        if (!vkCtx.initialize(window.getGlfwWindow())) {
            std::cerr << "Failed to initialize Vulkan\n";
            return -1;
        }

        FontRenderer fontRenderer(vkCtx.device, vkCtx.physicalDevice, vkCtx.commandPool, vkCtx.graphicsQueue);
        if (!fontRenderer.loadFont("fonts/Roboto-Regular.ttf", 72)) {
            std::cerr << "Failed to load font, trying system fallback\n";
        }

        VkImageView textTextureView = fontRenderer.createTextTexture(vkCtx, "Hello World!");
        if (textTextureView == VK_NULL_HANDLE) {
            std::cerr << "Failed to create text texture\n";
            return -1;
        }

        // Set text dimensions for proper aspect ratio scaling
        vkCtx.setTextDimensions(fontRenderer.getTextWidth(), fontRenderer.getTextHeight());

        vkCtx.allocateDescriptorSet(textTextureView);

        while (!window.shouldClose()) {
            window.pollEvents();
            vkCtx.drawFrame();
        }

        vkDeviceWaitIdle(vkCtx.device);
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return -1;
    }
    return 0;
}
