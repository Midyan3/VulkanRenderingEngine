#pragma once
#include <vulkan/vulkan.h>
#include <memory>
#include <string>
#include "../VulkanImage/VulkanImage.h"
#include "../VulkanImageView/VulkanImageView.h"
#include "../VulkanDevice/VulkanDevice.h"
#include "../VulkanCommandBuffer/VulkanCommandBuffer.h"
#include "../../DebugOutput/DubugOutput.h"

struct TextureInfo {
    AllocatedImage image;
    AllocatedImageView imageView;
    VkSampler sampler = VK_NULL_HANDLE;

    bool IsValid() const {
        return image.IsValid() &&
            imageView.IsValid() &&
            sampler != VK_NULL_HANDLE;
    }
};

struct SamplerOptions {
    VkFilter magFilter = VK_FILTER_LINEAR;
    VkFilter minFilter = VK_FILTER_LINEAR;
    VkSamplerMipmapMode mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    VkSamplerAddressMode addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    VkSamplerAddressMode addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    VkSamplerAddressMode addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    float mipLodBias = 0.0f;
    VkBool32 anisotropyEnable = VK_TRUE;
    float maxAnisotropy = 16.0f;
    VkBool32 compareEnable = VK_FALSE;
    VkCompareOp compareOp = VK_COMPARE_OP_ALWAYS;
    float minLod = 0.0f;
    float maxLod = VK_LOD_CLAMP_NONE;
    VkBorderColor borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    VkBool32 unnormalizedCoordinates = VK_FALSE;

    static SamplerOptions DefaultLinear() {
        return SamplerOptions{};
    }

    static SamplerOptions DefaultNearest() {
        SamplerOptions opts;
        opts.magFilter = VK_FILTER_NEAREST;
        opts.minFilter = VK_FILTER_NEAREST;
        opts.anisotropyEnable = VK_FALSE;
        return opts;
    }

    static SamplerOptions Clamp() {
        SamplerOptions opts;
        opts.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        opts.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        opts.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        return opts;
    }

    VkSamplerCreateInfo ToVulkan() const;
};

class Texture 
{
public:
    Texture();
    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    bool Initialize(
        VulkanImage* imageManager,
        VulkanImageView* viewManager,
        VulkanDevice* device);

    bool IsInitialized() const;

    bool LoadFromFile(
        const std::string& filepath,
        VulkanCommandBuffer* cmdBuffer,
        const SamplerOptions& samplerOpts = SamplerOptions::DefaultLinear());

    void Destroy();
    bool IsValid() const { return m_info.IsValid(); }

    VkImageView GetImageView() const { return m_info.imageView.view; }
    VkSampler GetSampler() const { return m_info.sampler; }
    const TextureInfo& GetInfo() const { return m_info; }

private:
    TextureInfo m_info;

    VulkanImage* m_imageManager = nullptr;
    VulkanImageView* m_viewManager = nullptr;
    VulkanDevice* m_device = nullptr;

    static const Debug::DebugOutput DebugOut;

    bool CreateSampler(const SamplerOptions& options);

    void ReportError(const std::string& message) const {
        DebugOut.outputDebug("Texture Error: " + message);
    }
};