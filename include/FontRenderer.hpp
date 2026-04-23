#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include <ft2build.h>
#include FT_FREETYPE_H

struct VulkanContext; // forward declaration

struct GlyphMetrics {
    int width = 0;
    int height = 0;
    int bearingX = 0;
    int bearingY = 0;
    uint32_t advance = 0; // in 26.6 fixed point
};

class FontRenderer {
public:
    FontRenderer(VkDevice device, VkPhysicalDevice physicalDevice,
                 VkCommandPool commandPool, VkQueue graphicsQueue);
    ~FontRenderer();

     bool loadFont(const std::string& fontPath, uint32_t fontSize);
     VkImageView createTextTexture(VulkanContext& ctx, const std::string& text);
     void cleanup();

     uint32_t getTextWidth() const { return m_textWidth; }
     uint32_t getTextHeight() const { return m_textHeight; }

 private:
     FT_Library m_freetype = nullptr;
     FT_Face m_face = nullptr;
     std::vector<GlyphMetrics> m_glyphs; // size 128, indexed by char
     uint32_t m_textWidth = 0;
     uint32_t m_textHeight = 0;

    VkDevice m_device;
    VkPhysicalDevice m_physicalDevice;
    VkCommandPool m_commandPool;
    VkQueue m_graphicsQueue;

    VkImage m_textImage = VK_NULL_HANDLE;
    VkDeviceMemory m_textImageMemory = VK_NULL_HANDLE;
    VkImageView m_textImageView = VK_NULL_HANDLE;
};
