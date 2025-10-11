#include "VulkanSurface.h"
#include "../../Window/OS-Windows/Win32/Win32Window.h"

const Debug::DebugOutput VulkanSurface::DebugOut;

VulkanSurface::VulkanSurface() : m_surface(VK_NULL_HANDLE) {}

VulkanSurface::~VulkanSurface()
{
	Cleanup();
}

void VulkanSurface::Cleanup()
{
	if (m_surface != VK_NULL_HANDLE)
	{
		if (m_instance && m_instance->IsInitialized())
			vkDestroySurfaceKHR(m_instance->GetInstance(), m_surface, nullptr);
		m_surface = VK_NULL_HANDLE;
	}

	m_instance.reset();
	m_device.reset();
	m_window.reset();

}

bool VulkanSurface::ValidateDependencies() const
{
    if (!m_instance)
    {
        ReportError("VulkanInstance is null. 0x00004000");
        return false;
    }

    if (!m_instance->IsInitialized())
    {
        ReportError("VulkanInstance is not initialized. 0x00004005");
        return false;
    }

    if (!m_device)
    {
        ReportError("VulkanDevice is null. 0x00004010");
        return false;
    }

    if (!m_device->IsInitialized())
    {
        ReportError("VulkanDevice is not initialized. 0x00004015");
        return false;
    }

    if (!m_window)
    {
        ReportError("Window is null. 0x00004020");
        return false;
    }

    return true;
}

bool VulkanSurface::CreateWin32Surface()
{
    std::shared_ptr<Win32Window> q_WindowInstance = std::dynamic_pointer_cast<Win32Window>(m_window);
    if (!q_WindowInstance)
    {
        ReportError("Window is not a Win32Window. 0x00004100");
        return false;
    }

    HWND q_HWND = q_WindowInstance->GetHWND();
    HINSTANCE q_HINSTANCE = q_WindowInstance->GetHINSTANCE();

    if (!q_HWND)
    {
        ReportError("Invalid HWND from window. 0x00004110");
        return false;
    }

    VkWin32SurfaceCreateInfoKHR q_WindowInfo = {};
    q_WindowInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    q_WindowInfo.pNext = nullptr;
    q_WindowInfo.flags = 0;  
    q_WindowInfo.hinstance = q_HINSTANCE;
    q_WindowInfo.hwnd = q_HWND;

    VkResult result = vkCreateWin32SurfaceKHR(
        m_instance->GetInstance(),
        &q_WindowInfo,
        nullptr,
        &m_surface);

    if (result != VK_SUCCESS)
    {
        ReportError("vkCreateWin32SurfaceKHR failed. 0x00004120");
        return false;
    }

    return true;
}

bool VulkanSurface::IsDeviceCompatible() const 
{
    if (!m_device || !m_device->IsInitialized() || m_surface == VK_NULL_HANDLE)
    {
        ReportError("Device or Surface not valid. 0x00004300");
        return false;
    }

    return m_device->IsSurfaceSupported(m_surface); 
}

VkSurfaceCapabilitiesKHR VulkanSurface::GetCapabilities() const
{
    if (!m_device || m_surface == VK_NULL_HANDLE)
        return {};

    return m_device->GetSurfaceCapabilities(m_surface);
}

std::vector<VkSurfaceFormatKHR> VulkanSurface::GetFormats() const
{
    if (!m_device || m_surface == VK_NULL_HANDLE)
        return {};

    return m_device->GetSurfaceFormats(m_surface);
}

std::vector<VkPresentModeKHR> VulkanSurface::GetPresentModes() const
{
    if (!m_device || m_surface == VK_NULL_HANDLE)
        return {};

    return m_device->GetPresentModes(m_surface);
}

std::string VulkanSurface::GetSurfaceInfo() const
{
    if (!IsInitialized())
        return "Surface not initialized";

    std::string info = "VulkanSurface Info:\n";

    info += "  Surface Handle: " + std::to_string(reinterpret_cast<uint64_t>(m_surface)) + "\n";

    VkSurfaceCapabilitiesKHR caps = GetCapabilities();
    info += "  Min Image Count: " + std::to_string(caps.minImageCount) + "\n";
    info += "  Max Image Count: " + std::to_string(caps.maxImageCount) + "\n";
    info += "  Current Extent: " + std::to_string(caps.currentExtent.width) + "x" +
        std::to_string(caps.currentExtent.height) + "\n";

    auto formats = GetFormats();
    info += "  Supported Formats: " + std::to_string(formats.size()) + "\n";

    auto presentModes = GetPresentModes();
    info += "  Supported Present Modes: " + std::to_string(presentModes.size()) + "\n";

    return info;
}

bool VulkanSurface::Initialize(
    std::shared_ptr<VulkanInstance> instance,
    std::shared_ptr<VulkanDevice> device,
    std::shared_ptr<Window> window)
{
    try
    {
        // Store dependencies first
        m_instance = instance;
        m_device = device;
        m_window = window;

        // Validate them
        if (!ValidateDependencies())
            return false;

                // Create platform-specific surface
        #ifdef _WIN32
                if (!CreateWin32Surface())
                {
                    ReportError("Failed to create Win32 surface. 0x000F4230");
                    return false;
                }
        #elif defined(__linux__)
                if (!CreateX11Surface())
                {
                    ReportError("Failed to create X11 surface. 0x000F4240");
                    return false;
                }
        #elif defined(__APPLE__)
                if (!CreateMetalSurface())
                {
                    ReportError("Failed to create Metal surface. 0x000F4250");
                    return false;
                }
        #else
        #error "Unsupported platform for surface creation"
        #endif

// Verify device compatibility
        if (!IsDeviceCompatible())
        {
            ReportError("Selected device does not support this surface. 0x000F4260");
            Cleanup();
            return false;
        }

        return true;
    }
    catch (const std::exception& e)
    {
        ReportError("Exception during surface initialization: " + std::string(e.what()));
        return false;
    }
    catch (...)
    {
        ReportError("Unknown exception during surface initialization. 0x000F4270");
        return false;
    }
}
