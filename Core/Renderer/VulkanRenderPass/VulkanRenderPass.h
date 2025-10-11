#pragma once

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include <string>
#include "../VulkanInstance/VulkanInstance.h"
#include "../VulkanDevice/VulkanDevice.h"
#include "../../DebugOutput/DubugOutput.h"

// Types
struct RenderPassAttachment
{
    VkFormat format = VK_FORMAT_UNDEFINED;
    VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
    VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    VkAttachmentLoadOp stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    VkAttachmentStoreOp stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImageLayout finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    static RenderPassAttachment ColorAttachment(VkFormat format)
    {
        RenderPassAttachment attachment;
        attachment.format = format;
        attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        return attachment;
    }

    static RenderPassAttachment DepthAttachment(VkFormat format)
    {
        RenderPassAttachment attachment;
        attachment.format = format;
        attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        return attachment;
    }
};

// Config
struct SubpassConfig
{
    std::vector<uint32_t> colorAttachments;
    int32_t depthAttachment = -1;
    VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
};

// Config
struct RenderPassConfig
{
    std::vector<RenderPassAttachment> attachments;
    std::vector<SubpassConfig> subpasses;
    VkClearColorValue clearColor = { {0.0f, 0.0f, 0.0f, 1.0f} };
    float clearDepth = 1.0f;
    uint32_t clearStencil = 0;

    static RenderPassConfig SingleColorAttachment(VkFormat colorFormat)
    {
        RenderPassConfig config;
        config.attachments.push_back(RenderPassAttachment::ColorAttachment(colorFormat));

        SubpassConfig subpass;
        subpass.colorAttachments.push_back(0);
        config.subpasses.push_back(subpass);

        return config;
    }
};

class VulkanRenderPass
{
public:
    VulkanRenderPass();
    ~VulkanRenderPass();

    // RAII
    VulkanRenderPass(const VulkanRenderPass&) = delete;
    VulkanRenderPass& operator=(const VulkanRenderPass&) = delete;
    VulkanRenderPass(VulkanRenderPass&& other) noexcept;
    VulkanRenderPass& operator=(VulkanRenderPass&& other) noexcept;

    // Lifecycle
    bool Initialize(std::shared_ptr<VulkanInstance> instance,
        std::shared_ptr<VulkanDevice> device,
        const RenderPassConfig& config);
    void Cleanup();
    bool IsInitialized() const { return m_renderPass != VK_NULL_HANDLE; }

    // Recording
    void Begin(VkCommandBuffer commandBuffer,
        VkFramebuffer framebuffer,
        VkExtent2D renderArea,
        const std::vector<VkClearValue>& clearValues = {});
    void End(VkCommandBuffer commandBuffer);

    // Getters
    VkRenderPass GetRenderPass() const { return m_renderPass; }
    const RenderPassConfig& GetConfig() const { return m_config; }
    uint32_t GetAttachmentCount() const { return static_cast<uint32_t>(m_config.attachments.size()); }
    std::vector<VkClearValue> GetDefaultClearValues() const;

    // Debug
    std::string GetRenderPassInfo() const;

private:
    // Handles
    std::shared_ptr<VulkanInstance> m_instance;
    std::shared_ptr<VulkanDevice> m_device;
    VkRenderPass m_renderPass = VK_NULL_HANDLE;
    RenderPassConfig m_config;

    // Logging
    static const Debug::DebugOutput DebugOut;

    // Internals
    bool CreateRenderPass();
    bool ValidateDependencies() const;
    bool ValidateConfig(const RenderPassConfig& config) const;

    // Errors
    void ReportError(const std::string& message) const
    {
        DebugOut.outputDebug("VulkanRenderPass Error: " + message);
    }

    void ReportWarning(const std::string& message) const
    {
        DebugOut.outputDebug("VulkanRenderPass Warning: " + message);
    }
};
