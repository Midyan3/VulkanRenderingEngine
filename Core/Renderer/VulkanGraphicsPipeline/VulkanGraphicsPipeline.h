#pragma once

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include <string>
#include "../VulkanInstance/VulkanInstance.h"
#include "../VulkanDevice/VulkanDevice.h"
#include "../VulkanRenderPass/VulkanRenderPass.h"
#include "../../DebugOutput/DubugOutput.h"

// Types
struct ShaderStage
{
    std::string filepath;
    VkShaderStageFlagBits stage;
    std::string entryPoint = "main";

    static ShaderStage Vertex(const std::string& path)
    {
        return { path, VK_SHADER_STAGE_VERTEX_BIT, "main" };
    }

    static ShaderStage Fragment(const std::string& path)
    {
        return { path, VK_SHADER_STAGE_FRAGMENT_BIT, "main" };
    }
};

// Config
struct VertexInputDescription
{
    std::vector<VkVertexInputBindingDescription> bindings;
    std::vector<VkVertexInputAttributeDescription> attributes;

    static VertexInputDescription Empty()
    {
        return {};
    }
};

// Config
struct GraphicsPipelineConfig
{
    std::vector<ShaderStage> shaders;
    std::vector<VkPushConstantRange> pushConstantRanges;
    VertexInputDescription vertexInput;
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
    VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;
    VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
    VkFrontFace frontFace = VK_FRONT_FACE_CLOCKWISE;
    float lineWidth = 1.0f;
    bool depthTestEnable = false;
    bool depthWriteEnable = false;
    VkCompareOp depthCompareOp = VK_COMPARE_OP_LESS;
    bool blendEnable = false;
    VkExtent2D viewport = { 0, 0 };

    static GraphicsPipelineConfig SimpleTriangle(const std::string& vertPath, const std::string& fragPath)
    {
        GraphicsPipelineConfig config;
        config.shaders = {
            ShaderStage::Vertex(vertPath),
            ShaderStage::Fragment(fragPath)
        };
        config.vertexInput = VertexInputDescription::Empty();
        config.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        config.polygonMode = VK_POLYGON_MODE_FILL;
        config.cullMode = VK_CULL_MODE_NONE;
        return config;
    }
};

class VulkanGraphicsPipeline
{
public:
    VulkanGraphicsPipeline();
    ~VulkanGraphicsPipeline();

    // RAII 
    VulkanGraphicsPipeline(const VulkanGraphicsPipeline&) = delete;
    VulkanGraphicsPipeline& operator=(const VulkanGraphicsPipeline&) = delete;
    VulkanGraphicsPipeline(VulkanGraphicsPipeline&& other) noexcept;
    VulkanGraphicsPipeline& operator=(VulkanGraphicsPipeline&& other) noexcept;

    // Core
    bool Initialize(std::shared_ptr<VulkanInstance> instance,
        std::shared_ptr<VulkanDevice> device,
        std::shared_ptr<VulkanRenderPass> renderPass,
        const GraphicsPipelineConfig& config);
    void Cleanup();
    bool IsInitialized() const { return m_pipeline != VK_NULL_HANDLE; }

    // Bind
    void Bind(VkCommandBuffer commandBuffer);

    // Getters
    VkPipeline GetPipeline() const { return m_pipeline; }
    VkPipelineLayout GetLayout() const { return m_pipelineLayout; }

    // Debug
    std::string GetPipelineInfo() const;

private:
    std::shared_ptr<VulkanInstance> m_instance;
    std::shared_ptr<VulkanDevice> m_device;
    std::shared_ptr<VulkanRenderPass> m_renderPass;
    VkPipeline m_pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    std::vector<VkShaderModule> m_shaderModules;
    GraphicsPipelineConfig m_config;

    static const Debug::DebugOutput DebugOut;

    // Internal helpers
    bool ValidateDependencies() const;
    bool ValidateConfig(const GraphicsPipelineConfig& config) const;
    bool LoadShaderModule(const std::string& filepath, VkShaderModule& outModule);
    bool CreatePipelineLayout();
    bool CreatePipeline();
    std::vector<char> ReadFile(const std::string& filename);

    void ReportError(const std::string& message) const
    {
        DebugOut.outputDebug("VulkanGraphicsPipeline Error: " + message);
    }

    void ReportWarning(const std::string& message) const
    {
        DebugOut.outputDebug("VulkanGraphicsPipeline Warning: " + message);
    }
};
