#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <optional>
#include <unordered_set>
#include "../VulkanInstance/VulkanInstance.h"
#include "../../DebugOutput/DubugOutput.h"

// Types
typedef struct VkSurfaceKHR_T* VkSurfaceKHR;

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;
    std::optional<uint32_t> computeFamily;
    std::optional<uint32_t> transferFamily;

    bool IsComplete() const {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }

    bool HasSeparateTransfer() const {
        return transferFamily.has_value() &&
            transferFamily != graphicsFamily;
    }
};

// Scoring
struct DeviceScore {
    int totalScore = 0;
    bool suitable = false;
    std::string deviceName;
    VkPhysicalDeviceType deviceType;

    int typeScore = 0;
    int memoryScore = 0;
    int queueScore = 0;
    int extensionScore = 0;
};

class VulkanDevice {
public:
    VulkanDevice();
    ~VulkanDevice();

    // Non-copyable, movable
    VulkanDevice(const VulkanDevice&) = delete;
    VulkanDevice& operator=(const VulkanDevice&) = delete;
    VulkanDevice(VulkanDevice&& other) noexcept;
    VulkanDevice& operator=(VulkanDevice&& other) noexcept;

    // Core initialization
    bool Initialize(std::shared_ptr<VulkanInstance> instance,
        VkSurfaceKHR surface = VK_NULL_HANDLE,
        const char* preferedDevice = nullptr);
    void Cleanup();
    bool IsInitialized() const { return m_device != VK_NULL_HANDLE; }

    // Device selection control
    std::vector<DeviceScore> GetAvailableDeviceScores(VkSurfaceKHR surface = VK_NULL_HANDLE) const;

    // Core getters
    VkDevice GetDevice() const { return m_device; }
    VkPhysicalDevice GetPhysicalDevice() const { return m_physicalDevice; }
    VkQueue GetGraphicsQueue() const { return m_graphicsQueue; }
    VkQueue GetPresentQueue() const { return m_presentQueue; }
    VkQueue GetComputeQueue() const { return m_computeQueue; }
    VkQueue GetTransferQueue() const { return m_transferQueue; }

    // Queue family information
    const QueueFamilyIndices& GetQueueFamilyIndices() const { return m_queueFamilyIndices; }
    uint32_t GetGraphicsQueueFamily() const { return m_queueFamilyIndices.graphicsFamily.value(); }
    uint32_t GetPresentQueueFamily() const { return m_queueFamilyIndices.presentFamily.value(); }

    // Device properties and capabilities
    const VkPhysicalDeviceProperties& GetDeviceProperties() const { return m_deviceProperties; }
    const VkPhysicalDeviceFeatures& GetDeviceFeatures() const { return m_deviceFeatures; }
    const VkPhysicalDeviceMemoryProperties& GetMemoryProperties() const { return m_memoryProperties; }

    // Extension support
    bool IsExtensionSupported(const std::string& extensionName) const;
    const std::vector<std::string>& GetSupportedExtensions() const { return m_supportedExtensions; }
    const std::vector<const char*>& GetEnabledExtensions() const { return m_enabledExtensions; }

    // Surface compatibility
    bool IsSurfaceSupported(VkSurfaceKHR surface) const;
    VkSurfaceCapabilitiesKHR GetSurfaceCapabilities(VkSurfaceKHR surface) const;
    std::vector<VkSurfaceFormatKHR> GetSurfaceFormats(VkSurfaceKHR surface) const;
    std::vector<VkPresentModeKHR> GetPresentModes(VkSurfaceKHR surface) const;

    bool FindDepthFormat(); 
    VkFormat GetDepthFormat(); 

    // Sync
    void WaitIdle() const;

    // Debug
    std::string GetDeviceInfo() const;

private:
    // Core Vulkan handles
    std::shared_ptr<VulkanInstance> m_instance;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;

    VkFormat m_prefferedDepth; 

    // Queues
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkQueue m_presentQueue = VK_NULL_HANDLE;
    VkQueue m_computeQueue = VK_NULL_HANDLE;
    VkQueue m_transferQueue = VK_NULL_HANDLE;

    // Queue families
    QueueFamilyIndices m_queueFamilyIndices;

    // Properties
    VkPhysicalDeviceProperties m_deviceProperties = {};
    VkPhysicalDeviceFeatures m_deviceFeatures = {};
    VkPhysicalDeviceMemoryProperties m_memoryProperties = {};

    // Extensions
    std::vector<std::string> m_supportedExtensions;
    std::vector<const char*> m_enabledExtensions;
    std::vector<std::string> m_requiredExtensions;
    std::vector<std::string> m_optionalExtensions;

    // Logging
    static const Debug::DebugOutput DebugOut;

    // Selection & init
    DeviceScore ScoreDevice(const VkPhysicalDevice& device, const VkSurfaceKHR& surface) const;
    QueueFamilyIndices FindQueueFamilies(const VkPhysicalDevice& device, const VkSurfaceKHR& surface) const;

    // Device creation
    bool CreateLogicalDevice();
    void SetupRequiredExtensions();

    // Extension checks
    bool CheckDeviceExtensionSupport(VkPhysicalDevice device) const;
    void QuerySupportedExtensions(VkPhysicalDevice device);

    // Queue management
    void RetrieveQueues();

    // Property querying
    void QueryDeviceProperties();

    // Utilities
    int GetDeviceTypeScore(const VkPhysicalDeviceType& deviceType) const;
    int GetMemoryScore(const VkPhysicalDeviceMemoryProperties& memProps) const;
    int GetQueueScore(const QueueFamilyIndices& queueFamilies) const;
    std::string DeviceTypeToString(VkPhysicalDeviceType type) const;

    // Errors
    void ReportError(const std::string& message) const {
        DebugOut.outputDebug("VulkanDevice Error: " + message);
    }
};
