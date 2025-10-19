#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <memory>
#include <vector>
#include <string>
#include "../VulkanCommandBuffer/VulkanCommandBuffer.h"
#include "../VulkanInstance/VulkanInstance.h"
#include "../VulkanDevice/VulkanDevice.h"
#include "../../DebugOutput/DubugOutput.h"

// Forward declarations
struct VkCommandBuffer_T;
typedef struct VkCommandBuffer_T* VkCommandBuffer;

// Config
struct MemoryAllocationInfo
{
    VmaMemoryUsage usage = VMA_MEMORY_USAGE_UNKNOWN;
    VmaAllocationCreateFlags flags = 0;
    VkMemoryPropertyFlags requiredFlags = 0;
    VkMemoryPropertyFlags preferredFlags = 0;
    float priority = 0.5f;

    static MemoryAllocationInfo DeviceLocal()
    {
        MemoryAllocationInfo info;
        info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        info.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        return info;
    }

    static MemoryAllocationInfo HostVisible()
    {
        MemoryAllocationInfo info;
        info.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        info.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        return info;
    }

    static MemoryAllocationInfo Readback()
    {
        MemoryAllocationInfo info;
        info.usage = VMA_MEMORY_USAGE_GPU_TO_CPU;
        info.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        info.preferredFlags = VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
        return info;
    }

    static MemoryAllocationInfo Staging()
    {
        MemoryAllocationInfo info;
        info.usage = VMA_MEMORY_USAGE_CPU_ONLY;
        info.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        return info;
    }
};

// Resources
struct AllocatedBuffer
{
    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VmaAllocationInfo allocationInfo = {};
    size_t size = 0;
    VkBufferUsageFlags usage = 0;
    bool isPersistentlyMapped = false;
    void* mappedData = nullptr;
    VkIndexType indexType = VK_INDEX_TYPE_UINT32;

    bool IsValid() const { return buffer != VK_NULL_HANDLE && allocation != VK_NULL_HANDLE; }
};

struct AllocatedImage
{
    VkImage image = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VmaAllocationInfo allocationInfo = {};
    VkExtent3D extent = {};
    VkFormat format = VK_FORMAT_UNDEFINED;
    VkImageUsageFlags usage = 0;
    uint32_t mipLevels = 1;
    uint32_t arrayLayers = 1;
    VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;

    bool IsValid() const { return image != VK_NULL_HANDLE && allocation != VK_NULL_HANDLE; }
};

// Stats
struct MemoryStatistics
{
    size_t totalAllocatedBytes = 0;
    size_t totalUsedBytes = 0;
    uint32_t allocationCount = 0;
    uint32_t unusedRangeCount = 0;

    std::vector<VmaBudget> heapBudgets;

    size_t deviceLocalBytes = 0;
    size_t hostVisibleBytes = 0;
    size_t stagingBytes = 0;
};

class VulkanMemoryAllocator
{
public:
    VulkanMemoryAllocator();
    ~VulkanMemoryAllocator();

    // RAII
    VulkanMemoryAllocator(const VulkanMemoryAllocator&) = delete;
    VulkanMemoryAllocator& operator=(const VulkanMemoryAllocator&) = delete;
    //VulkanMemoryAllocator(VulkanMemoryAllocator&& other) noexcept;
    //VulkanMemoryAllocator& operator=(VulkanMemoryAllocator&& other) noexcept;

    // Lifecycle
    bool Initialize(std::shared_ptr<VulkanInstance> instance,
        std::shared_ptr<VulkanDevice> device,
        VkDeviceSize preferredLargeHeapBlockSize = 0);
    void Cleanup();
    bool IsInitialized() const { return m_allocator != VK_NULL_HANDLE; }

    // Buffers
    bool CreateBuffer(size_t size, VkBufferUsageFlags usage,
        const MemoryAllocationInfo& memInfo, AllocatedBuffer& outBuffer);
    void DestroyBuffer(AllocatedBuffer& buffer);

    // Mapping
    bool MapMemory(AllocatedBuffer& buffer);
    void UnmapMemory(AllocatedBuffer& buffer);
    bool FlushMappedMemory(const AllocatedBuffer& buffer, size_t offset = 0, size_t size = VK_WHOLE_SIZE);
    bool InvalidateMappedMemory(const AllocatedBuffer& buffer, size_t offset = 0, size_t size = VK_WHOLE_SIZE);

    // Convenience
    bool CreateVertexBuffer(
        VulkanCommandBuffer* commandBuffer,
        const void* vertices,
        size_t size,
        AllocatedBuffer& outBuffer);
    bool CreateIndexBuffer(
        VulkanCommandBuffer* commandBuffer,
        const void* indices,
        size_t size,
        VkIndexType indexType,
        AllocatedBuffer& outBuffer);
    bool CreateUniformBuffer(size_t size, AllocatedBuffer& outBuffer, bool persistentlyMapped = true);

    // Transfers
    bool UploadDataToBuffer(AllocatedBuffer& buffer, const void* data, size_t size, size_t offset = 0);
    bool CopyBuffer(VkCommandBuffer commandBuffer, const AllocatedBuffer& src,
        AllocatedBuffer& dst, size_t size, size_t srcOffset = 0, size_t dstOffset = 0);

    // Getters
    VmaAllocator GetAllocator() const { return m_allocator; }
    std::shared_ptr<VulkanDevice> GetDevice() const { return m_device; }
    std::shared_ptr<VulkanInstance> GetInstance() const { return m_instance; }

private:
    // Handles
    std::shared_ptr<VulkanInstance> m_instance;
    std::shared_ptr<VulkanDevice> m_device;
    VmaAllocator m_allocator = VK_NULL_HANDLE;

    // State
    VkDeviceSize m_preferredLargeHeapBlockSize = 0;
    uint32_t m_currentFrameIndex = 0;

    // Stats
    mutable std::vector<VmaBudget> m_lastBudgetSnapshot;

    // Logging
    static const Debug::DebugOutput DebugOut;

    // Errors
    void ReportError(const std::string& message) const
    {
        DebugOut.outputDebug("VulkanMemoryAllocator Error: " + message);
    }

    void ReportWarning(const std::string& message) const
    {
        DebugOut.outputDebug("VulkanMemoryAllocator Warning: " + message);
    }
};
