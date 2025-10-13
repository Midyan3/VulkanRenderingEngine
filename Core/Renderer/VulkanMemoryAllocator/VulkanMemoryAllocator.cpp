#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include "VulkanMemoryAllocator.h"
#include <memory>

VulkanMemoryAllocator::VulkanMemoryAllocator()
{
	m_preferredLargeHeapBlockSize = 0;
	m_currentFrameIndex = 0; 
}

void VulkanMemoryAllocator::Cleanup()
{
	if (m_allocator != VK_NULL_HANDLE)
	{
		vmaDestroyAllocator(m_allocator);
		m_allocator = VK_NULL_HANDLE;
	}

	m_device.reset(); 
	m_instance.reset(); 
	m_preferredLargeHeapBlockSize = 0;
	m_currentFrameIndex = 0;
}

VulkanMemoryAllocator::~VulkanMemoryAllocator()
{
	Cleanup(); 
}

bool VulkanMemoryAllocator::FlushMappedMemory(
	const AllocatedBuffer& buffer, size_t offset, size_t size)
{
	if (!buffer.IsValid()) 
		return false;

	vmaFlushAllocation(m_allocator, buffer.allocation,
		static_cast<VkDeviceSize>(offset),
		size == VK_WHOLE_SIZE ? VK_WHOLE_SIZE : static_cast<VkDeviceSize>(size));
	
	return true;
}


bool VulkanMemoryAllocator::InvalidateMappedMemory(
	const AllocatedBuffer& buffer, size_t offset, size_t size)
{
	if (!buffer.IsValid())
		return false;

	VkResult result = vmaInvalidateAllocation(m_allocator, buffer.allocation,
		static_cast<VkDeviceSize>(offset),
		size == VK_WHOLE_SIZE ? VK_WHOLE_SIZE : static_cast<VkDeviceSize>(size));

	if (result != VK_SUCCESS)
	{
		ReportError("Failed to flush buffer memory. 0x00003510");
		return false;
	}
	
	return true;
}


bool VulkanMemoryAllocator::MapMemory(AllocatedBuffer& buffer)
{
	if (!buffer.IsValid())
	{
		ReportError("Cannot map invalid buffer. 0x00003210");
		return false;
	}

	if (buffer.mappedData)
	{
		ReportWarning("Buffer is already mapped. 0x00003220");
		return true; 
	}

	VkResult result = vmaMapMemory(m_allocator, buffer.allocation, &buffer.mappedData); 

	if (result != VK_SUCCESS)
	{
		ReportError("Failed to map buffer memory. 0x00003230");
		buffer.mappedData = nullptr;
		return false;
	}

	return true;

	vmaUnmapMemory(m_allocator, buffer.allocation); 

	buffer.mappedData = VK_NULL_HANDLE; 
	buffer.isPersistentlyMapped = false;


}

bool VulkanMemoryAllocator::CreateVertexBuffer(
	VulkanCommandBuffer* commandBuffer,
	const void* vertices,
	size_t size,
	AllocatedBuffer& outBuffer)
{
	if (!commandBuffer || !commandBuffer->IsInitialized())
	{
		ReportError("Invalid command buffer system. 0x00003301");
		return false;
	}

	AllocatedBuffer stagingBuffer;
	if (!CreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		MemoryAllocationInfo::Staging(), stagingBuffer))
	{
		ReportError("Failed to create vertex staging buffer. 0x00003300");
		return false;
	}

	if (!MapMemory(stagingBuffer))
	{
		DestroyBuffer(stagingBuffer);
		ReportError("Failed to map vertex staging buffer. 0x00003310");
		return false;
	}

	memcpy(stagingBuffer.mappedData, vertices, size);
	UnmapMemory(stagingBuffer);

	if (!CreateBuffer(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		MemoryAllocationInfo::DeviceLocal(), outBuffer))
	{
		DestroyBuffer(stagingBuffer);
		ReportError("Failed to create vertex buffer. 0x00003315");
		return false;
	}

	VkCommandBuffer cmd = commandBuffer->BeginSingleTimeCommands();
	CopyBuffer(cmd, stagingBuffer, outBuffer, size);
	commandBuffer->EndSingleTimeCommands(cmd);  // This does everything

	DestroyBuffer(stagingBuffer);
	return true;
}

bool VulkanMemoryAllocator::CreateIndexBuffer(
	VulkanCommandBuffer* commandBuffer,
	const void* indices,
	size_t size,
	VkIndexType indexType,
	AllocatedBuffer& outBuffer)
{
	if (!commandBuffer || !commandBuffer->IsInitialized())
	{
		ReportError("Invalid command buffer system. 0x00003301");
		return false;
	}

	AllocatedBuffer stagingBuffer;
	if (!CreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		MemoryAllocationInfo::Staging(), stagingBuffer))
	{
		ReportError("Failed to create vertex staging buffer. 0x00003300");
		return false;
	}

	if (!MapMemory(stagingBuffer))
	{
		DestroyBuffer(stagingBuffer);
		ReportError("Failed to map vertex staging buffer. 0x00003310");
		return false;
	}

	memcpy(stagingBuffer.mappedData, indices, size);
	UnmapMemory(stagingBuffer);

	if (!CreateBuffer(size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		MemoryAllocationInfo::DeviceLocal(), outBuffer))
	{
		DestroyBuffer(stagingBuffer);
		ReportError("Failed to create vertex buffer. 0x00003315");
		return false;
	}

	VkCommandBuffer cmd = commandBuffer->BeginSingleTimeCommands();
	CopyBuffer(cmd, stagingBuffer, outBuffer, size);
	commandBuffer->EndSingleTimeCommands(cmd);  // This does everything

	DestroyBuffer(stagingBuffer);
	return true;
}

bool VulkanMemoryAllocator::CopyBuffer(
	VkCommandBuffer commandBuffer,
	const AllocatedBuffer& src,
	AllocatedBuffer& dst,
	size_t size,
	size_t srcOffset,
	size_t dstOffset)
{

	if (commandBuffer == VK_NULL_HANDLE)
	{
		ReportError("Failed to copy buffer, invalid command buffer. 0x00003700");
		return false;
	}

	if (!src.IsValid() || !dst.IsValid())
	{
		ReportError("Failed to copy buffer, either src or dst is invalid. 0x00003710");
		return false;
	}

	if (size + srcOffset > src.size || size + dstOffset > dst.size)
	{
		ReportError("Failed to copy buffer, offset out of bound. 0x000B3720");
		return false;
	}

	VkBufferCopy copy = {};

	copy.srcOffset = srcOffset; 
	copy.dstOffset = dstOffset;
	copy.size = size;

	vkCmdCopyBuffer(commandBuffer, src.buffer, dst.buffer, 1, &copy);

	return true;
}

bool VulkanMemoryAllocator::CreateUniformBuffer(
	size_t size,
	AllocatedBuffer& outBuffer,
	bool persistentlyMapped
)
{
	if (!size)
	{
		ReportError("Cannot create uniform buffer with size 0. 0x00003605");
		return false;
	}

	if (!CreateBuffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		MemoryAllocationInfo::HostVisible(), outBuffer))
	{
		ReportError("Failed to create uniform buffer. 0x000030B0");
		return false;
	}


	if (persistentlyMapped){
		if (!MapMemory(outBuffer))
		{
			DestroyBuffer(outBuffer);
			ReportError("Failed to persistently map uniform buffer. 0x00FB3610");
			return false;
		}
		outBuffer.isPersistentlyMapped = true;
	}
	return true;
}

bool VulkanMemoryAllocator::UploadDataToBuffer(
	AllocatedBuffer& buffer,
	const void* data,
	size_t size,
	size_t offset
)
{
	if (!buffer.IsValid())
	{
		ReportError("Cannot upload to invalid buffer. 0x00003410");
		return false;
	}

	if (!data || size == 0) {
		if (!data && size != 0) {
			ReportError("Null data with non-zero size. 0x00003415");
			return false;
		}
		return true;
	}

	if (offset + size > buffer.size)
	{
		ReportError("Upload would exceed buffer size. 0x00003420");
		return false;
	}

	if (!MapMemory(buffer))
		return false; 

	memcpy(static_cast<char*>(buffer.mappedData) + offset, data, size);

	FlushMappedMemory(buffer, offset, size); 

	if(!buffer.isPersistentlyMapped)
		UnmapMemory(buffer);

	return true;
}

void VulkanMemoryAllocator::UnmapMemory(AllocatedBuffer& buffer)
{
	if (!buffer.IsValid())
		return;

	if (!buffer.mappedData)
		return;

	vmaUnmapMemory(m_allocator, buffer.allocation); 

	buffer.mappedData = nullptr;
	buffer.isPersistentlyMapped = false;
}

void VulkanMemoryAllocator::DestroyBuffer(AllocatedBuffer& buffer)
{
	if (!buffer.IsValid())
		return;

	if (buffer.isPersistentlyMapped && buffer.mappedData)
		UnmapMemory(buffer);

	vmaDestroyBuffer(m_allocator, buffer.buffer, buffer.allocation);

	buffer.buffer = VK_NULL_HANDLE; 
	buffer.allocation = VK_NULL_HANDLE;
	buffer.size = 0; 
	buffer.usage = 0; 
	buffer.mappedData = nullptr;
	buffer.allocationInfo = {};
}

bool VulkanMemoryAllocator::CreateBuffer(
	size_t size, 
	VkBufferUsageFlags usage,
	const MemoryAllocationInfo& memInfo, 
	AllocatedBuffer& outBuffer
)
{
	VkBufferCreateInfo bufferInfo = {}; 
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size; 
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; 

	VmaAllocationCreateInfo allocInfo = {}; 
	allocInfo.usage = memInfo.usage;
	allocInfo.flags = memInfo.flags; 
	allocInfo.requiredFlags = memInfo.requiredFlags;
	allocInfo.preferredFlags = memInfo.preferredFlags; 
	allocInfo.priority = memInfo.priority; 

	VkResult result = vmaCreateBuffer(
		m_allocator,
		&bufferInfo,
		&allocInfo,
		&outBuffer.buffer,
		&outBuffer.allocation,
		&outBuffer.allocationInfo); 
	
	if (result != VK_SUCCESS)
	{
		ReportError("Failed to create buffer. 0x00003110");
		return false;
	}

	outBuffer.size = size; 
	outBuffer.usage = usage; 

	return true;

}

bool VulkanMemoryAllocator::Initialize(
	std::shared_ptr<VulkanInstance> instance,
	std::shared_ptr<VulkanDevice> device,
	VkDeviceSize preferredLargeHeapBlockSize)
{
	if (!instance || !instance->IsInitialized())
	{
		ReportError("Invalid or uninitialized VulkanInstance provided. 0x00003000");
		return false;
	}

	if (!device || !device->IsInitialized())
	{
		ReportError("Invalid or uninitialized VulkanDevice provided. 0x00003010");
		return false;
	}

	m_instance = instance; 
	m_device = device; 
	m_preferredLargeHeapBlockSize = preferredLargeHeapBlockSize; 

	VmaAllocatorCreateInfo allocatorInfo = {}; 

	allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3;
	allocatorInfo.physicalDevice = m_device->GetPhysicalDevice(); 
	allocatorInfo.device = m_device->GetDevice(); 
	allocatorInfo.instance = m_instance->GetInstance();

	if (preferredLargeHeapBlockSize > 0)
		allocatorInfo.preferredLargeHeapBlockSize = preferredLargeHeapBlockSize; 

	VkResult result = vmaCreateAllocator(&allocatorInfo, &m_allocator);

	if (result != VK_SUCCESS)
	{
		ReportError("Failed to create VMA allocator. 0x00003020");
		return false;
	}
	
	return true; 

}
