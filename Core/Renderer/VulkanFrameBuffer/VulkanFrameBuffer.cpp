#include "VulkanFrameBuffer.h"

const Debug::DebugOutput VulkanFrameBuffer::DebugOut;

VulkanFrameBuffer::VulkanFrameBuffer()
{
    m_framebuffer = VK_NULL_HANDLE;
    m_width = 0;
    m_height = 0;
}

void VulkanFrameBuffer::Cleanup()
{
    if (m_framebuffer != VK_NULL_HANDLE && m_device && m_device->IsInitialized())
    {
        vkDestroyFramebuffer(m_device->GetDevice(), m_framebuffer, nullptr);
        m_framebuffer = VK_NULL_HANDLE;
    }

    m_renderPass.reset();
    m_device.reset();
    m_instance.reset();
    m_width = 0;
    m_height = 0;
}

VulkanFrameBuffer::~VulkanFrameBuffer()
{
    Cleanup();
}

bool VulkanFrameBuffer::ValidateDependencies() const
{
    if (!m_instance)
    {
        ReportError("Instance null. 0x00008000");
        return false;
    }

    if (!m_instance->IsInitialized())
    {
        ReportError("Instance not initialized. 0x00008010");
        return false;
    }

    if (!m_device)
    {
        ReportError("Device null. 0x00008020");
        return false;
    }

    if (!m_device->IsInitialized())
    {
        ReportError("Device not initialized. 0x00008030");
        return false;
    }

    if (!m_renderPass)
    {
        ReportError("Render pass null. 0x00008040");
        return false;
    }

    if (!m_renderPass->IsInitialized())
    {
        ReportError("Render pass not initialized. 0x00008050");
        return false;
    }

    return true;
}

bool VulkanFrameBuffer::Initialize(
    std::shared_ptr<VulkanInstance> instance,
    std::shared_ptr<VulkanDevice> device,
    std::shared_ptr<VulkanRenderPass> renderPass,
    const std::vector<VkImageView>& attachments,
    uint32_t width,
    uint32_t height)
{
    try
    {
        m_instance = instance;
        m_device = device;
        m_renderPass = renderPass;
        m_width = width;
        m_height = height;

        if (!ValidateDependencies())
            return false;

        if (attachments.empty())
        {
            ReportError("Attachments cannot be empty. 0x00008100");
            return false;
        }

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_renderPass->GetRenderPass();
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = width;
        framebufferInfo.height = height;
        framebufferInfo.layers = 1;

        VkResult result = vkCreateFramebuffer(m_device->GetDevice(), &framebufferInfo, nullptr, &m_framebuffer);
        if (result != VK_SUCCESS)
        {
            ReportError("Failed to create framebuffer. 0x00008110");
            return false;
        }

        return true;
    }
    catch (const std::exception& e)
    {
        ReportError("Exception during initialization: " + std::string(e.what()));
        return false;
    }
    catch (...)
    {
        ReportError("Unknown exception during initialization. 0x00008120");
        return false;
    }
}

std::string VulkanFrameBuffer::GetFramebufferInfo() const
{
    if (!IsInitialized())
        return "Framebuffer not initialized";

    std::string info = "VulkanFramebuffer Info:\n";
    info += "  Framebuffer Handle: " + std::to_string(reinterpret_cast<uint64_t>(m_framebuffer)) + "\n";
    info += "  Dimensions: " + std::to_string(m_width) + "x" + std::to_string(m_height) + "\n";

    return info;
}