#pragma once

#include <vector>
#include <string>
#include <cstring>

#ifdef _WIN32
#include <windows.h> 
#endif

#include <vulkan/vulkan.h>
#include "VkPropStructs/VkPropStructs.h"

#ifdef _WIN32
#include <vulkan/vulkan_win32.h>
#endif

class VulkanInstance {
private:

	// Members
	VkPropStructs::VkExtensionProp extensions;
	VkPropStructs::VkLayerProp layers;
	VkInstance m_instance;
	VkDebugUtilsMessengerEXT m_debugMessenger;
	bool m_validationEnabled;
	bool m_isInitialized;
	std::vector<const char*> m_enabledExtensions;
	std::vector<const char*> m_enabledLayers;

	// Internals
	bool CheckLayerAvailable(const char* layer);
	bool CheckExtensionAvailable(const std::string& ext);
	bool SetupDebugMessenger();

	// Debug callback
	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData);

public:

	VulkanInstance();
	~VulkanInstance();

	// Lifecycle
	bool Initialize(const std::string& applicationName, const std::vector<const char*>& userExtensions = {});
	void Cleanup();

	// Queries
	std::vector<VkPhysicalDevice> GetAvailableDevices() const;

	// Getters
	VkInstance GetInstance() const;
	bool IsValidationEnabled() const;
	const bool IsInitialized() const { return m_isInitialized; }

};
