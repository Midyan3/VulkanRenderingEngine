#pragma once

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include <functional>
#include "../VulkanInstance/VulkanInstance.h"
#include "../VulkanDevice/VulkanDevice.h"
#include "../../DebugOutput/DubugOutput.h"

// Types
enum class CommandBufferState
{
    Initial,
    Recording,
    Executable,
    Pending,
    Invalid
};

// Config
struct CommandBufferConfig
{
    VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    bool oneTimeSubmit = false;
};

class VulkanCommandBuffer
{
public:
    VulkanCommandBuffer();
    ~VulkanCommandBuffer();

    // RAII
    VulkanCommandBuffer(const VulkanCommandBuffer&) = delete;
    VulkanCommandBuffer& operator=(const VulkanCommandBuffer&) = delete;
    VulkanCommandBuffer(VulkanCommandBuffer&& other) noexcept;
    VulkanCommandBuffer& operator=(VulkanCommandBuffer&& other) noexcept;

    // Lifecycle
    bool Initialize(std::shared_ptr<VulkanInstance> instance,
        std::shared_ptr<VulkanDevice> device,
        uint32_t queueFamilyIndex);
    void Cleanup();
    bool IsInitialized() const { return m_commandPool != VK_NULL_HANDLE; }

    // Allocation
    VkCommandBuffer AllocateCommandBuffer(const CommandBufferConfig& config = CommandBufferConfig());
    std::vector<VkCommandBuffer> AllocateCommandBuffers(uint32_t count,
        const CommandBufferConfig& config = CommandBufferConfig());
    void FreeCommandBuffer(VkCommandBuffer commandBuffer);
    void FreeCommandBuffers(const std::vector<VkCommandBuffer>& commandBuffers);

    // Recording
    bool BeginRecording(VkCommandBuffer commandBuffer,
        VkCommandBufferUsageFlags usageFlags = 0);
    bool EndRecording(VkCommandBuffer commandBuffer);
    bool Reset(VkCommandBuffer commandBuffer,
        VkCommandBufferResetFlags flags = 0);

    // Single-use helpers
    VkCommandBuffer BeginSingleTimeCommands();
    bool EndSingleTimeCommands(VkCommandBuffer commandBuffer);

    // Submission
    bool Submit(VkCommandBuffer commandBuffer,
        VkQueue queue,
        const std::vector<VkSemaphore>& waitSemaphores = {},
        const std::vector<VkPipelineStageFlags>& waitStages = {},
        const std::vector<VkSemaphore>& signalSemaphores = {},
        VkFence fence = VK_NULL_HANDLE);

    bool SubmitMultiple(const std::vector<VkCommandBuffer>& commandBuffers,
        VkQueue queue,
        const std::vector<VkSemaphore>& waitSemaphores = {},
        const std::vector<VkPipelineStageFlags>& waitStages = {},
        const std::vector<VkSemaphore>& signalSemaphores = {},
        VkFence fence = VK_NULL_HANDLE);

    // Pool ops
    bool ResetPool(VkCommandPoolResetFlags flags = 0);

    // Getters
    VkCommandPool GetCommandPool() const { return m_commandPool; }
    uint32_t GetQueueFamilyIndex() const { return m_queueFamilyIndex; }
    std::shared_ptr<VulkanDevice> GetDevice() const { return m_device; }

    // Debug
    std::string GetCommandBufferInfo() const;

private:
    // Handles & deps
    std::shared_ptr<VulkanInstance> m_instance;
    std::shared_ptr<VulkanDevice> m_device;
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    uint32_t m_queueFamilyIndex = 0;

    // Tracking
    std::vector<VkCommandBuffer> m_allocatedBuffers;

    // Logging
    static const Debug::DebugOutput DebugOut;

    // Internals
    bool CreateCommandPool();
    bool ValidateDependencies() const;

    // Errors
    void ReportError(const std::string& message) const
    {
        DebugOut.outputDebug("VulkanCommandBuffer Error: " + message);
    }

    void ReportWarning(const std::string& message) const
    {
        DebugOut.outputDebug("VulkanCommandBuffer Warning: " + message);
    }
};
