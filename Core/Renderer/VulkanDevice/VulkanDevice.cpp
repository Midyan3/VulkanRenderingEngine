#include "VulkanDevice.h"
#include <functional>
#include <algorithm>
#include <set>

VulkanDevice::VulkanDevice() {}

void VulkanDevice::Cleanup()
{
	if (m_device != VK_NULL_HANDLE)
	{
		vkDestroyDevice(m_device, nullptr);
		m_device = VK_NULL_HANDLE;
	}

	m_graphicsQueue = VK_NULL_HANDLE;
	m_presentQueue = VK_NULL_HANDLE;
	m_computeQueue = VK_NULL_HANDLE;
	m_transferQueue = VK_NULL_HANDLE;
	
	m_physicalDevice = VK_NULL_HANDLE;

	m_enabledExtensions.clear();
	m_supportedExtensions.clear();

}

VulkanDevice::~VulkanDevice()
{
	Cleanup();
}

bool VulkanDevice::Initialize(std::shared_ptr<VulkanInstance> instance,
	VkSurfaceKHR surface, const char* preferedDevice)
{
	if (!instance || !instance->IsInitialized())
	{
		ReportError("Invalid or uninitialized VulkanInstance provided. 0x00002000");
		return false; 
	}

	m_instance = instance; 

	std::vector<VkPhysicalDevice> availableDevices = m_instance->GetAvailableDevices();

	if (availableDevices.empty())
	{
		ReportError("No Vulkan device found. 0x00002010");
		return false;
	}

	DeviceScore bestScore;
	VkPhysicalDevice selectedDevice = VK_NULL_HANDLE; 
	

	if (preferedDevice) {
		auto it = std::find_if(availableDevices.begin(), availableDevices.end(),
			[preferedDevice](const VkPhysicalDevice& device) {
				VkPhysicalDeviceProperties deviceProps;
				vkGetPhysicalDeviceProperties(device, &deviceProps);
				return std::strcmp(deviceProps.deviceName, preferedDevice) == 0;
			});

		if (it != availableDevices.end()) {
			DeviceScore score = ScoreDevice(*it, surface);
			if (score.suitable) {
				selectedDevice = *it;
			}
		}
	}

	if (selectedDevice == VK_NULL_HANDLE) {
		for (const auto& device : availableDevices) {
			DeviceScore score = ScoreDevice(device, surface);
			if (score.suitable && score.totalScore > bestScore.totalScore) {
				bestScore = score;
				selectedDevice = device;
			}
		}
	}


	if (selectedDevice == VK_NULL_HANDLE)
	{
		ReportError("No suitable Vulkan device found. 0x00002020"); 
		return false;
	}

	m_physicalDevice = selectedDevice; 

	m_queueFamilyIndices = FindQueueFamilies(selectedDevice, surface); 

	SetupRequiredExtensions(); 

	if (!CreateLogicalDevice())
	{
		ReportError("Failed to create logical device. 0x00002030");
		return false; 
	}

	RetrieveQueues();
	QueryDeviceProperties();

	return true; 

}

void VulkanDevice::QueryDeviceProperties()
{
	vkGetPhysicalDeviceProperties(m_physicalDevice, &m_deviceProperties); 
	vkGetPhysicalDeviceFeatures(m_physicalDevice, &m_deviceFeatures); 
	vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &m_memoryProperties);
}

void VulkanDevice::RetrieveQueues()
{
	if (m_queueFamilyIndices.graphicsFamily.has_value())
	{
		vkGetDeviceQueue(m_device, m_queueFamilyIndices.graphicsFamily.value(), 0, &m_graphicsQueue);
	}

	if (m_queueFamilyIndices.presentFamily.has_value()) 
	{
		vkGetDeviceQueue(m_device, m_queueFamilyIndices.presentFamily.value(), 0, &m_presentQueue);
	}

	if (m_queueFamilyIndices.computeFamily.has_value()) 
	{
		vkGetDeviceQueue(m_device, m_queueFamilyIndices.computeFamily.value(), 0, &m_computeQueue);
	}

	if (m_queueFamilyIndices.transferFamily.has_value()) 
	{
		vkGetDeviceQueue(m_device, m_queueFamilyIndices.transferFamily.value(), 0, &m_transferQueue);
	}
}

bool VulkanDevice::CreateLogicalDevice()
{
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos; 
	std::set<uint32_t> uniqueQueueFam =
	{
		m_queueFamilyIndices.graphicsFamily.value(),
		m_queueFamilyIndices.presentFamily.value()
	}; 

	if (m_queueFamilyIndices.computeFamily.has_value()) {
		uniqueQueueFam.insert(m_queueFamilyIndices.computeFamily.value());
	}
	if (m_queueFamilyIndices.transferFamily.has_value()) {
		uniqueQueueFam.insert(m_queueFamilyIndices.transferFamily.value());
	}

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFam) 
	{
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;  // We only need 1 queue per family
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.samplerAnisotropy = VK_TRUE; 

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.pEnabledFeatures = &deviceFeatures;

	// Extensions
	createInfo.enabledExtensionCount = static_cast<uint32_t>(m_enabledExtensions.size());
	createInfo.ppEnabledExtensionNames = m_enabledExtensions.data();

	// Create device
	VkResult result = vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device);

	return result == VK_SUCCESS;
}

void VulkanDevice::QuerySupportedExtensions(VkPhysicalDevice device)
{
	uint32_t supportedExtensionCount; 
	VkResult result = vkEnumerateDeviceExtensionProperties(device, nullptr, &supportedExtensionCount, nullptr); 

	if (result != VK_SUCCESS)
		return;

	std::vector<VkExtensionProperties> q_extensions(supportedExtensionCount);
	result = vkEnumerateDeviceExtensionProperties(device, nullptr, &supportedExtensionCount, q_extensions.data());

	if (result != VK_SUCCESS)
		return;

	m_supportedExtensions.clear();
	m_supportedExtensions.resize(supportedExtensionCount); 

	for (const auto& ext : q_extensions)
	{
		m_supportedExtensions.emplace_back(ext.extensionName); 
	}
}

bool VulkanDevice::IsExtensionSupported(const std::string& extensionName) const
{
	return (std::find(m_supportedExtensions.begin(), m_supportedExtensions.end(),
		extensionName) != m_supportedExtensions.end()); 
}

void VulkanDevice::SetupRequiredExtensions()
{
	m_requiredExtensions.clear(); 
	m_optionalExtensions.clear(); 
	m_enabledExtensions.clear();

	m_requiredExtensions =
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	m_optionalExtensions = 
	{
		VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
		VK_EXT_MESH_SHADER_EXTENSION_NAME,
		VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME  
	};

	QuerySupportedExtensions(m_physicalDevice);

	for (const auto& required : m_requiredExtensions)
	{
		m_enabledExtensions.push_back(required.c_str()); 
	}

	for (const auto& optional : m_optionalExtensions)
	{
		if (IsExtensionSupported(optional))
		{
			m_enabledExtensions.push_back(optional.c_str()); 
		}
	}

}

QueueFamilyIndices VulkanDevice::FindQueueFamilies(const VkPhysicalDevice& device, const VkSurfaceKHR& surface) const
{
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr); 

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount); 
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data()); 

	for (uint32_t i = 0; i < queueFamilies.size(); ++i)
	{
		const VkQueueFamilyProperties& queueFam = queueFamilies[i]; 

		if (queueFam.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			indices.graphicsFamily = i;

		if (queueFam.queueFlags & VK_QUEUE_COMPUTE_BIT)
			indices.computeFamily = i; 

		if ((queueFam.queueFlags & VK_QUEUE_TRANSFER_BIT)
			&& !(queueFam.queueFlags & VK_QUEUE_GRAPHICS_BIT))
				indices.transferFamily = i; 
		
		if (surface != VK_NULL_HANDLE)
		{
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

			if (presentSupport)
				indices.presentFamily = i;
		}

		if (indices.IsComplete())
			break; 

	}

	if (!indices.transferFamily.has_value() && indices.graphicsFamily.has_value()) {
		indices.transferFamily = indices.graphicsFamily;
	}

	return indices;

}

int VulkanDevice::GetDeviceTypeScore(const VkPhysicalDeviceType& deviceType) const
{
	switch (deviceType)
	{
		case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
			return 1000; 

		case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
			return 500; 

		case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
			return 300;  

		case VK_PHYSICAL_DEVICE_TYPE_CPU:
			return 100;  

		case VK_PHYSICAL_DEVICE_TYPE_OTHER:
		default:
			return 50;  
	} 
}

int VulkanDevice::GetQueueScore(const QueueFamilyIndices& queueFamilies) const
{
	int score = 0;

	if (queueFamilies.graphicsFamily.has_value()) {
		score += 100;  // Essential for rendering
	}

	if (queueFamilies.presentFamily.has_value()) {
		score += 100;  // Essential for display
	}

	if (queueFamilies.graphicsFamily == queueFamilies.presentFamily) {
		score += 50;  
	}

	if (queueFamilies.computeFamily.has_value() &&
		queueFamilies.computeFamily != queueFamilies.graphicsFamily) {
		score += 25;  
	}

	if (queueFamilies.HasSeparateTransfer()) {
		score += 25;   
	}

	return score;
}

int VulkanDevice::GetMemoryScore(const VkPhysicalDeviceMemoryProperties& memProps) const
{
	VkDeviceSize totalDeviceLocalMemory = 0;

	// Sum up all device-local memory heaps
	for (uint32_t i = 0; i < memProps.memoryHeapCount; i++) {
		if (memProps.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
			totalDeviceLocalMemory += memProps.memoryHeaps[i].size;
		}
	}

	// Convert to MB for scoring
	VkDeviceSize memoryMB = totalDeviceLocalMemory / (1024 * 1024);

	if (memoryMB >= 8192) {        // 8GB+ - High-end
		return 400;
	}
	else if (memoryMB >= 4096) { // 4-8GB - Mid-range
		return 300;
	}
	else if (memoryMB >= 2048) { // 2-4GB - Entry level
		return 200;
	}
	else if (memoryMB >= 1024) { // 1-2GB - Low-end
		return 100;
	}
	else {                       // <1GB - Very limited
		return 50;
	}
}

bool VulkanDevice::CheckDeviceExtensionSupport(VkPhysicalDevice device) const
{
	uint32_t extensionCount; 
	VkResult result = vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr); 
	
	if (result != VK_SUCCESS)
		return false; 

	std::vector<VkExtensionProperties> deviceExtensions(extensionCount); 
	result = vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, deviceExtensions.data());

	if (result != VK_SUCCESS)
		return false;

	std::vector<const char*> requiredExtensions =
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME //  Very important to have. For my implementation, need it. Will add user added extensions later
	};

	for (const char* required : requiredExtensions)
	{
		bool found = false;

		for (const auto& available : deviceExtensions)
		{
			if (std::strcmp(available.extensionName, required) == 0)
				found = true;

		}

		if (!found)
			return false;

	}

	return true;

}

DeviceScore VulkanDevice::ScoreDevice(const VkPhysicalDevice& device, const VkSurfaceKHR& surface) const
{
	DeviceScore score;

	VkPhysicalDeviceProperties deviceProps;
	VkPhysicalDeviceFeatures deviceFeatures;
	VkPhysicalDeviceMemoryProperties memoryProps;

	vkGetPhysicalDeviceProperties(device, &deviceProps);
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
	vkGetPhysicalDeviceMemoryProperties(device, &memoryProps);

	score.deviceName = deviceProps.deviceName;
	score.deviceType = deviceProps.deviceType;

	QueueFamilyIndices queueFamilies = FindQueueFamilies(device, surface);

	if (!queueFamilies.IsComplete()) {
		score.suitable = false;
		return score;
	}

	if (!CheckDeviceExtensionSupport(device)) {
		score.suitable = false;
		return score;
	}

	score.suitable = true;
	score.typeScore = GetDeviceTypeScore(deviceProps.deviceType);
	score.memoryScore = GetMemoryScore(memoryProps);
	score.queueScore = GetQueueScore(queueFamilies);
	score.totalScore = score.typeScore + score.memoryScore + score.queueScore;

	return score;
}

void VulkanDevice::WaitIdle() const
{
	if (m_device != VK_NULL_HANDLE)
	{
		vkDeviceWaitIdle(m_device);
	}
}

bool VulkanDevice::IsSurfaceSupported(VkSurfaceKHR surface) const
{
	if (surface == VK_NULL_HANDLE || m_physicalDevice == VK_NULL_HANDLE)
		return false;

	if (m_queueFamilyIndices.presentFamily.has_value())
	{
		VkBool32 supported = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(m_physicalDevice,
			m_queueFamilyIndices.presentFamily.value(), surface, &supported);
		return supported == VK_TRUE;
	}
	return false;
}

VkSurfaceCapabilitiesKHR VulkanDevice::GetSurfaceCapabilities(VkSurfaceKHR surface) const
{
	VkSurfaceCapabilitiesKHR capabilities = {};
	if (surface != VK_NULL_HANDLE && m_physicalDevice != VK_NULL_HANDLE)
	{
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, surface, &capabilities);
	}
	return capabilities;
}

std::vector<VkSurfaceFormatKHR> VulkanDevice::GetSurfaceFormats(VkSurfaceKHR surface) const
{
	std::vector<VkSurfaceFormatKHR> formats;
	if (surface != VK_NULL_HANDLE && m_physicalDevice != VK_NULL_HANDLE)
	{
		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, surface, &formatCount, nullptr);
		if (formatCount > 0)
		{
			formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, surface, &formatCount, formats.data());
		}
	}
	return formats;
}

std::vector<VkPresentModeKHR> VulkanDevice::GetPresentModes(VkSurfaceKHR surface) const
{
	std::vector<VkPresentModeKHR> presentModes;
	if (surface != VK_NULL_HANDLE && m_physicalDevice != VK_NULL_HANDLE)
	{
		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, surface, &presentModeCount, nullptr);
		if (presentModeCount > 0)
		{
			presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, surface, &presentModeCount, presentModes.data());
		}
	}
	return presentModes;
}