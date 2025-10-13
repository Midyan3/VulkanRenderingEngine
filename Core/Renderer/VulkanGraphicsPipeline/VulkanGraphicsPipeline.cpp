#include "VulkanGraphicsPipeline.h"
#include <fstream>

const Debug::DebugOutput VulkanGraphicsPipeline::DebugOut;

VulkanGraphicsPipeline::VulkanGraphicsPipeline()
{
    m_pipeline = VK_NULL_HANDLE;
    m_pipelineLayout = VK_NULL_HANDLE;
}

void VulkanGraphicsPipeline::Cleanup()
{
    if (m_device && m_device->IsInitialized())
    {
        if (m_pipeline != VK_NULL_HANDLE)
        {
            vkDestroyPipeline(m_device->GetDevice(), m_pipeline, nullptr);
            m_pipeline = VK_NULL_HANDLE;
        }

        if (m_pipelineLayout != VK_NULL_HANDLE)
        {
            vkDestroyPipelineLayout(m_device->GetDevice(), m_pipelineLayout, nullptr);
            m_pipelineLayout = VK_NULL_HANDLE;
        }

        for (auto shaderModule : m_shaderModules)
        {
            vkDestroyShaderModule(m_device->GetDevice(), shaderModule, nullptr);
        }
        m_shaderModules.clear();
    }

    m_renderPass.reset();
    m_device.reset();
    m_instance.reset();
    m_config = GraphicsPipelineConfig();
}

VulkanGraphicsPipeline::~VulkanGraphicsPipeline()
{
    Cleanup();
}

bool VulkanGraphicsPipeline::ValidateDependencies() const
{
    if (!m_instance)
    {
        ReportError("Instance null. 0x00009000");
        return false;
    }

    if (!m_instance->IsInitialized())
    {
        ReportError("Instance not initialized. 0x00009010");
        return false;
    }

    if (!m_device)
    {
        ReportError("Device null. 0x00009020");
        return false;
    }

    if (!m_device->IsInitialized())
    {
        ReportError("Device not initialized. 0x00009030");
        return false;
    }

    if (!m_renderPass)
    {
        ReportError("Render pass null. 0x00009040");
        return false;
    }

    if (!m_renderPass->IsInitialized())
    {
        ReportError("Render pass not initialized. 0x00009050");
        return false;
    }

    return true;
}

bool VulkanGraphicsPipeline::ValidateConfig(const GraphicsPipelineConfig& config) const
{
    if (config.shaders.empty())
    {
        ReportError("Shaders vector cannot be empty. 0x00009100");
        return false;
    }

    if (config.viewport.width == 0 || config.viewport.height == 0)
    {
        ReportError("Viewport dimensions cannot be zero. 0x00009110");
        return false;
    }

    for (const auto& shader : config.shaders)
    {
        if (shader.filepath.empty())
        {
            ReportError("Shader filepath cannot be empty. 0x00009120");
            return false;
        }
    }

    return true;
}

std::vector<char> VulkanGraphicsPipeline::ReadFile(const std::string& filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        ReportError("Failed to open file: " + filename + " 0x00009200");
        return {};
    }

    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}

bool VulkanGraphicsPipeline::LoadShaderModule(const std::string& filepath, VkShaderModule& outModule)
{
    std::vector<char> code = ReadFile(filepath);
    if (code.empty())
    {
        ReportError("Failed to read shader file: " + filepath + " 0x00009210");
        return false;
    }

    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkResult result = vkCreateShaderModule(m_device->GetDevice(), &createInfo, nullptr, &outModule);
    if (result != VK_SUCCESS)
    {
        ReportError("Failed to create shader module: " + filepath + " 0x00009220");
        return false;
    }

    return true;
}

bool VulkanGraphicsPipeline::CreatePipelineLayout()
{
    VkPipelineLayoutCreateInfo pipelineInfo = {};

    pipelineInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO; 
    pipelineInfo.setLayoutCount = 0;
    pipelineInfo.pSetLayouts = nullptr;
    //Add pushConstant 3.
    pipelineInfo.pushConstantRangeCount = static_cast<uint32_t>(m_config.pushConstantRanges.size());
    pipelineInfo.pPushConstantRanges = m_config.pushConstantRanges.data();
    //End of 3.

    VkResult result = vkCreatePipelineLayout(
        m_device->GetDevice(),
        &pipelineInfo,
        nullptr,
        &m_pipelineLayout
    );

    if (result != VK_SUCCESS)
    {
        ReportError("Could not create pipeline layout. 0x00009300"); 
        return false;
    }

    return true;

}

bool VulkanGraphicsPipeline::CreatePipeline()
{
    // Main graphic pipeline function creator. Sections for future changes and TODO's
    // Section 1: Load shader modules and create stage info
    // Section 2: Vertex input state
    // Section 3: Input assembly state
    // Section 4: Viewport and scissor
    // Section 5: Rasterization state
    // Section 6: Multisampling state
    // Section 7: Depth/stencil state
    // Section 8: Color blend state
    // Section 9: Create graphics pipeline

    std::vector<VkPipelineShaderStageCreateInfo> shaderStageInfos;

    for (const auto& shaderStage : m_config.shaders)
    {
        VkShaderModule shaderModule;
        if (!LoadShaderModule(shaderStage.filepath, shaderModule))
            return false;

        m_shaderModules.push_back(shaderModule);  // Storing here for cleanup. See Cleanup()

        VkPipelineShaderStageCreateInfo stageInfo = {};
        stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stageInfo.stage = shaderStage.stage;
        stageInfo.module = shaderModule;
        stageInfo.pName = shaderStage.entryPoint.c_str();

        shaderStageInfos.push_back(stageInfo); 
    }

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(m_config.vertexInput.bindings.size());
    vertexInputInfo.pVertexBindingDescriptions = m_config.vertexInput.bindings.empty() ? nullptr : m_config.vertexInput.bindings.data();
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(m_config.vertexInput.attributes.size());
    vertexInputInfo.pVertexAttributeDescriptions = m_config.vertexInput.attributes.empty() ? nullptr : m_config.vertexInput.attributes.data();


    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = m_config.topology;  
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};

    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.height = static_cast<float>(m_config.viewport.height);
    viewport.width = static_cast<float>(m_config.viewport.width);

    VkRect2D scissor = {};
    scissor.offset = { 0, 0 }; 
    scissor.extent = m_config.viewport;

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = m_config.polygonMode;  
    rasterizer.lineWidth = m_config.lineWidth;
    rasterizer.cullMode = m_config.cullMode;       
    rasterizer.frontFace = m_config.frontFace;     
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT; 

    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = m_config.depthTestEnable ? VK_TRUE : VK_FALSE;
    depthStencil.depthWriteEnable = m_config.depthWriteEnable ? VK_TRUE : VK_FALSE;
    depthStencil.depthCompareOp = m_config.depthCompareOp;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = m_config.blendEnable ? VK_TRUE : VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = static_cast<uint32_t>(shaderStageInfos.size());
    pipelineInfo.pStages = shaderStageInfos.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = m_renderPass->GetRenderPass();
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    VkResult result = vkCreateGraphicsPipelines(m_device->GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline);
    if (result != VK_SUCCESS)
    {
        ReportError("Failed to create graphics pipeline. 0x00009400");
        return false;
    }

    return true;
}

bool VulkanGraphicsPipeline::Initialize(
    std::shared_ptr<VulkanInstance> instance,
    std::shared_ptr<VulkanDevice> device,
    std::shared_ptr<VulkanRenderPass> renderPass,
    const GraphicsPipelineConfig& config)
{
    try
    {
        m_instance = instance;
        m_device = device;
        m_renderPass = renderPass;
        m_config = config;

        if (!ValidateDependencies())
            return false;

        if (!ValidateConfig(m_config))
            return false;

        if (!CreatePipelineLayout())
            return false;

        if (!CreatePipeline())
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
        ReportError("Unknown exception. 0x00009500");
        return false;
    }
}

void VulkanGraphicsPipeline::Bind(VkCommandBuffer commandBuffer)
{
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
}

std::string VulkanGraphicsPipeline::GetPipelineInfo() const
{
    if (!IsInitialized())
    {
        return "Pipeline not initialized";
    }

    std::string output = "VulkanGraphicsPipeline Info:\n";
    output += "  Pipeline Handle: " + std::to_string(reinterpret_cast<uint64_t>(m_pipeline)) + "\n";
    output += "  Layout Handle: " + std::to_string(reinterpret_cast<uint64_t>(m_pipelineLayout)) + "\n";
    output += "  Shader Count: " + std::to_string(m_shaderModules.size()) + "\n";
    output += "  Viewport: " + std::to_string(m_config.viewport.width) + "x" + std::to_string(m_config.viewport.height) + "\n";
    return output;
}