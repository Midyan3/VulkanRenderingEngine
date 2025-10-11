#include "VulkanRenderPass.h"
#include <set>

const Debug::DebugOutput VulkanRenderPass::DebugOut;

void VulkanRenderPass::Cleanup()
{
    if (m_renderPass != VK_NULL_HANDLE && m_device && m_device->IsInitialized())
    {
        vkDestroyRenderPass(m_device->GetDevice(), m_renderPass, nullptr);
        m_renderPass = VK_NULL_HANDLE;
    }

    m_device.reset();
    m_instance.reset();
    m_config = RenderPassConfig();  // Reset to default
}

VulkanRenderPass::VulkanRenderPass()
{
    m_renderPass = VK_NULL_HANDLE;
}

VulkanRenderPass::~VulkanRenderPass()
{
    Cleanup();
}

bool VulkanRenderPass::ValidateDependencies() const
{
    if (!m_instance)
    {
        ReportError("Instance null. 0x00007000");
        return false;
    }

    if (!m_instance->IsInitialized())
    {
        ReportError("Instance not initialized. 0x00007010");
        return false;
    }

    if (!m_device)
    {
        ReportError("Device null. 0x00007020");
        return false;
    }

    if (!m_device->IsInitialized())
    {
        ReportError("Device not initialized. 0x00007030");
        return false;
    }

    return true;
}

bool VulkanRenderPass::ValidateConfig(const RenderPassConfig& config) const
{
    if (config.attachments.empty())
    {
        ReportError("Render pass must have at least one attachment. 0x00007100");
        return false;
    }

    if (config.subpasses.empty())
    {
        ReportError("Render pass must have at least one subpass. 0x00007110");
        return false;
    }

    for (const auto& attachment : config.attachments)
    {
        if (attachment.format == VK_FORMAT_UNDEFINED)
        {
            ReportError("Attachment format cannot be undefined. 0x00007140");
            return false;
        }
    }

    for (size_t i = 0; i < config.subpasses.size(); i++)
    {
        const auto& subpass = config.subpasses[i];

        for (uint32_t colorIdx : subpass.colorAttachments)
        {
            if (colorIdx >= config.attachments.size())
            {
                ReportError("Subpass references invalid color attachment. 0x00007120");
                return false;
            }
        }

        if (subpass.depthAttachment >= 0 &&
            static_cast<uint32_t>(subpass.depthAttachment) >= config.attachments.size())
        {
            ReportError("Subpass references invalid depth attachment. 0x00007130");
            return false;
        }
    }

    return true;
}

bool VulkanRenderPass::CreateRenderPass()
{
    std::vector<VkAttachmentDescription> attachments;
    for (const auto& attachment : m_config.attachments)
    {
        VkAttachmentDescription desc = {};
        desc.format = attachment.format;
        desc.samples = attachment.samples;
        desc.loadOp = attachment.loadOp;
        desc.storeOp = attachment.storeOp;
        desc.stencilLoadOp = attachment.stencilLoadOp;
        desc.stencilStoreOp = attachment.stencilStoreOp;
        desc.initialLayout = attachment.initialLayout;
        desc.finalLayout = attachment.finalLayout;
        attachments.push_back(desc);
    }

    //subpass descriptions building for the renderpass. 
    std::vector<VkSubpassDescription> subpassDescs;
    std::vector<std::vector<VkAttachmentReference>> colorRefs(m_config.subpasses.size());
    std::vector<VkAttachmentReference> depthRefs(m_config.subpasses.size());

    for (size_t i = 0; i < m_config.subpasses.size(); i++)
    {
        const auto& subpass = m_config.subpasses[i];

        for (uint32_t attachmentIdx : subpass.colorAttachments)
        {
            VkAttachmentReference colorRef = {};
            colorRef.attachment = attachmentIdx;
            colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            colorRefs[i].push_back(colorRef);
        }

        VkAttachmentReference* pDepthRef = nullptr;
        if (subpass.depthAttachment >= 0)
        {
            depthRefs[i].attachment = static_cast<uint32_t>(subpass.depthAttachment);
            depthRefs[i].layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            pDepthRef = &depthRefs[i];
        }

        VkSubpassDescription desc = {};
        desc.pipelineBindPoint = subpass.bindPoint;
        desc.colorAttachmentCount = static_cast<uint32_t>(colorRefs[i].size());
        desc.pColorAttachments = colorRefs[i].data();
        desc.pDepthStencilAttachment = pDepthRef;

        subpassDescs.push_back(desc);
    }

    std::vector<VkSubpassDependency> dependencies(2);
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;  // Before render pass
    dependencies[0].dstSubpass = 0;                     // First subpass will be the dist. Only one subpass was made because it was simpler. Will add functionailty for more.
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = 0;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = 0;

    dependencies[1].dependencyFlags = 0;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = 0; 
    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;

    VkRenderPassCreateInfo renderPassInfo = {}; 

    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO; 
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderPassInfo.subpassCount = static_cast<uint32_t>(subpassDescs.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.pDependencies = dependencies.data();
    renderPassInfo.pSubpasses = subpassDescs.data(); 

    VkResult result = vkCreateRenderPass(m_device->GetDevice(), &renderPassInfo, nullptr, &m_renderPass);

    if (result != VK_SUCCESS)
    {
        ReportError("Failed to create render pass. 0x000F7200");
        return false;
    }

    return true;
}

bool VulkanRenderPass::Initialize(
    std::shared_ptr<VulkanInstance> instance,
    std::shared_ptr<VulkanDevice> device,
    const RenderPassConfig& config)
{
    try
    {
        m_instance = instance;
        m_device = device;
        m_config = config;

        if (!ValidateDependencies())
            return false;

        if (!ValidateConfig(m_config))
            return false;

        if (!CreateRenderPass())
            return false;

        return true;
    }
    catch (const std::exception& e)
    {
        ReportError("Exception during initialization: " + std::string(e.what()));
        return false;
    }
    catch (...)
    {
        ReportError("Unknown exception during initialization. 0x00007300");
        return false;
    }
}


// TLDR; Begin command for a specific renderpass. Tells Vulkan which renderpass to use and renderpass begin
// must be the begining command before draw calls(); 
void VulkanRenderPass::Begin(
    VkCommandBuffer commandBuffer,
    VkFramebuffer framebuffer,
    VkExtent2D renderArea,
    const std::vector<VkClearValue>& clearValues)
{
    VkRenderPassBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    beginInfo.renderPass = m_renderPass;
    beginInfo.framebuffer = framebuffer;
    beginInfo.renderArea.offset = { 0, 0 };
    beginInfo.renderArea.extent = renderArea;
    beginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    beginInfo.pClearValues = clearValues.empty() ? nullptr : clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

//Just tells Vulkan that this is the end of the render commands using this renderpass and ends it. ^
void VulkanRenderPass::End(VkCommandBuffer commandBuffer)
{
    vkCmdEndRenderPass(commandBuffer);
}

std::vector<VkClearValue> VulkanRenderPass::GetDefaultClearValues() const
{
    std::vector<VkClearValue> clearValues(m_config.attachments.size());

    for (size_t i = 0; i < m_config.attachments.size(); ++i)
    {
        const auto& attach = m_config.attachments[i]; 

        bool isDepth = (attach.format == VK_FORMAT_D32_SFLOAT ||
            attach.format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
            attach.format == VK_FORMAT_D24_UNORM_S8_UINT ||
            attach.format == VK_FORMAT_D16_UNORM);

        if (isDepth)
        {
            clearValues[i].depthStencil.depth = m_config.clearDepth; 
            clearValues[i].depthStencil.stencil = m_config.clearStencil;
        }
        else
        {
            clearValues[i].color = m_config.clearColor;
        }

    }
        
    return clearValues; 
}

std::string VulkanRenderPass::GetRenderPassInfo() const
{
    if (!IsInitialized())
        return "Render pass not initialized";

    std::string info = "VulkanRenderPass Info:\n";

    info += "  Render Pass Handle: " + std::to_string(reinterpret_cast<uint64_t>(m_renderPass)) + "\n";
    info += "  Attachment Count: " + std::to_string(m_config.attachments.size()) + "\n";
    info += "  Subpass Count: " + std::to_string(m_config.subpasses.size()) + "\n";

    for (size_t i = 0; i < m_config.attachments.size(); i++)
    {
        const auto& attachment = m_config.attachments[i];
        info += "  Attachment " + std::to_string(i) + ":\n";
        info += "    Format: " + std::to_string(attachment.format) + "\n";
        info += "    Load Op: " + std::to_string(attachment.loadOp) + "\n";
        info += "    Store Op: " + std::to_string(attachment.storeOp) + "\n";
    }

    return info;
}