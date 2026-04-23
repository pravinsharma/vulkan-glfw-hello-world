#include "FontRenderer.hpp"
#include "VulkanContext.hpp"
#include <stdexcept>
#include <algorithm>
#include <cstring>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <string>

FontRenderer::FontRenderer(VkDevice device, VkPhysicalDevice physicalDevice,
                          VkCommandPool commandPool, VkQueue graphicsQueue)
    : m_device(device), m_physicalDevice(physicalDevice), m_commandPool(commandPool), m_graphicsQueue(graphicsQueue) {
    m_glyphs.resize(128);
}

FontRenderer::~FontRenderer() {
    cleanup();
}

bool FontRenderer::loadFont(const std::string& fontPath, uint32_t fontSize) {
    if (FT_Init_FreeType(&m_freetype)) {
        std::cerr << "Could not init FreeType library\n";
        return false;
    }

    // Try the provided font path, fallback to system fonts
    std::vector<std::string> fallbackPaths;
#ifdef _WIN32
    fallbackPaths = {
        "C:/Windows/Fonts/arial.ttf",
        "C:/Windows/Fonts/segoeui.ttf",
        "C:/Windows/Fonts/calibri.ttf"
    };
#elif defined(__APPLE__)
    fallbackPaths = {
        "/System/Library/Fonts/Helvetica.ttc",
        "/System/Library/Fonts/Arial.ttf"
    };
#else
    fallbackPaths = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/truetype/freefont/FreeSans.ttf"
    };
#endif

    std::string pathToUse = fontPath;
    if (!std::ifstream(pathToUse).good()) {
        bool found = false;
        for (const auto& fallback : fallbackPaths) {
            if (std::ifstream(fallback).good()) {
                pathToUse = fallback;
                found = true;
                break;
            }
        }
        if (!found) {
            std::cerr << "No usable font found. Tried " << fontPath << " and system fallbacks.\n";
            return false;
        }
    }

    if (FT_New_Face(m_freetype, pathToUse.c_str(), 0, &m_face)) {
        std::cerr << "Failed to load font: " << pathToUse << "\n";
        return false;
    }

    FT_Set_Pixel_Sizes(m_face, 0, fontSize);

    // Load glyph metrics for ASCII 0-127
    for (int c = 0; c < 128; c++) {
        if (FT_Load_Char(m_face, static_cast<unsigned char>(c), FT_LOAD_RENDER)) {
            continue;
        }
        FT_GlyphSlot slot = m_face->glyph;
        GlyphMetrics metrics;
        metrics.width = slot->bitmap.width;
        metrics.height = slot->bitmap.rows;
        metrics.bearingX = slot->bitmap_left;
        metrics.bearingY = slot->bitmap_top;
        metrics.advance = slot->advance.x;
        m_glyphs[c] = metrics;
    }

    return true;
}

VkImageView FontRenderer::createTextTexture(VulkanContext& ctx, const std::string& text) {
    if (!m_face) {
        throw std::runtime_error("Font not loaded");
    }

    // Compute layout
    int totalWidth = 0;
    int penX = 0;
    int minX = 0, maxX = 0;
    std::vector<std::pair<char, int>> positions;
    positions.reserve(text.size());

    // First pass: compute bounding box
    for (unsigned char c : text) {
        const GlyphMetrics& gm = m_glyphs[c];
        int xOffset = penX + gm.bearingX;
        minX = std::min(minX, xOffset);
        maxX = std::max(maxX, xOffset + gm.width);
        positions.push_back({c, xOffset});
        int advance = (gm.advance + 32) >> 6;
        penX += advance;
    }
    totalWidth = maxX - minX;

    int baselineY = 0;
    for (const auto& p : positions) {
        const GlyphMetrics& gm = m_glyphs[p.first];
        baselineY = std::max(baselineY, gm.bearingY);
    }
    int maxDescent = 0;
    for (const auto& p : positions) {
        const GlyphMetrics& gm = m_glyphs[p.first];
        maxDescent = std::max(maxDescent, gm.height - gm.bearingY);
    }
    int totalHeight = baselineY + maxDescent;

    if (totalWidth <= 0 || totalHeight <= 0) {
        return VK_NULL_HANDLE;
    }

    // Store dimensions for aspect ratio correction
    m_textWidth = static_cast<uint32_t>(totalWidth);
    m_textHeight = static_cast<uint32_t>(totalHeight);

    // Allocate pixel buffer (8-bit alpha)
    std::vector<uint8_t> pixels(totalWidth * totalHeight, 0);

    // Second pass: render each glyph into the pixel buffer
    for (const auto& p : positions) {
        unsigned char c = p.first;
        const GlyphMetrics& gm = m_glyphs[c];
        if (gm.width == 0 || gm.height == 0) continue; // space or empty

        if (FT_Load_Char(m_face, c, FT_LOAD_RENDER)) {
            continue;
        }
        FT_GlyphSlot slot = m_face->glyph;
        uint8_t* src = slot->bitmap.buffer;
        int x = p.second - minX;
        int y = baselineY - gm.bearingY;

        // Copy each row
        for (int row = 0; row < gm.height; row++) {
            uint8_t* dstRow = &pixels[(y + row) * totalWidth + x];
            uint8_t* srcRow = src + row * slot->bitmap.pitch;
            memcpy(dstRow, srcRow, gm.width);
        }
    }

    // Create Vulkan image
    VkImage image;
    VkDeviceMemory imageMemory;
    ctx.createImage(totalWidth, totalHeight, VK_FORMAT_R8_UNORM, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, imageMemory);

    // Create staging buffer
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;
    VkDeviceSize imageSize = totalWidth * totalHeight;
    ctx.createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer, stagingMemory);

    // Copy pixel data to staging buffer
    void* data;
    vkMapMemory(ctx.device, stagingMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels.data(), imageSize);
    vkUnmapMemory(ctx.device, stagingMemory);

    // Transfer to image using single-time commands
    VkCommandBuffer cmd;
    ctx.beginSingleTimeCommands(cmd);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(cmd,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
        0, nullptr, 0, nullptr, 1, &barrier);

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {static_cast<uint32_t>(totalWidth), static_cast<uint32_t>(totalHeight), 1};

    vkCmdCopyBufferToImage(cmd, stagingBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    // Transition for shader read
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(cmd,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
        0, nullptr, 0, nullptr, 1, &barrier);

    ctx.endSingleTimeCommands(cmd);

    // Cleanup staging resources
    vkDestroyBuffer(ctx.device, stagingBuffer, nullptr);
    vkFreeMemory(ctx.device, stagingMemory, nullptr);

    // Create image view
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8_UNORM;
    viewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
    viewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
    viewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
    viewInfo.components.a = VK_COMPONENT_SWIZZLE_A;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(ctx.device, &viewInfo, nullptr, &m_textImageView) != VK_SUCCESS) {
        vkDestroyImage(ctx.device, image, nullptr);
        vkFreeMemory(ctx.device, imageMemory, nullptr);
        throw std::runtime_error("Failed to create texture image view");
    }

    m_textImage = image;
    m_textImageMemory = imageMemory;

    return m_textImageView;
}

void FontRenderer::cleanup() {
    if (m_device != VK_NULL_HANDLE) {
        if (m_textImageView != VK_NULL_HANDLE) vkDestroyImageView(m_device, m_textImageView, nullptr);
        if (m_textImage != VK_NULL_HANDLE) vkDestroyImage(m_device, m_textImage, nullptr);
        if (m_textImageMemory != VK_NULL_HANDLE) vkFreeMemory(m_device, m_textImageMemory, nullptr);
    }
    if (m_freetype) {
        if (m_face) FT_Done_Face(m_face);
        FT_Done_FreeType(m_freetype);
    }
}
