#pragma once

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include "../VulkanInstance/VulkanInstance.h"
#include "../VulkanDevice/VulkanDevice.h"
#include "../../DebugOutput/DubugOutput.h"

// Binding description
struct DescriptorBinding
{
    uint32_t binding;
    VkDescriptorType type;
    VkShaderStageFlags stages;
    uint32_t count = 1;
};

class VulkanDescriptor
{
public:
    VulkanDescriptor();
    ~VulkanDescriptor() = default;

    // Non-copyable
    VulkanDescriptor(const VulkanDescriptor&) = delete;
    VulkanDescriptor& operator=(const VulkanDescriptor&) = delete;

    // Lifecycle
    bool Initialize(std::shared_ptr<VulkanInstance> instance,
        std::shared_ptr<VulkanDevice> device);
    void Cleanup();
    bool IsInitialized() const { return m_descriptorSetLayout != VK_NULL_HANDLE; }

    bool ValidateDependencies() const;

    // Setup (call before Build)
    void AddBinding(uint32_t binding,
        VkDescriptorType type,
        VkShaderStageFlags stages,
        uint32_t count = 1);

    // Build layout and allocate set
    bool Build(uint32_t maxSets = 1);

    // Bindings
    bool BindBuffer(uint32_t binding, VkBuffer buffer, VkDeviceSize size, VkDeviceSize offset = 0, uint32_t arrayElement = 0);
    bool BindImage(uint32_t binding, VkImageView imageView, VkSampler sampler,
        VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        uint32_t arrayElement = 0);

    // Getters
    VkDescriptorSetLayout GetLayout() const { return m_descriptorSetLayout; }
    VkDescriptorSet GetSet() const { return m_descriptorSet; }

private:
    std::shared_ptr<VulkanInstance> m_instance;
    std::shared_ptr<VulkanDevice> m_device;

    std::vector<DescriptorBinding> m_bindings;

    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;

    static const Debug::DebugOutput DebugOut;

    bool CreateDescriptorSetLayout();
    bool CreateDescriptorPool(uint32_t maxSets);
    bool AllocateDescriptorSet();

    void ReportError(const std::string& message) const
    {
        DebugOut.outputDebug("VulkanDescriptor Error: " + message);
    }
};