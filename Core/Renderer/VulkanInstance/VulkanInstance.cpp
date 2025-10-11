#include "VulkanInstance.h"
#include "../../DebugOutput/DubugOutput.h"

VulkanInstance::VulkanInstance() 
{
	m_instance = VK_NULL_HANDLE;
	m_debugMessenger = VK_NULL_HANDLE; 
	m_validationEnabled = false;
    m_isInitialized = false;
}

VulkanInstance::~VulkanInstance()
{
    Cleanup(); 
}

void VulkanInstance::Cleanup()
{
    if (m_debugMessenger != VK_NULL_HANDLE)
    {
        auto cleanFunc = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT");
        if (cleanFunc)
            cleanFunc(m_instance, m_debugMessenger, nullptr); 
        m_debugMessenger = VK_NULL_HANDLE; 
    }

    if (m_instance != VK_NULL_HANDLE)
    {
        vkDestroyInstance(m_instance, nullptr); 
        m_instance = VK_NULL_HANDLE; 
    }

    m_validationEnabled = false; 

    m_enabledExtensions.clear(); 
    m_enabledLayers.clear();
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanInstance::DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
 
    Debug::DebugOutput::outputDebug("Vulkan Validation: " + std::string(pCallbackData->pMessage));

    // Future Reference: VK_TRUE would abort. Useful for later
    return VK_FALSE;
}

bool VulkanInstance::CheckExtensionAvailable(const std::string& ext)
{
    for (const auto& self_ext : extensions.extensionPropArray)
    {
        if (self_ext.extensionName == ext)
        {
            return true;
        }
    }

    return false;
}

bool VulkanInstance::CheckLayerAvailable(const char* layer)
{
    for (const auto& self_layer : layers.layerPropArray)
    {
        if (!std::strcmp(self_layer.layerName, layer)) 
        {
            return true; 
        }
    }

    return false;
}

bool VulkanInstance::Initialize(const std::string& applicationName, 
    const std::vector<const char*>& userExtensions)
{
	try 
	{
        VkResult result = vkEnumerateInstanceExtensionProperties(nullptr, &extensions.extensionCount, nullptr);
        if (result != VK_SUCCESS) 
        {
            return false;
        }

        extensions.extensionPropArray.resize(extensions.extensionCount); 
        result = vkEnumerateInstanceExtensionProperties(nullptr, &extensions.extensionCount, extensions.extensionPropArray.data());
        if (result != VK_SUCCESS) 
        {
            return false;
        }

        result = vkEnumerateInstanceLayerProperties(&layers.layerCount, nullptr);
        if (result != VK_SUCCESS) 
        {  
            return false;
        }

        layers.layerPropArray.resize(layers.layerCount); 
        result = vkEnumerateInstanceLayerProperties(&layers.layerCount, layers.layerPropArray.data());
        if (result != VK_SUCCESS) 
        {
            return false;
        }

        VkApplicationInfo appInfo = {};
        
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pNext = nullptr;
        appInfo.pApplicationName = applicationName.c_str();
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "CustomEngine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_3;

        m_enabledExtensions.clear(); 

        m_enabledExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
        m_enabledExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);

        for (const char* ext : userExtensions) {
            if (CheckExtensionAvailable(ext)) {
                m_enabledExtensions.push_back(ext);
            }
        }

        #ifdef _DEBUG
            if (CheckExtensionAvailable(VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) 
            {
                m_enabledExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            }
        #endif // _DEBUG

        m_enabledLayers.clear();

        #ifdef _DEBUG
            if (CheckLayerAvailable("VK_LAYER_KHRONOS_validation")) 
            {
                m_enabledLayers.push_back("VK_LAYER_KHRONOS_validation");
                m_validationEnabled = true;
            }
        #endif

        VkInstanceCreateInfo info = {};

        info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        info.pNext = nullptr;
        info.flags = 0;
        info.pApplicationInfo = &appInfo;
        info.enabledLayerCount = static_cast<uint32_t>(m_enabledLayers.size());
        info.ppEnabledLayerNames = m_enabledLayers.data();
        info.enabledExtensionCount = static_cast<uint32_t>(m_enabledExtensions.size());
        info.ppEnabledExtensionNames = m_enabledExtensions.data();

        result = vkCreateInstance(&info, nullptr, &m_instance);

        if (result != VK_SUCCESS)
        {
            return false;
        }

        if (m_validationEnabled)
        {
            if (!SetupDebugMessenger())
            {
                return false; 
            }
        }

        m_isInitialized = true;

        return m_isInitialized;

	}
    catch (const std::exception& e) 
    {

        Debug::DebugOutput::outputDebug(e.what()); 
        return false; 

    }
	catch (...) 
    {

        Debug::DebugOutput::outputDebug("Unexpected Error: Failed to create Vulkan Instance. 0x00001023"); 
        return false;
        
	}
}


std::vector<VkPhysicalDevice> VulkanInstance::GetAvailableDevices() const 
{   
    try {

        if (m_instance == VK_NULL_HANDLE)
            return {}; 

        uint32_t q_deviceCount = 0; 
        std::vector<VkPhysicalDevice> q_availableDevices; 

        VkResult result = vkEnumeratePhysicalDevices(m_instance, &q_deviceCount, nullptr); 

        if (result != VK_SUCCESS)
            return {}; 

        q_availableDevices.resize(static_cast<size_t>(q_deviceCount)); 

        result = vkEnumeratePhysicalDevices(m_instance, &q_deviceCount, q_availableDevices.data());

        if (result != VK_SUCCESS)
            return {}; 

        return q_availableDevices; 
        
    }
    catch (const std::exception& e)
    {

        Debug::DebugOutput::outputDebug(e.what());
        return {}; 

    }
    catch (...)
    {

        Debug::DebugOutput::outputDebug("Unexpected Error: Failed to retrieve devices from system. 0x00011500");
        return {};

    }
}

bool VulkanInstance::SetupDebugMessenger()
{

    auto createFunc = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT");

    if (!createFunc)
        return false;

    VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = DebugCallback;
    createInfo.pUserData = nullptr;

    VkResult result = createFunc(m_instance, &createInfo, nullptr, &m_debugMessenger);

    return result == VK_SUCCESS;
}

bool VulkanInstance::IsValidationEnabled() const
{
    return m_validationEnabled;
}

VkInstance VulkanInstance::GetInstance() const
{
    return m_instance;
}