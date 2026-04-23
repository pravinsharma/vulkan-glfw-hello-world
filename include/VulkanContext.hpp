#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <string>

struct VulkanContext {
    void* glfwWindow = nullptr;  // GLFW window handle for swapchain recreation

    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    std::vector<VkImage> swapchainImages;
    VkFormat swapchainImageFormat;
    VkExtent2D swapchainExtent;
    std::vector<VkImageView> swapchainImageViews;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline graphicsPipeline = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> swapchainFramebuffers;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
     VkSampler sampler = VK_NULL_HANDLE;
     VkBuffer vertexBuffer = VK_NULL_HANDLE;
     VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;

     // Text dimensions for proper scaling
     uint32_t m_textWidth = 0;
     uint32_t m_textHeight = 0;

     // Per-frame synchronization (one per swapchain image)
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    uint32_t currentFrame = 0;

     ~VulkanContext();
     bool initialize(void* glfwWindow);
     void cleanup();

     void recreateSwapchain(void* glfwWindow);
     void drawFrame();

     void setTextDimensions(uint32_t width, uint32_t height) {
         m_textWidth = width;
         m_textHeight = height;
     }

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                      VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
                     VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
                     VkImage& image, VkDeviceMemory& imageMemory);
    void generateMipmaps(VkImage image, VkFormat format, int32_t texWidth, int32_t texHeight);
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

    void createDescriptorSetLayout();
    void createSampler();
    void createDescriptorPool();
    void allocateDescriptorSet(VkImageView textureView);
    void createVertexBuffer();
    void beginSingleTimeCommands(VkCommandBuffer& commandBuffer);
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
};
