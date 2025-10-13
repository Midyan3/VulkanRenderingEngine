#include "VulkanSynchronization.h"

const Debug::DebugOutput VulkanSynchronization::DebugOut;

VulkanSynchronization::VulkanSynchronization(){}

void VulkanSynchronization::Cleanup()
{
	if (!m_frameSyncObjects.empty() && m_device && m_device->IsInitialized())
	{
		for (auto& frameSyncObject : m_frameSyncObjects)
		{
			if (frameSyncObject.IsValid())
			{
				vkDestroyFence(m_device->GetDevice(), frameSyncObject.inFlightFence, nullptr);
			}
		}
	}

    if (!m_imageSyncObjects.empty() && m_device && m_device->IsInitialized())
    {
        for (auto& imageSyncObject : m_imageSyncObjects)
        {
            if (imageSyncObject.IsValid())
            {
                vkDestroySemaphore(m_device->GetDevice(), imageSyncObject.renderFinishedSemaphore, nullptr);
                vkDestroySemaphore(m_device->GetDevice(), imageSyncObject.imageAvailableSemaphore, nullptr);
            }
        }
    }

	m_instance.reset();
	m_device.reset();
	m_frameSyncObjects.clear();
    m_imageSyncObjects.clear(); 
}

VulkanSynchronization::~VulkanSynchronization()
{
	Cleanup(); 
}

bool VulkanSynchronization::ValidateDependencies() const
{
    if (!m_instance)
    {
        ReportError("Instance null. 0x0000A000");
        return false;
    }

    if (!m_instance->IsInitialized())
    {
        ReportError("Instance not initialized. 0x0000A010");
        return false;
    }

    if (!m_device)
    {
        ReportError("Device null. 0x0000A020");
        return false;
    }

    if (!m_device->IsInitialized())
    {
        ReportError("Device not initialized. 0x0000A030");
        return false;
    }

    return true;
}

bool VulkanSynchronization::CreateSyncObjects(uint32_t count)
{
    m_frameSyncObjects.resize(count);

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32_t i = 0; i < count; i++)
    {
       VkResult result = vkCreateFence(m_device->GetDevice(), &fenceInfo, nullptr,
            &m_frameSyncObjects[i].inFlightFence);
        if (result != VK_SUCCESS)
        {
            ReportError("Failed to create fence. 0x0000A120");
            Cleanup();
            return false;
        }
    }

    return true;
}


bool VulkanSynchronization::CreateImageSyncObjects(uint32_t count)
{
    m_imageSyncObjects.resize(count);

    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    for (uint32_t i = 0; i < count; i++)
    {
        VkResult result = vkCreateSemaphore(m_device->GetDevice(), &semaphoreInfo, nullptr,
            &m_imageSyncObjects[i].imageAvailableSemaphore);
        if (result != VK_SUCCESS)
        {
            ReportError("Failed to create imageAvailable semaphore. 0x0000A100");
            Cleanup();
            return false;
        }

        result = vkCreateSemaphore(m_device->GetDevice(), &semaphoreInfo, nullptr,
            &m_imageSyncObjects[i].renderFinishedSemaphore);
        if (result != VK_SUCCESS)
        {
            ReportError("Failed to create renderFinished semaphore. 0x0000A110");
            Cleanup();
            return false;
        }
    }

    return true;
}


bool VulkanSynchronization::Initialize(
    std::shared_ptr<VulkanInstance> instance,
    std::shared_ptr<VulkanDevice> device,
    uint32_t maxFramesInFlight,
    uint32_t imageCount)
{
    try
    {
        m_device = device;
        m_instance = instance;

        if (!ValidateDependencies())
        {
            return false;
        }

        if (maxFramesInFlight == 0)
        {
            ReportError("Max frames in flight cannot be zero. 0x0000A210");
            return false;
        }

        if (!CreateSyncObjects(maxFramesInFlight))
        {
            return false;
        }

        if (!CreateImageSyncObjects(imageCount))
        {
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
        ReportError("Unknown exception. 0x000A200");
        return false;
    }
}

bool VulkanSynchronization::WaitForFence(uint32_t frameIndex, uint64_t timeout)
{
    if (frameIndex >= m_frameSyncObjects.size())
    {
        ReportError("Frame index out of bounds. 0x0000A300");
        return false;
    }

    VkResult result = vkWaitForFences(m_device->GetDevice(), 1,
        &m_frameSyncObjects[frameIndex].inFlightFence,
        VK_TRUE, timeout);
    if (result != VK_SUCCESS)
    {
        ReportError("Failed to wait for fence. 0x0000A310");
        return false;
    }

    return true;
}

bool VulkanSynchronization::ResetFence(uint32_t frameIndex)
{
    if (frameIndex >= m_frameSyncObjects.size())
    {
        ReportError("Frame index out of bounds. 0x0000A400");
        return false;
    }

    VkResult result = vkResetFences(m_device->GetDevice(), 1,
        &m_frameSyncObjects[frameIndex].inFlightFence);
    if (result != VK_SUCCESS)
    {
        ReportError("Failed to reset fence. 0x0000A410");
        return false;
    }

    return true;
}

const FrameSyncObjects& VulkanSynchronization::GetFrameSync(uint32_t frameIndex) const
{
    static const FrameSyncObjects invalidSync;  

    if (frameIndex >= m_frameSyncObjects.size())
    {
        ReportError("Frame index out of bounds. 0x0000A500");
        return invalidSync;
    }

    return m_frameSyncObjects[frameIndex];
}

const ImageSyncObjects& VulkanSynchronization::GetImageSync(uint32_t imageIndex) const
{
    static const ImageSyncObjects invalidSync;

    if (imageIndex >= m_imageSyncObjects.size())
    {
        ReportError("Image index out of bounds. 0x0000A505");
        return invalidSync;
    }

    return m_imageSyncObjects[imageIndex]; 
}


std::string VulkanSynchronization::GetSyncInfo() const
{
    if (!IsInitialized())
        return "VulkanSynchronization system not initialized";

    std::string info = "VulkanSynchronization Info:\n";

    info += "  Max Frames in flight: " + std::to_string(m_frameSyncObjects.size()) + "\n";


    return info;
}