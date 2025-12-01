#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <memory>
#include <string>
#include "../VulkanInstance/VulkanInstance.h"
#include "../VulkanDevice/VulkanDevice.h"
#include "../VulkanMemoryAllocator/VulkanMemoryAllocator.h"
#include "../VulkanCommandBuffer/VulkanCommandBuffer.h"
#include "../../DebugOutput/DubugOutput.h"

struct ImageOptions 
{
    uint32_t width = 0;
    uint32_t height = 0;
    VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
    uint32_t mipLevels = 1;
    VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
        VK_IMAGE_USAGE_SAMPLED_BIT;
};

struct ImageCreateInfo 
{
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t depth = 1;
    VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
    VkImageType imageType = VK_IMAGE_TYPE_2D;
    VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
        VK_IMAGE_USAGE_SAMPLED_BIT;
    VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
    uint32_t mipLevels = 1;
    uint32_t arrayLayers = 1;
    VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
    VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    static ImageCreateInfo FromOptions(const ImageOptions& opts) 
    {
        ImageCreateInfo info = {};
        info.width = opts.width;
        info.height = opts.height;
        info.depth = 1;
        info.format = opts.format;
        info.imageType = VK_IMAGE_TYPE_2D;
        info.usage = opts.usage;
        info.tiling = VK_IMAGE_TILING_OPTIMAL;
        info.mipLevels = opts.mipLevels;
        info.arrayLayers = 1;
        info.samples = VK_SAMPLE_COUNT_1_BIT;
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        return info;
    }

    static ImageCreateInfo Texture2D(uint32_t w, uint32_t h, VkFormat fmt = VK_FORMAT_R8G8B8A8_SRGB) 
    {
        ImageCreateInfo info = {};
        info.width = w;
        info.height = h;
        info.depth = 1;
        info.format = fmt;
        info.imageType = VK_IMAGE_TYPE_2D;
        info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        info.tiling = VK_IMAGE_TILING_OPTIMAL;
        info.mipLevels = 1;
        info.arrayLayers = 1;
        info.samples = VK_SAMPLE_COUNT_1_BIT;
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        return info;
    }

    VkImageCreateInfo ToVulkan() const 
    {
        VkImageCreateInfo vkInfo = {};
        vkInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        vkInfo.imageType = imageType;
        vkInfo.extent.width = width;
        vkInfo.extent.height = height;
        vkInfo.extent.depth = depth;
        vkInfo.mipLevels = mipLevels;
        vkInfo.arrayLayers = arrayLayers;
        vkInfo.format = format;
        vkInfo.tiling = tiling;
        vkInfo.initialLayout = initialLayout;
        vkInfo.usage = usage;
        vkInfo.samples = samples;
        vkInfo.sharingMode = sharingMode;
        vkInfo.queueFamilyIndexCount = 0;
        vkInfo.pQueueFamilyIndices = nullptr;
        return vkInfo;
    }
};



class VulkanImage
{
public:
    VulkanImage();
    ~VulkanImage();

    VulkanImage(const VulkanImage&) = delete;
    VulkanImage& operator=(const VulkanImage&) = delete;
    VulkanImage(VulkanImage&& other) noexcept;
    VulkanImage& operator=(VulkanImage&& other) noexcept;

    bool Initialize(
        std::shared_ptr<VulkanInstance> instance,
        std::shared_ptr<VulkanDevice> device,
        std::shared_ptr<VulkanMemoryAllocator> allocator
    );
    void Cleanup();
    bool IsInitialized() const { return m_instance && m_device && m_allocator; }

    bool CreateImage(const ImageOptions& options, AllocatedImage& outImage);

    bool CreateImage(const ImageCreateInfo& createInfo, AllocatedImage& outImage);

    void DestroyImage(AllocatedImage& image);

    bool TransitionLayout(
        VkCommandBuffer cmd,
        AllocatedImage& image,
        VkImageLayout newLayout,
        VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT
    );
    
    bool UploadData(
        VulkanCommandBuffer* commandBuffer,
        AllocatedImage& image,
        const void* data,
        size_t dataSize,
        bool transitionToShaderOptimal = true
    ); 

    std::shared_ptr<VulkanDevice> GetDevice() const { return m_device; }
    std::shared_ptr<VulkanMemoryAllocator> GetAllocator() const { return m_allocator; }

private:
    std::shared_ptr<VulkanInstance> m_instance;
    std::shared_ptr<VulkanDevice> m_device;
    std::shared_ptr<VulkanMemoryAllocator> m_allocator;

    static const Debug::DebugOutput DebugOut;

    bool ValidateDependencies() const;
    bool ValidateImageCreateInfo(const ImageCreateInfo& createInfo) const;

    VkAccessFlags GetAccessMask(VkImageLayout layout) const; 

    void ReportError(const std::string& message) const {
        DebugOut.outputDebug("VulkanImage Error: " + message);
    }

    void ReportWarning(const std::string& message) const {
        DebugOut.outputDebug("VulkanImage Warning: " + message);
    }
};