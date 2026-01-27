#include "VulkanImage.h"

const Debug::DebugOutput VulkanImage::DebugOut;

static bool HasStencil(VkFormat fmt) 
{
    return fmt == VK_FORMAT_D32_SFLOAT_S8_UINT || fmt == VK_FORMAT_D24_UNORM_S8_UINT;
}

static VkImageAspectFlags AspectFromFormat(VkFormat fmt) 
{
    switch (fmt) {
    case VK_FORMAT_D16_UNORM:
    case VK_FORMAT_D32_SFLOAT:
        return VK_IMAGE_ASPECT_DEPTH_BIT;
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
    case VK_FORMAT_D24_UNORM_S8_UINT:
        return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    default:
        return VK_IMAGE_ASPECT_COLOR_BIT;
    }
}

VulkanImage::VulkanImage()
{
}

VulkanImage::~VulkanImage()
{
    Cleanup();
}

VulkanImage::VulkanImage(VulkanImage&& other) noexcept
    : m_instance(std::move(other.m_instance))
    , m_device(std::move(other.m_device))
    , m_allocator(std::move(other.m_allocator))
{
}

VulkanImage& VulkanImage::operator=(VulkanImage&& other) noexcept
{
    if (this != &other)
    {
        Cleanup();
        m_instance = std::move(other.m_instance);
        m_device = std::move(other.m_device);
        m_allocator = std::move(other.m_allocator);
    }
    return *this;
}

bool VulkanImage::Initialize(
    std::shared_ptr<VulkanInstance> instance,
    std::shared_ptr<VulkanDevice> device,
    std::shared_ptr<VulkanMemoryAllocator> allocator)
{
    try
    {
        m_instance = instance;
        m_device = device;
        m_allocator = allocator;

        if (!ValidateDependencies())
        {
            m_instance.reset(); 
            m_device.reset(); 
            m_allocator.reset(); 
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
        ReportError("Unknown exception during initialization. 0x00010040");
        return false;
    }
}

void VulkanImage::Cleanup()
{
    m_instance.reset();
    m_device.reset();
    m_allocator.reset();
}

bool VulkanImage::ValidateDependencies() const  
{
    if (!m_instance)
    {
        ReportError("Instance null. 0x00010005");
        return false;
    }
    if (!m_instance->IsInitialized())
    {
        ReportError("Instance not initialized. 0x00010010");
        return false;
    }
    if (!m_device)
    {
        ReportError("Device null. 0x00010015");
        return false;
    }
    if (!m_device->IsInitialized())
    {
        ReportError("Device not initialized. 0x00010020"); 
        return false;
    }

    if (!m_allocator)
    {
        ReportError("Allocator null. 0x00010025"); 
        return false;
    }
    if (!m_allocator->IsInitialized())
    {
        ReportError("Allocator not initialized. 0x00010030"); 
        return false;
    }

    return true;
}

bool VulkanImage::ValidateImageCreateInfo(const ImageCreateInfo& createInfo) const
{
    if (createInfo.width == 0 || createInfo.height == 0)
    {
        ReportError("Image dimensions cannot be zero. 0x00010100");
        return false;
    }

    if (createInfo.mipLevels == 0)
    {
        ReportError("Mip levels cannot be zero. 0x00010110");
        return false;
    }

    if (createInfo.arrayLayers == 0)
    {
        ReportError("Array layers cannot be zero. 0x00010120");
        return false;
    }

    if (createInfo.format == VK_FORMAT_UNDEFINED)
    {
        ReportError("Image format cannot be undefined. 0x00010130");
        return false;
    }

    return true;
}

bool VulkanImage::CreateImage(const ImageOptions& options, AllocatedImage& outImage)
{
    if (!IsInitialized())
    {
        ReportError("Not initialized yet (Make sure Initialize was called with proper dependencies). 0x00011000");
        return false;
    }

    ImageCreateInfo info = ImageCreateInfo::FromOptions(options);

    return CreateImage(info, outImage);
}

bool VulkanImage::CreateImage(const ImageCreateInfo& createInfo, AllocatedImage& outImage)
{
    if (!IsInitialized())
    {
        ReportError("Not initialized yet. 0x00011010");
        return false;
    }

    if (!ValidateImageCreateInfo(createInfo))
    {
        return false;
    }

    try
    {
        VkImageCreateInfo imageInfo = createInfo.ToVulkan(); 

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        VkImage image = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;
        VmaAllocationInfo allocationInfo = {};

        VkResult result = vmaCreateImage(
            m_allocator->GetAllocator(),
            &imageInfo,
            &allocInfo,
            &image,
            &allocation,
            &allocationInfo
        );

        if (result != VK_SUCCESS)
        {
            ReportError("Failed to create image. 0x00011020");
            return false;
        }

        outImage.image = image;
        outImage.allocation = allocation;
        outImage.allocationInfo = allocationInfo;
        outImage.extent.width = createInfo.width;
        outImage.extent.height = createInfo.height;
        outImage.extent.depth = createInfo.depth;
        outImage.format = createInfo.format;
        outImage.mipLevels = createInfo.mipLevels;
        outImage.arrayLayers = createInfo.arrayLayers;
        outImage.currentLayout = createInfo.initialLayout;

        return true;
    }
    catch (const std::exception& e)
    {
        ReportError("Exception during image creation: " + std::string(e.what()));
        return false;
    }
    catch (...)
    {
        ReportError("Unknown exception during image creation. 0x00011030");
        return false;
    }
}

void VulkanImage::DestroyImage(AllocatedImage& image)
{
    if (!image.IsValid())
    {
        ReportWarning("Attempted to destroy invalid image. 0x00011100");
        return;
    }

    if (!m_allocator || !m_allocator->IsInitialized())
    {
        ReportError("Cannot destroy image: allocator not initialized. 0x00011110");
        return;
    }

    vmaDestroyImage(m_allocator->GetAllocator(), image.image, image.allocation);

    image.image = VK_NULL_HANDLE;
    image.allocation = VK_NULL_HANDLE;
    image.allocationInfo = {};
    image.extent = { 0, 0, 0 };
    image.format = VK_FORMAT_UNDEFINED;
    image.mipLevels = 0;
    image.arrayLayers = 0;
    image.currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
}

VkAccessFlags VulkanImage::GetAccessMask(VkImageLayout layout) const
{
    switch (layout)
    {
    case VK_IMAGE_LAYOUT_UNDEFINED:
        return 0;  

    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        return VK_ACCESS_TRANSFER_WRITE_BIT;  // Writing w/ transfer

    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        return VK_ACCESS_TRANSFER_READ_BIT;  // Reading w/ transfer

    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        return VK_ACCESS_SHADER_READ_BIT;  // Reading in shader

    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;  // Writing as render target

    case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
        return 0; 

    default:
        return 0;
    }
}

bool VulkanImage::TransitionLayout(
    VkCommandBuffer cmd,
    AllocatedImage& image,
    VkImageLayout newLayout,
    VkPipelineStageFlags srcStage,
    VkPipelineStageFlags dstStage

)
{
    if (!IsInitialized())
    {
        ReportError("Not initialized yet. 0x00012000");
        return false;
    }

    if (!image.IsValid())
    {
        ReportError("Image provided not valid. 0x00012010");
        return false;
    }
    
    VkImageMemoryBarrier barrier{}; 
    barrier.image = image.image; 
    barrier.oldLayout = image.currentLayout; 
    barrier.newLayout = newLayout; 
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // Ignoring here | Not transferring bewteen
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // Same reason here  
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER; 
    barrier.subresourceRange.aspectMask = AspectFromFormat(image.format);
    barrier.subresourceRange.baseMipLevel = 0;
    
    barrier.subresourceRange.levelCount = image.mipLevels; 
    barrier.subresourceRange.baseArrayLayer = 0; 
    barrier.subresourceRange.layerCount = image.arrayLayers; 

    barrier.srcAccessMask = GetAccessMask(image.currentLayout); 
    barrier.dstAccessMask = GetAccessMask(newLayout); 

    vkCmdPipelineBarrier(
        cmd,
        srcStage,
        dstStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    ); 

    image.currentLayout = newLayout; 

    return true;

}

bool VulkanImage::UploadData(
    VulkanCommandBuffer* commandBuffer,
    AllocatedImage& image,
    const void* data,
    size_t dataSize,
    bool transitionToShaderOptimal)  
{
    if (!IsInitialized()) {
        ReportError("Not initialized. 0x00012000");
        return false;
    }
    if (!image.IsValid()) {
        ReportError("Invalid image. 0x00012010");
        return false;
    }
    if (!commandBuffer || !commandBuffer->IsInitialized()) {
        ReportError("Invalid command buffer. 0x00012020");
        return false;
    }
    if (!data || dataSize == 0) {
        ReportError("Invalid data. 0x00012030");
        return false;
    }

    AllocatedBuffer stagingBuffer;
    if (!m_allocator->CreateBuffer(
        dataSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        MemoryAllocationInfo::Staging(),
        stagingBuffer))
    {
        ReportError("Failed to create staging buffer. 0x00012040");
        return false;
    }

    if (!m_allocator->MapMemory(stagingBuffer)) {
        m_allocator->DestroyBuffer(stagingBuffer);
        ReportError("Failed to map staging buffer. 0x00012050");
        return false;
    }
    memcpy(stagingBuffer.mappedData, data, dataSize);
    m_allocator->UnmapMemory(stagingBuffer);

    VkCommandBuffer cmd = commandBuffer->BeginSingleTimeCommands();
    if (cmd == VK_NULL_HANDLE) {
        m_allocator->DestroyBuffer(stagingBuffer);
        ReportError("Failed to begin command buffer. 0x00012060");
        return false;
    }

    if (!TransitionLayout(
        cmd,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT))
    {
        m_allocator->DestroyBuffer(stagingBuffer);
        ReportError("Failed to transition to TRANSFER_DST. 0x00012065");
        return false;
    }

    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = AspectFromFormat(image.format);
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = {
        image.extent.width,
        image.extent.height,
        1
    };

    vkCmdCopyBufferToImage(
        cmd,
        stagingBuffer.buffer,
        image.image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region
    );

    if (transitionToShaderOptimal) {
        if (!TransitionLayout(
            cmd,
            image,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_TRANSFER_BIT,       
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT  
        )) {
            m_allocator->DestroyBuffer(stagingBuffer);
            ReportError("Failed to transition to SHADER_READ. 0x00012067");
            return false;
        }
    }

    if (!commandBuffer->EndSingleTimeCommands(cmd)) {
        m_allocator->DestroyBuffer(stagingBuffer);
        ReportError("Failed to submit commands. 0x00012070");
        return false;
    }

    m_allocator->DestroyBuffer(stagingBuffer);

    return true;
}