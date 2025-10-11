#pragma once

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include <string>
#include "../VulkanInstance/VulkanInstance.h"
#include "../VulkanDevice/VulkanDevice.h"
#include "../VulkanRenderPass/VulkanRenderPass.h"
#include "../../DebugOutput/DubugOutput.h"

class VulkanFrameBuffer
{
public:
    VulkanFrameBuffer();
    ~VulkanFrameBuffer();

    // RAII
    VulkanFrameBuffer(const VulkanFrameBuffer&) = delete;
    VulkanFrameBuffer& operator=(const VulkanFrameBuffer&) = delete;
    VulkanFrameBuffer(VulkanFrameBuffer&& other) noexcept;
    VulkanFrameBuffer& operator=(VulkanFrameBuffer&& other) noexcept;

    // Core lifecycle 
    bool Initialize(std::shared_ptr<VulkanInstance> instance,
        std::shared_ptr<VulkanDevice> device,
        std::shared_ptr<VulkanRenderPass> renderPass,
        const std::vector<VkImageView>& attachments,
        uint32_t width,
        uint32_t height);
    void Cleanup();
    bool IsInitialized() const { return m_framebuffer != VK_NULL_HANDLE; }

    // Getters
    VkFramebuffer GetFramebuffer() const { return m_framebuffer; }
    uint32_t GetWidth() const { return m_width; }
    uint32_t GetHeight() const { return m_height; }

    // Debug
    std::string GetFramebufferInfo() const;

private:
    std::shared_ptr<VulkanInstance> m_instance;
    std::shared_ptr<VulkanDevice> m_device;
    std::shared_ptr<VulkanRenderPass> m_renderPass;
    VkFramebuffer m_framebuffer = VK_NULL_HANDLE;
    uint32_t m_width = 0;
    uint32_t m_height = 0;

    static const Debug::DebugOutput DebugOut;

    bool ValidateDependencies() const;

    void ReportError(const std::string& message) const
    {
        DebugOut.outputDebug("VulkanFrameBuffer Error: " + message);
    }

    void ReportWarning(const std::string& message) const
    {
        DebugOut.outputDebug("VulkanFrameBuffer Warning: " + message);
    }
};