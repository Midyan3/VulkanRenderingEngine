#include "VulkanImageView.h"

const Debug::DebugOutput VulkanImageView::DebugOut;

VulkanImageView::VulkanImageView()
{}

VulkanImageView::~VulkanImageView()
{
	Cleanup(); 
}

void VulkanImageView::Cleanup() 
{
	m_instance.reset(); 
	m_device.reset(); 
}

VulkanImageView::VulkanImageView(VulkanImageView&& other) noexcept
    : m_instance(std::move(other.m_instance))
    , m_device(std::move(other.m_device))
{

}

VulkanImageView& VulkanImageView::operator=(VulkanImageView&& other) noexcept
{
    if (this != &other)
    {
        Cleanup();
        m_instance = std::move(other.m_instance);
        m_device = std::move(other.m_device);
    }

    return *this;
}

bool VulkanImageView::ValidateDependencies() const
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

    return true;
}

bool VulkanImageView::IsInitialized() const
{
    return m_instance && m_device; 
}

bool VulkanImageView::Initialize(
    std::shared_ptr<VulkanInstance> instance,
    std::shared_ptr<VulkanDevice> device)
{
    try
    {
        m_instance = instance;
        m_device = device;

        if (!ValidateDependencies())
        {
            m_instance.reset();
            m_device.reset();
            return false;
        }

        return true;
    }
    catch (const std::exception& e)
    {
        ReportError("Exception during initialization: " + std::string(e.what()));
        m_instance.reset();
        m_device.reset();
        return false;
    }
    catch (...)
    {
        ReportError("Unknown exception during initialization. 0x00010025");
        m_instance.reset();
        m_device.reset();
        return false;
    }
}

bool VulkanImageView::ValidateImageViewCreateInfo(
    const AllocatedImage& image,
    const ImageViewCreateInfo& createInfo) const
{
    if (!image.IsValid())
    {
        ReportError("Invalid image. 0x00010100");
        return false;
    }

    if (createInfo.format != VK_FORMAT_UNDEFINED &&
        createInfo.format != image.format)
    {
        ReportWarning("View format differs from image format. 0x00010110");
    }

    if (createInfo.baseMipLevel >= image.mipLevels)
    {
        ReportError("baseMipLevel >= image mipLevels. 0x00010120");
        return false;
    }

    uint32_t actualLevelCount = createInfo.levelCount;
    if (actualLevelCount == VK_REMAINING_MIP_LEVELS)
    {
        actualLevelCount = image.mipLevels - createInfo.baseMipLevel;
    }

    if (createInfo.baseMipLevel + actualLevelCount > image.mipLevels)
    {
        ReportError("Mip range exceeds image mip levels. 0x00010130");
        return false;
    }

    if (createInfo.baseArrayLayer >= image.arrayLayers)
    {
        ReportError("baseArrayLayer >= image arrayLayers. 0x00010140");
        return false;
    }

    uint32_t actualLayerCount = createInfo.layerCount;
    if (actualLayerCount == VK_REMAINING_ARRAY_LAYERS)
    {
        actualLayerCount = image.arrayLayers - createInfo.baseArrayLayer;
    }

    if (createInfo.baseArrayLayer + actualLayerCount > image.arrayLayers)
    {
        ReportError("Layer range exceeds image array layers. 0x00010150");
        return false;
    }

    bool validViewType = false;
    switch (createInfo.viewType)
    {
    case VK_IMAGE_VIEW_TYPE_1D:
        validViewType = (image.extent.height == 1 && image.extent.depth == 1);
        break;
    case VK_IMAGE_VIEW_TYPE_2D:
        validViewType = (image.extent.depth == 1);
        break;
    case VK_IMAGE_VIEW_TYPE_3D:
        validViewType = true;  
        break;
    case VK_IMAGE_VIEW_TYPE_CUBE:
        validViewType = (actualLayerCount == 6 &&
            image.extent.width == image.extent.height);
        break;
    case VK_IMAGE_VIEW_TYPE_1D_ARRAY:
        validViewType = (image.extent.height == 1 && image.extent.depth == 1);
        break;
    case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
        validViewType = (image.extent.depth == 1);
        break;
    case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY:
        validViewType = (actualLayerCount >= 6 && actualLayerCount % 6 == 0 &&
            image.extent.width == image.extent.height);
        break;
    default:
        ReportError("Unknown view type. 0x00010160");
        return false;
    }

    if (!validViewType)
    {
        ReportError("View type incompatible with image dimensions. 0x00010170");
        return false;
    }

    return true;  
}

bool VulkanImageView::CreateView(const AllocatedImage& image,
    AllocatedImageView& outView,
    const ImageViewCreateInfo& createInfo)
{
    if (!IsInitialized())
    {
        return false; 
    }

    if (!image.IsValid())
    {
        return false; 
    }

    if (!ValidateImageViewCreateInfo(image, createInfo))
    {
        return false; 
    }
    
    VkImageViewCreateInfo vkCreateInfo = createInfo.ToVulkan(
        image.image,
        image.format
    );

    VkResult result = vkCreateImageView(
        m_device->GetDevice(),
        &vkCreateInfo,
        nullptr,
        &outView.view
    );

    if (result != VK_SUCCESS)
    {
        ReportError("Failed to create image view. 0x00010210");
        return false;
    }

    outView.type = createInfo.viewType;
    outView.format = vkCreateInfo.format;  
    outView.aspectMask = createInfo.aspectMask;
    outView.baseMipLevel = createInfo.baseMipLevel;

    if (createInfo.levelCount == VK_REMAINING_MIP_LEVELS)
    {
        outView.levelCount = image.mipLevels - createInfo.baseMipLevel;
    }
    else
    {
        outView.levelCount = createInfo.levelCount;
    }

    outView.baseArrayLayer = createInfo.baseArrayLayer;

    if (createInfo.layerCount == VK_REMAINING_ARRAY_LAYERS)
    {
        outView.layerCount = image.arrayLayers - createInfo.baseArrayLayer;
    }
    else
    {
        outView.layerCount = createInfo.layerCount;
    }

    return true;
 
}


bool VulkanImageView::CreateView(const AllocatedImage& image,
        AllocatedImageView& outView,
    const ImageViewOptions& options)
{
    ImageViewCreateInfo opts = ImageViewCreateInfo::FromOptions(options); 

    return CreateView(image, outView, opts);
}

VkImageViewCreateInfo ImageViewCreateInfo::ToVulkan(
    VkImage image,
    VkFormat imageFormat) const
{
    VkImageViewCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.pNext = nullptr;
    info.flags = 0;
    info.image = image;
    info.viewType = viewType;

    info.format = (format == VK_FORMAT_UNDEFINED) ? imageFormat : format;

    info.components = components;

    info.subresourceRange.aspectMask = aspectMask;
    info.subresourceRange.baseMipLevel = baseMipLevel;
    info.subresourceRange.levelCount = levelCount;
    info.subresourceRange.baseArrayLayer = baseArrayLayer;
    info.subresourceRange.layerCount = layerCount;

    return info;
}

void VulkanImageView::DestroyView(AllocatedImageView& view)
{
    if (!view.IsValid())
    {
        return; 
    }

    if (!IsInitialized())
    {
        ReportWarning("Destroying view but manager not initialized. 0x00010300"); 
    }

    if (m_device && m_device->IsInitialized())
    {
        vkDestroyImageView(
            m_device->GetDevice(),
            view.view,
            nullptr
        );
    }

    view.view = VK_NULL_HANDLE;
    view.type = VK_IMAGE_VIEW_TYPE_2D;
    view.format = VK_FORMAT_UNDEFINED;
    view.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view.baseMipLevel = 0;
    view.levelCount = 1;
    view.baseArrayLayer = 0;
    view.layerCount = 1;
}

