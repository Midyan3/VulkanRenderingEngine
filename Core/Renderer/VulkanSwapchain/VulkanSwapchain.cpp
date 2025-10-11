#include "VulkanSwapchain.h"
#include <algorithm>
#include <string>
#include <cmath>

const Debug::DebugOutput VulkanSwapchain::DebugOut;

VulkanSwapchain::VulkanSwapchain()
{
    m_swapchain = VK_NULL_HANDLE;
    m_extent = { 0, 0 };
    m_format = VK_FORMAT_UNDEFINED;
    m_colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    m_presentMode = VK_PRESENT_MODE_FIFO_KHR;
    m_imageCount = 0;
}

VulkanSwapchain::~VulkanSwapchain()
{
    Cleanup();
}

void VulkanSwapchain::DestroyImageViews()
{
    if (m_device && m_device->IsInitialized())
    {
        for (auto& image : m_images)
        {
            if (image.imageView != VK_NULL_HANDLE)
            {
                vkDestroyImageView(m_device->GetDevice(), image.imageView, nullptr);
                image.imageView = VK_NULL_HANDLE;
            }
        }
    }
}

void VulkanSwapchain::Cleanup()
{
    DestroyImageViews();

    if (m_swapchain != VK_NULL_HANDLE)
    {
        if (m_device && m_device->IsInitialized())
        {
            vkDestroySwapchainKHR(m_device->GetDevice(), m_swapchain, nullptr);
        }
        m_swapchain = VK_NULL_HANDLE;
    }

    m_images.clear();

    m_extent = { 0, 0 };
    m_format = VK_FORMAT_UNDEFINED;
    m_colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    m_presentMode = VK_PRESENT_MODE_FIFO_KHR;
    m_imageCount = 0;

    m_instance.reset();
    m_device.reset();
    m_surface.reset();
}

bool VulkanSwapchain::ValidateDependencies() const
{
    if (!m_instance)
    {
        ReportError("Check instance. 0x00005000");
        return false;
    }

    if (!m_device)
    {
        ReportError("Check device. 0x00005010");
        return false;
    }

    if (!m_surface)
    {
        ReportError("Check surface. 0x00005020");
        return false;
    }

    if (!m_instance->IsInitialized())
    {
        ReportError("Check instance. 0x00005005");
        return false;
    }

    if (!m_device->IsInitialized())
    {
        ReportError("Check device. 0x00005015");
        return false;
    }

    if (!m_surface->IsInitialized())
    {
        ReportError("Check surface. 0x00005025");
        return false;
    }

    return true;
}


uint32_t VulkanSwapchain::ChooseImageCount(const VkSurfaceCapabilitiesKHR& capabilities) const
{
    uint32_t imageCount = m_config.preferredImageCount;

    if (imageCount < capabilities.minImageCount)
        imageCount = capabilities.minImageCount;

    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
        imageCount = capabilities.maxImageCount;

    return imageCount;
}

VkSurfaceFormatKHR VulkanSwapchain::ChooseSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR>& availableFormats) const
{
    for (const auto& format : availableFormats)
    {
        if (format.format == m_config.preferredFormat &&
            format.colorSpace == m_config.preferredColorSpace)
        {
            return format;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR VulkanSwapchain::ChoosePresentMode(
    const std::vector<VkPresentModeKHR>& availableModes) const
{

    for (const auto& mode : availableModes)
    {
        if (mode == m_config.preferredPresentMode)
            return mode;
    }
    
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanSwapchain::ChooseExtent(const VkSurfaceCapabilitiesKHR& capabilities) const
{
    if (capabilities.currentExtent.width != UINT32_MAX)
        return capabilities.currentExtent;

    uint32_t width = static_cast<uint32_t>(m_surface->GetWindow()->GetWidth());
    uint32_t height = static_cast<uint32_t>(m_surface->GetWindow()->GetHeight());

    width = std::clamp(width,
        capabilities.minImageExtent.width,
        capabilities.maxImageExtent.width);

    height = std::clamp(height,
        capabilities.minImageExtent.height,
        capabilities.maxImageExtent.height);

    return VkExtent2D{ width, height };
}

bool VulkanSwapchain::ValidateSwapchainSupport() const
{
    std::vector<VkSurfaceFormatKHR> formats = m_surface->GetFormats();
    if (formats.empty())
    {
        ReportError("No surface formats available. 0x00005130");
        return false;
    }

    std::vector<VkPresentModeKHR> modes = m_surface->GetPresentModes();
    if (modes.empty())
    {
        ReportError("No present modes available. 0x00005140");
        return false;
    }

    return true;
}

bool VulkanSwapchain::CreateImageViews()
{
    for (auto& swapchainImage : m_images)
    {
        VkImageViewCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swapchainImage.image;
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = m_format;

        // Keep's the Default mapping here
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        VkResult result = vkCreateImageView(m_device->GetDevice(), &createInfo, nullptr, &swapchainImage.imageView);
        if (result != VK_SUCCESS)
        {
            ReportError("Failed to create image view. 0x00005300");
            return false;
        }
    }

    return true;
}

bool VulkanSwapchain::CreateSwapchain(VkSwapchainKHR oldSwapchain)
{

    try 
    {
        VkSurfaceCapabilitiesKHR capabilities = m_surface->GetCapabilities();
        std::vector<VkSurfaceFormatKHR> formats = m_surface->GetFormats();
        std::vector<VkPresentModeKHR> presentModes = m_surface->GetPresentModes();

        // Choose swapchain settings
        VkSurfaceFormatKHR surfaceFormat = ChooseSurfaceFormat(formats);
        VkPresentModeKHR presentMode = ChoosePresentMode(presentModes);
        VkExtent2D extent = ChooseExtent(capabilities);
        uint32_t imageCount = ChooseImageCount(capabilities);

        VkSwapchainCreateInfoKHR createInfo = {};

        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = m_surface->GetSurface();
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        createInfo.presentMode = presentMode;
        createInfo.imageExtent = extent;
        createInfo.preTransform = capabilities.currentTransform;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = oldSwapchain;
        VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

        // Doing because not always guaranteed to be VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR. TLDR when re-looking back at code   
        if (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
        {
            compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        }
        else if (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR)
        {
            compositeAlpha = VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;
        }
        else if (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR)
        {
            compositeAlpha = VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR;
        }
        else
        {
            compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
        }

        createInfo.compositeAlpha = compositeAlpha;

        uint32_t queueFamilyIndices[] = {
        m_device->GetQueueFamilyIndices().graphicsFamily.value(),
        m_device->GetQueueFamilyIndices().presentFamily.value()
        };

        if (queueFamilyIndices[0] != queueFamilyIndices[1])
        {
            // Different queues so needs concurrent mode
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else
        {
            // Same queue so exclusive mode = better performance
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;
            createInfo.pQueueFamilyIndices = nullptr;
        }

        VkResult result = vkCreateSwapchainKHR(m_device->GetDevice(), &createInfo, nullptr, &m_swapchain);
        if (result != VK_SUCCESS)
        {
            ReportError("Failed to create Swapchain. 0x00AF5200");
            return false;
        }

        uint32_t acutalImageCount = 0;

        result = vkGetSwapchainImagesKHR(m_device->GetDevice(), m_swapchain, &acutalImageCount, nullptr);
        if (result != VK_SUCCESS)
        {
            ReportError("Failed to retireve image count. 0x000F5210");
            return false;
        }

        std::vector<VkImage> swapchainImages(acutalImageCount);
        result = vkGetSwapchainImagesKHR(m_device->GetDevice(), m_swapchain, &acutalImageCount, swapchainImages.data());

        if (result != VK_SUCCESS)
        {
            ReportError("Failed to retireve swapchainImages. 0x000F5220");
            return false;
        }

        for (auto& swapchainImage : swapchainImages)
        {
            SwapchainImage tmp;
            tmp.image = swapchainImage;
            m_images.push_back(std::move(tmp));
        }

        m_extent = extent;
        m_format = surfaceFormat.format;
        m_colorSpace = surfaceFormat.colorSpace;
        m_presentMode = presentMode;
        m_imageCount = static_cast<uint32_t>(m_images.size());

        bool q_result = CreateImageViews();

        if (!q_result)
        {
            ReportError("Failed to create image views. 0x000F5230");
            return false;
        }


        return true;
    }
    catch (const std::exception& e)
    {
        ReportError("Error creating Swapchain. 0X000F5200");
        ReportError(e.what());
        return false;
    }
    catch (...)
    {
        ReportError("Error creating Swapchain. 0X000F5200");
        return false;
    }
}

bool VulkanSwapchain::Initialize(
    std::shared_ptr<VulkanInstance> instance,
    std::shared_ptr<VulkanDevice> device,
    std::shared_ptr<VulkanSurface> surface,
    const SwapchainConfig& config)
{
    try
    {
        m_instance = instance;
        m_device = device;
        m_surface = surface;
        m_config = config;

       return ValidateDependencies() && ValidateSwapchainSupport() && CreateSwapchain(VK_NULL_HANDLE);
    }
    catch (const std::exception& e)
    {
        ReportError("Exception during swapchain initialization: " + std::string(e.what()));
        return false;
    }
    catch (...)
    {
        ReportError("Unknown exception during swapchain initialization. 0x000F5400");
        return false;
    }
}

bool VulkanSwapchain::AcquireNextImage(
    uint32_t& outImageIndex,
    VkSemaphore signalSemaphore,
    VkFence fence,
    uint64_t timeout)
{
    if (!IsInitialized())
    {
        ReportError("Swapchain not initialized. 0x00005500");
        return false;
    }

    VkResult result = vkAcquireNextImageKHR(
        m_device->GetDevice(),
        m_swapchain,
        timeout,
        signalSemaphore,
        fence,
        &outImageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        // Swapchain is out of date (window resized), caller must recreate. <-Important for later
        ReportWarning("Swapchain out of date. 0x00005505");
        return false;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        ReportError("Failed to acquire next image. 0x00005510");
        return false;
    }

    return true;
}

bool VulkanSwapchain::PresentImage(
    uint32_t imageIndex,
    const std::vector<VkSemaphore>& waitSemaphores)
{
    if (!IsInitialized())
    {
        ReportError("Swapchain not initialized. 0x00005600");
        return false;
    }

    if (imageIndex >= m_images.size())
    {
        ReportError("Image index out of bounds. 0x00005610");
        return false;
    }

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
    presentInfo.pWaitSemaphores = waitSemaphores.data();
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &m_swapchain;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;

    VkResult result = vkQueuePresentKHR(m_device->GetPresentQueue(), &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        ReportWarning("Swapchain out of date. 0x00005620");
        return false;
    }

    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        ReportError("Failed to present image. 0x00005630");
        return false;
    }

    return true;
}

bool VulkanSwapchain::Recreate(uint32_t width, uint32_t height)
{
    // Note: width and height are ignored - CreateSwapchain queries surface automatically
    // They're here for API clarity (user knows they're recreating for a new size)

    m_device->WaitIdle();

    VkSwapchainKHR oldSwapchain = m_swapchain;

    DestroyImageViews();

    if (!CreateSwapchain(oldSwapchain))
    {
        ReportError("Failed to recreate swapchain. 0x00005700");
        return false;
    }

    return true;
}

bool VulkanSwapchain::Recreate()
{
    // surface will provide current extent so me putting 0,0 would not be used anyways.
    return Recreate(0, 0);  // Width/height ignored anyway
}

const SwapchainImage& VulkanSwapchain::GetImage(uint32_t index) const
{
    static const SwapchainImage invalidImage = {};

    if (index >= m_images.size())
    {
        ReportError("Image index out of bounds. 0x00005800");
        return invalidImage;
    }

    return m_images[index];
}

VkImageView VulkanSwapchain::GetImageView(uint32_t index) const
{
    
    if (index < m_images.size())
    {
        return m_images[index].imageView;
    }

    return VkImageView{};
}

std::string VulkanSwapchain::GetSwapchainInfo() const
{
    if (!IsInitialized())
        return "Swapchain not initialized";

    std::string info = "VulkanSwapchain Info:\n";

    info += "  Swapchain Handle: " + std::to_string(reinterpret_cast<uint64_t>(m_swapchain)) + "\n";

    info += "  Image Count: " + std::to_string(m_imageCount) + "\n";

    info += "  Extent: " + std::to_string(m_extent.width) + "x" + std::to_string(m_extent.height) + "\n";

    info += "  Format: " + std::to_string(m_format) + "\n";

    info += "  Present Mode: " + std::to_string(m_presentMode) + "\n";

    return info;
}