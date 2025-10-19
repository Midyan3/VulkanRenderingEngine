#include "VulkanDescriptor.h"


VulkanDescriptor::VulkanDescriptor(){}

void VulkanDescriptor::Cleanup()
{
    if (m_device && m_device->IsInitialized())
    {
        if (m_descriptorPool != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorPool(m_device->GetDevice(), m_descriptorPool, nullptr);
            m_descriptorPool = VK_NULL_HANDLE;
        }

        if (m_descriptorSetLayout != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(m_device->GetDevice(), m_descriptorSetLayout, nullptr);
            m_descriptorSetLayout = VK_NULL_HANDLE;
        }

        m_descriptorSet = VK_NULL_HANDLE;
    }

    m_bindings.clear();
    m_instance.reset();
    m_device.reset();
}

bool VulkanDescriptor::ValidateDependencies() const
{
    if (!m_instance)
    {
        ReportError("Instance is null. 0x0000D000");
        return false;
    }

    if (!m_instance->IsInitialized())
    {
        ReportError("Instance not initialized. 0x0000D010");
        return false;
    }

    if (!m_device)
    {
        ReportError("Device is null. 0x0000D020");
        return false;
    }

    if (!m_device->IsInitialized())
    {
        ReportError("Device not initialized. 0x0000D030");
        return false;
    }

    return true;
}

bool VulkanDescriptor::Initialize(
    std::shared_ptr<VulkanInstance> instance,
    std::shared_ptr<VulkanDevice> device)
{
    try
    {
        m_instance = instance;
        m_device = device;

        if (!ValidateDependencies())
        {
            return false;
        }

        return true;
    }
    catch (const std::exception& e)
    {
        ReportError("Exception during initialization: " + std::string(e.what()) + " 0x0000D040");
        return false;
    }
    catch (...)
    {
        ReportError("Unknown exception during initialization. 0x0000D050");
        return false;
    }
}

void VulkanDescriptor::AddBinding(
    uint32_t binding,
    VkDescriptorType type,
    VkShaderStageFlags stages,
    uint32_t count)
{
    // TODO: Create DescriptorBinding struct
    // TODO: Fill in the values
    // TODO: Add to m_bindings vector
    DescriptorBinding bind = {};
    bind.binding = binding;
    bind.type = type;
    bind.stages = stages; 
    bind.count = count;
    
    m_bindings.push_back(std::move(bind)); 
}

bool VulkanDescriptor::CreateDescriptorSetLayout()
{
    std::vector<VkDescriptorSetLayoutBinding> layoutBindings;

    for (const auto& binding : m_bindings)
    {
        VkDescriptorSetLayoutBinding layoutBinding = {};
        layoutBinding.binding = binding.binding;
        layoutBinding.descriptorType = binding.type;
        layoutBinding.descriptorCount = binding.count;
        layoutBinding.stageFlags = binding.stages;
        layoutBinding.pImmutableSamplers = nullptr;

        layoutBindings.push_back(layoutBinding);
    }
   
    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
    layoutInfo.pBindings = layoutBindings.data();

    VkResult result = vkCreateDescriptorSetLayout(
        m_device->GetDevice(),
        &layoutInfo,
        nullptr,
        &m_descriptorSetLayout
    );

    if (result != VK_SUCCESS)
    {
        ReportError("Failed to create descriptor set layout. 0x0000D210");
        return false;
    }

    return true;
}

bool VulkanDescriptor::AllocateDescriptorSet()
{
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;   
    allocInfo.descriptorSetCount = 1;                  
    allocInfo.pSetLayouts = &m_descriptorSetLayout;

    VkResult result = vkAllocateDescriptorSets(
        m_device->GetDevice(),
        &allocInfo,
        &m_descriptorSet  
    );

    if (result != VK_SUCCESS)
    {
        ReportError("Failed to allocate descriptor set. 0x0000D230");
        return false;
    }

    return true;
}

bool VulkanDescriptor::CreateDescriptorPool(uint32_t maxSets)
{
    std::vector<VkDescriptorPoolSize> poolSizes; 

    for (const auto& binding : m_bindings)
    {
        VkDescriptorPoolSize poolSize = {}; 
        poolSize.descriptorCount = binding.count * maxSets;
        poolSize.type = binding.type;
        poolSizes.push_back(std::move(poolSize));
    }

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = maxSets;  

    VkResult result = vkCreateDescriptorPool(
        m_device->GetDevice(),
        &poolInfo,
        nullptr,
        &m_descriptorPool
    );

    if (result != VK_SUCCESS)
    {
        ReportError("Failed to create descriptor pool. 0x0000D220");
        return false;
    }

    return true;

}

bool VulkanDescriptor::BindBuffer(uint32_t binding,
    VkBuffer buffer,
    VkDeviceSize size,
    VkDeviceSize offset,
    uint32_t arrayElement)
{
    const DescriptorBinding* decl = nullptr;
    for (auto& b : m_bindings)
        if (b.binding == binding) { decl = &b; break; }

    if (!decl) 
    { 
        ReportError("BindBuffer: Unknown binding. 0x000BD300"); 
        return false; 
    }

    if (decl->type != VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER &&
        decl->type != VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC &&
        decl->type != VK_DESCRIPTOR_TYPE_STORAGE_BUFFER &&
        decl->type != VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC)
    {
        ReportError("BindBuffer called for a non-buffer descriptor type 0x000BD310");
        return false;
    }

    VkDescriptorBufferInfo info{};
    info.buffer = buffer;
    info.offset = offset;
    info.range = size;

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = m_descriptorSet;
    write.dstBinding = binding;
    write.dstArrayElement = arrayElement;     
    write.descriptorType = decl->type;       
    write.descriptorCount = 1;              
    write.pBufferInfo = &info;

    vkUpdateDescriptorSets(m_device->GetDevice(), 1, &write, 0, nullptr);
    return true;
}

bool VulkanDescriptor::BindImage(
    uint32_t binding,
    VkImageView imageView,
    VkSampler sampler,
    VkImageLayout imageLayout,
    uint32_t arrayElement)
{
    // Find the declared binding
    const DescriptorBinding* decl = nullptr;
    for (auto& b : m_bindings)
    {
        if (b.binding == binding)
        {
            decl = &b;
            break;
        }
    }

    if (!decl)
    {
        ReportError("BindImage: Unknown binding. 0x0000D300");
        return false;
    }

    // Check if it's an image/sampler type
    if (decl->type != VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER &&
        decl->type != VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE &&
        decl->type != VK_DESCRIPTOR_TYPE_STORAGE_IMAGE &&
        decl->type != VK_DESCRIPTOR_TYPE_SAMPLER)
    {
        ReportError("BindImage: Called for non-image descriptor type. 0x0000D310");
        return false;
    }

    // Describe the image
    VkDescriptorImageInfo imageInfo = {};
    imageInfo.imageLayout = imageLayout;
    imageInfo.imageView = imageView;
    imageInfo.sampler = sampler;

    // Update descriptor set
    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = m_descriptorSet;
    write.dstBinding = binding;
    write.dstArrayElement = arrayElement;
    write.descriptorType = decl->type;
    write.descriptorCount = 1;
    write.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(m_device->GetDevice(), 1, &write, 0, nullptr);

    return true;
}

bool VulkanDescriptor::Build(uint32_t maxSets)
{
    // TODO: Step 1 - Check we have bindings
    if (m_bindings.empty())
    {
        ReportError("No bindings added. 0x0000D200");
        return false;
    }

    // TODO: Step 2 - Create descriptor set layout
    if (!CreateDescriptorSetLayout())
    {
        return false;
    }

    // TODO: Step 3 - Create descriptor pool
    if (!CreateDescriptorPool(maxSets))
    {
        return false;
    }

    // TODO: Step 4 - Allocate descriptor set from pool
    if (!AllocateDescriptorSet())
    {
        return false;
    }

    return true;
}