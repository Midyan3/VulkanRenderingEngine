#pragma once

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include <string>
#include "../VulkanInstance/VulkanInstance.h"
#include "../VulkanDevice/VulkanDevice.h"
#include "../VulkanSurface/VulkanSurface.h"
#include "../../DebugOutput/DubugOutput.h"

// Config
struct SwapchainConfig
{
    uint32_t preferredImageCount = 3;
    VkPresentModeKHR preferredPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
    VkFormat preferredFormat = VK_FORMAT_B8G8R8A8_SRGB;
    VkColorSpaceKHR preferredColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    bool clampImageCount = true;
};

// Resources
struct SwapchainImage
{
    VkImage image = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
    VkFramebuffer framebuffer = VK_NULL_HANDLE;
};

class VulkanSwapchain
{
public:
    VulkanSwapchain();
    ~VulkanSwapchain();

    // RAII
    VulkanSwapchain(const VulkanSwapchain&) = delete;
    VulkanSwapchain& operator=(const VulkanSwapchain&) = delete;
    VulkanSwapchain(VulkanSwapchain&& other) noexcept;
    VulkanSwapchain& operator=(VulkanSwapchain&& other) noexcept;

    // Lifecycle
    bool Initialize(std::shared_ptr<VulkanInstance> instance,
        std::shared_ptr<VulkanDevice> device,
        std::shared_ptr<VulkanSurface> surface,
        const SwapchainConfig& config = SwapchainConfig());
    void Cleanup();
    bool IsInitialized() const { return m_swapchain != VK_NULL_HANDLE; }

    // Recreate
    bool Recreate(uint32_t width, uint32_t height);
    bool Recreate();

    // Acquire/Present
    bool AcquireNextImage(uint32_t& outImageIndex, VkSemaphore signalSemaphore,
        VkFence fence = VK_NULL_HANDLE, uint64_t timeout = UINT64_MAX);
    bool PresentImage(uint32_t imageIndex, const std::vector<VkSemaphore>& waitSemaphores);

    // Properties
    VkSwapchainKHR GetSwapchain() const { return m_swapchain; }
    uint32_t GetImageCount() const { return static_cast<uint32_t>(m_images.size()); }
    VkExtent2D GetExtent() const { return m_extent; }
    VkFormat GetFormat() const { return m_format; }
    VkColorSpaceKHR GetColorSpace() const { return m_colorSpace; }
    VkPresentModeKHR GetPresentMode() const { return m_presentMode; }

    // Images
    const std::vector<SwapchainImage>& GetImages() const { return m_images; }
    const SwapchainImage& GetImage(uint32_t index) const;
    VkImageView GetImageView(uint32_t index) const;

    // Deps
    std::shared_ptr<VulkanInstance> GetInstance() const { return m_instance; }
    std::shared_ptr<VulkanDevice> GetDevice() const { return m_device; }
    std::shared_ptr<VulkanSurface> GetSurface() const { return m_surface; }

    // Debug
    std::string GetSwapchainInfo() const;

private:
    // Handles
    std::shared_ptr<VulkanInstance> m_instance;
    std::shared_ptr<VulkanDevice> m_device;
    std::shared_ptr<VulkanSurface> m_surface;
    VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;

    // State
    VkExtent2D m_extent = {};
    VkFormat m_format = VK_FORMAT_UNDEFINED;
    VkColorSpaceKHR m_colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    VkPresentModeKHR m_presentMode = VK_PRESENT_MODE_FIFO_KHR;
    uint32_t m_imageCount = 0;

    // Images
    std::vector<SwapchainImage> m_images;

    // Config
    SwapchainConfig m_config;

    // Logging
    static const Debug::DebugOutput DebugOut;

    // Creation
    bool CreateSwapchain(VkSwapchainKHR oldSwapchain = VK_NULL_HANDLE);
    bool CreateImageViews();
    void DestroyImageViews();

    // Selection
    VkSurfaceFormatKHR ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) const;
    VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR>& availableModes) const;
    VkExtent2D ChooseExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;
    uint32_t ChooseImageCount(const VkSurfaceCapabilitiesKHR& capabilities) const;

    // Validation
    bool ValidateDependencies() const;
    bool ValidateSwapchainSupport() const;

    // Errors
    void ReportError(const std::string& message) const
    {
        DebugOut.outputDebug("VulkanSwapchain Error: " + message);
    }

    void ReportWarning(const std::string& message) const
    {
        DebugOut.outputDebug("VulkanSwapchain Warning: " + message);
    }
};
