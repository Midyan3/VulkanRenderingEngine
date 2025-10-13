#pragma once

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include <string>
#include "../VulkanInstance/VulkanInstance.h"
#include "../VulkanDevice/VulkanDevice.h"
#include "../../DebugOutput/DubugOutput.h"

// Types
struct FrameSyncObjects
{
    VkFence inFlightFence = VK_NULL_HANDLE;

    bool IsValid() const
    {
        return inFlightFence != VK_NULL_HANDLE;
    }
};

struct ImageSyncObjects
{
    VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
    VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;
    bool IsValid() const
    {
        return imageAvailableSemaphore != VK_NULL_HANDLE &&
            renderFinishedSemaphore != VK_NULL_HANDLE;
    }
};

class VulkanSynchronization
{
public:
    VulkanSynchronization();
    ~VulkanSynchronization();

    // RAII
    VulkanSynchronization(const VulkanSynchronization&) = delete;
    VulkanSynchronization& operator=(const VulkanSynchronization&) = delete;
    VulkanSynchronization(VulkanSynchronization&& other) noexcept;
    VulkanSynchronization& operator=(VulkanSynchronization&& other) noexcept;

    // Lifecycle
    bool Initialize(std::shared_ptr<VulkanInstance> instance,
        std::shared_ptr<VulkanDevice> device,
        uint32_t maxFramesInFlight = 2,
        uint32_t imageCount = 2);
    void Cleanup();
    bool IsInitialized() const { return !m_frameSyncObjects.empty(); }

    // Access
    const FrameSyncObjects& GetFrameSync(uint32_t frameIndex) const;
    uint32_t GetMaxFramesInFlight() const { return static_cast<uint32_t>(m_frameSyncObjects.size()); }
    const ImageSyncObjects& GetImageSync(uint32_t imageIndex) const;

    // Fences
    bool WaitForFence(uint32_t frameIndex, uint64_t timeout = UINT64_MAX);
    bool ResetFence(uint32_t frameIndex);

    // Debug
    std::string GetSyncInfo() const;

private:
    std::shared_ptr<VulkanInstance> m_instance;
    std::shared_ptr<VulkanDevice> m_device;
    std::vector<FrameSyncObjects> m_frameSyncObjects;
    std::vector<ImageSyncObjects> m_imageSyncObjects;

    static const Debug::DebugOutput DebugOut;

    bool ValidateDependencies() const;
    bool CreateSyncObjects(uint32_t count);
    bool CreateImageSyncObjects(uint32_t count);

    void ReportError(const std::string& message) const
    {
        DebugOut.outputDebug("VulkanSynchronization Error: " + message);
    }

    void ReportWarning(const std::string& message) const
    {
        DebugOut.outputDebug("VulkanSynchronization Warning: " + message);
    }
};
