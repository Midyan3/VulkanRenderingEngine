#include "VulkanCommandBuffer.h"

const Debug::DebugOutput VulkanCommandBuffer::DebugOut; 

VulkanCommandBuffer::VulkanCommandBuffer()
{
	m_commandPool = VK_NULL_HANDLE;
	m_queueFamilyIndex = 0;
}

void VulkanCommandBuffer::Cleanup()
{
	if (m_commandPool != VK_NULL_HANDLE)
	{
		if (m_device && m_device->IsInitialized())
		{
			vkFreeCommandBuffers(m_device->GetDevice(), m_commandPool,
				static_cast<uint32_t>(m_allocatedBuffers.size()), m_allocatedBuffers.data());

			vkDestroyCommandPool(m_device->GetDevice(), m_commandPool, nullptr);

		}

		m_commandPool = VK_NULL_HANDLE;
	}

	m_allocatedBuffers.clear();

	m_device.reset(); 
	m_instance.reset();
	m_queueFamilyIndex = 0;

}

VulkanCommandBuffer::~VulkanCommandBuffer()
{
	Cleanup();
}

bool VulkanCommandBuffer::CreateCommandPool()
{
	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = m_queueFamilyIndex;

	if (!m_device || !m_device->IsInitialized())
	{
		ReportError("Failed to create command pool. Device either null or unintitiliazed. 0x00006200");
		return false;
	}

	VkResult result = vkCreateCommandPool(m_device->GetDevice(), &poolInfo, nullptr, &m_commandPool);

	if (result != VK_SUCCESS)
	{
		ReportError("Failed to create command pool. 0x00006210");
		return false;
	}

	return true;
}

bool VulkanCommandBuffer::ValidateDependencies() const
{

	if (!m_instance)
	{
		ReportError("Instance null. 0x00006000");
		return false;
	}	

	if (!m_instance->IsInitialized())
	{
		ReportError("Instance not initialized. 0x00006005");
		return false;
	}

	if (!m_device)
	{
		ReportError("Device null. 0x00006010");
		return false;
	}

	if (!m_device->IsInitialized())
	{
		ReportError("Device not initialized. 0x00006015");
		return false;
	}

	return true;
}


bool VulkanCommandBuffer::Initialize(
	std::shared_ptr<VulkanInstance> instance,
	std::shared_ptr<VulkanDevice> device,
	uint32_t queueFamilyIndex)
{
	try
	{
		m_instance = instance;
		m_device = device;
		m_queueFamilyIndex = queueFamilyIndex;

		if (!ValidateDependencies())
			return false;

		if (!CreateCommandPool())
			return false;

		return true;
	}
	catch (const std::exception& e)
	{
		ReportError("Exception during initialization: " + std::string(e.what()));
		return false;
	}
	catch (...)
	{
		ReportError("Unknown exception during initialization. 0x00006300");
		return false;
	}
}

VkCommandBuffer VulkanCommandBuffer::AllocateCommandBuffer(const CommandBufferConfig& config)
{
	if (!IsInitialized())
	{
		ReportError("Command buffer not initialized. 0x00006400");
		return VK_NULL_HANDLE;
	}

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_commandPool;
	allocInfo.level = config.level;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	VkResult result = vkAllocateCommandBuffers(m_device->GetDevice(), &allocInfo, &commandBuffer);

	if (result != VK_SUCCESS)
	{
		ReportError("Failed to allocate command buffer. 0x00006410");
		return VK_NULL_HANDLE;
	}

	m_allocatedBuffers.push_back(commandBuffer);
	return commandBuffer;
}

std::vector<VkCommandBuffer> VulkanCommandBuffer::AllocateCommandBuffers(
	uint32_t count,
	const CommandBufferConfig& config)
{
	if (!IsInitialized())
	{
		ReportError("Command buffer not initialized. 0x000A6420");
		return {};
	}

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_commandPool;
	allocInfo.level = config.level;
	allocInfo.commandBufferCount = count;

	std::vector<VkCommandBuffer> commandBuffers(static_cast<size_t>(count));
	VkResult result = vkAllocateCommandBuffers(m_device->GetDevice(), &allocInfo, commandBuffers.data());

	if (result != VK_SUCCESS)
	{
		ReportError("Failed to allocate command buffers. 0x000A6430");
		return {};
	}

	m_allocatedBuffers.insert(m_allocatedBuffers.end(), commandBuffers.begin(), commandBuffers.end());
	return commandBuffers;
}

void VulkanCommandBuffer::FreeCommandBuffer(VkCommandBuffer commandBuffer)
{
	if (!IsInitialized())
	{
		ReportError("Command buffer system not initialized. 0x00006500");
		return;
	}

	vkFreeCommandBuffers(m_device->GetDevice(), m_commandPool, 1, &commandBuffer);

	auto it = std::find(m_allocatedBuffers.begin(), m_allocatedBuffers.end(), commandBuffer);
	if (it != m_allocatedBuffers.end())
	{
		m_allocatedBuffers.erase(it);
	}
}

void VulkanCommandBuffer::FreeCommandBuffers(const std::vector<VkCommandBuffer>& commandBuffers)
{
	if (!IsInitialized())
	{
		ReportError("Command buffer system not initialized. 0x00006510");
		return;
	}

	if (commandBuffers.empty())
	{
		return;
	}

	vkFreeCommandBuffers(m_device->GetDevice(), m_commandPool,
		static_cast<uint32_t>(commandBuffers.size()),
		commandBuffers.data());

	for (const auto& commandBuffer : commandBuffers)
	{
		auto it = std::find(m_allocatedBuffers.begin(), m_allocatedBuffers.end(), commandBuffer);
		if (it != m_allocatedBuffers.end())
		{
			m_allocatedBuffers.erase(it);
		}
	}
}

bool VulkanCommandBuffer::BeginRecording(
	VkCommandBuffer commandBuffer,
	VkCommandBufferUsageFlags usageFlags)
{
	if (commandBuffer == VK_NULL_HANDLE)
	{
		ReportError("Command buffer not initilized, failed to begin recording. 0x00006600"); 
		return false;
	}

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = usageFlags;
	beginInfo.pInheritanceInfo = nullptr;

	VkResult result = vkBeginCommandBuffer(commandBuffer, &beginInfo);

	if (result != VK_SUCCESS)
	{
		ReportError("Failed to begin recording. 0x00006610");
		return false;
	}

	return true;
}

bool VulkanCommandBuffer::EndRecording(VkCommandBuffer commandBuffer)
{
	if (commandBuffer == VK_NULL_HANDLE)
	{
		ReportError("Command buffer not initilized, failed to end recording. 0x00006700");
		return false;
	}

	VkResult result = vkEndCommandBuffer(commandBuffer);

	if (result != VK_SUCCESS)
	{
		ReportError("Failed to end recording. 0x00006710");
		return false;
	}

	return true;
}

bool VulkanCommandBuffer::Reset(
	VkCommandBuffer commandBuffer,
	VkCommandBufferResetFlags flags)
{
	if (commandBuffer == VK_NULL_HANDLE)
	{
		ReportError("Command buffer not initilized, failed to reset command buffer. 0x00006800");
		return false;
	}

	VkResult result = vkResetCommandBuffer(commandBuffer, flags);

	if (result != VK_SUCCESS)
	{
		ReportError("Failed to reset command buffer. 0x00006810");
		return false;
	}

	return true;
}

VkCommandBuffer VulkanCommandBuffer::BeginSingleTimeCommands()
{
	if (!IsInitialized())
	{
		ReportError("Command buffer system not initialized. 0x00006900");
		return VK_NULL_HANDLE;
	}

	VkCommandBuffer commandBuffer = AllocateCommandBuffer();
	if (commandBuffer == VK_NULL_HANDLE)
	{
		return VK_NULL_HANDLE; 
	}

	if (!BeginRecording(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT))
	{
		FreeCommandBuffer(commandBuffer);  
		return VK_NULL_HANDLE;  
	}

	return commandBuffer;
}

bool VulkanCommandBuffer::EndSingleTimeCommands(VkCommandBuffer commandBuffer)
{
	if (!EndRecording(commandBuffer))
	{
		return false;
	}

	VkQueue graphicsQueue = m_device->GetGraphicsQueue();

	if (!Submit(commandBuffer, graphicsQueue))
	{
		return false;
	}

	VkResult result = vkQueueWaitIdle(graphicsQueue);
	if (result != VK_SUCCESS)
	{
		ReportError("Failed to wait for queue to finish. 0x00006A00");
		FreeCommandBuffer(commandBuffer);  
		return false;
	}

	FreeCommandBuffer(commandBuffer);
	return true;
}

bool VulkanCommandBuffer::Submit(
	VkCommandBuffer commandBuffer,
	VkQueue queue,
	const std::vector<VkSemaphore>& waitSemaphores,
	const std::vector<VkPipelineStageFlags>& waitStages,
	const std::vector<VkSemaphore>& signalSemaphores,
	VkFence fence)
{
	if (commandBuffer == VK_NULL_HANDLE)
	{
		ReportError("Invalid command buffer. 0x00006B00");
		return false;
	}

	if (queue == VK_NULL_HANDLE)
	{
		ReportError("Invalid queue. 0x00006B05");
		return false;
	}

	if (waitSemaphores.size() != waitStages.size())
	{
		ReportError("Wait semaphores and wait stages must have same count. 0x00006B10");
		return false;
	}

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
	submitInfo.pWaitSemaphores = waitSemaphores.empty() ? nullptr : waitSemaphores.data();
	submitInfo.pWaitDstStageMask = waitStages.empty() ? nullptr : waitStages.data();

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
	submitInfo.pSignalSemaphores = signalSemaphores.empty() ? nullptr : signalSemaphores.data();

	VkResult result = vkQueueSubmit(queue, 1, &submitInfo, fence);
	if (result != VK_SUCCESS)
	{
		ReportError("Failed to submit command buffer to queue. 0x00006B20");
		return false;
	}

	return true;
}

bool VulkanCommandBuffer::SubmitMultiple(
	const std::vector<VkCommandBuffer>& commandBuffers,
	VkQueue queue,
	const std::vector<VkSemaphore>& waitSemaphores,
	const std::vector<VkPipelineStageFlags>& waitStages,
	const std::vector<VkSemaphore>& signalSemaphores,
	VkFence fence)
{

	if (commandBuffers.empty())
	{
		return true; // Im returning true since techincally 0 work = work done;
	}

	if (queue == VK_NULL_HANDLE)
	{
		ReportError("Invalid queue. 0x00006C00");
		return false;
	}

	if (waitSemaphores.size() != waitStages.size())
	{
		ReportError("Wait semaphores and wait stages must have same count. 0x00006B10");
		return false;
	}

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
	submitInfo.pWaitSemaphores = waitSemaphores.empty() ? nullptr : waitSemaphores.data();
	submitInfo.pWaitDstStageMask = waitStages.empty() ? nullptr : waitStages.data();

	submitInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
	submitInfo.pCommandBuffers = commandBuffers.data();

	submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
	submitInfo.pSignalSemaphores = signalSemaphores.empty() ? nullptr : signalSemaphores.data();

	VkResult result = vkQueueSubmit(queue, 1, &submitInfo, fence);
	if (result != VK_SUCCESS)
	{
		ReportError("Failed to submit command buffer to queue. 0x00006C20");
		return false;
	}

	return true;

}

bool VulkanCommandBuffer::ResetPool(VkCommandPoolResetFlags flags)
{

	if (m_commandPool == VK_NULL_HANDLE || !IsInitialized())
	{
		ReportError("Failed to reset pool, VulkanCommandBuffer not initlized or commandpool null. 0x00006D00");
		return false;
	}

	VkResult result = vkResetCommandPool(m_device->GetDevice(), m_commandPool, flags);

	if (result != VK_SUCCESS)
	{
		ReportError("Failed to reset command pool. 0x00006D10");
		return false;
	}

	return true;
}

std::string VulkanCommandBuffer::GetCommandBufferInfo() const
{
	if (!IsInitialized())
		return "Command buffer system not initialized";

	std::string info = "VulkanCommandBuffer Info:\n";
	
	info += "  Command Pool Handle: " + std::to_string(reinterpret_cast<uint64_t>(m_commandPool)) + "\n";

	info += "  Queue Family Index: " + std::to_string(m_queueFamilyIndex) + "\n";

	info += "  Allocated Buffers: " + std::to_string(m_allocatedBuffers.size()) + "\n";

	return info;
}