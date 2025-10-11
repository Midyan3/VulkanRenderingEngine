#pragma once

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include <string>
#include "../VulkanInstance/VulkanInstance.h"
#include "../VulkanDevice/VulkanDevice.h"
#include "../../Window/Window.h"
#include "../../DebugOutput/DubugOutput.h"

// Platform includes
#ifdef _WIN32
#include <vulkan/vulkan_win32.h>
#endif

#ifdef __linux__
#include <vulkan/vulkan_xlib.h>
#endif

#ifdef __APPLE__
#include <vulkan/vulkan_metal.h>
#endif

class VulkanSurface
{
public:
    VulkanSurface();
    ~VulkanSurface();

    // RAII
    VulkanSurface(const VulkanSurface&) = delete;
    VulkanSurface& operator=(const VulkanSurface&) = delete;
    VulkanSurface(VulkanSurface&& other) noexcept;
    VulkanSurface& operator=(VulkanSurface&& other) noexcept;

    // Lifecycle
    bool Initialize(std::shared_ptr<VulkanInstance> instance,
        std::shared_ptr<VulkanDevice> device,
        std::shared_ptr<Window> window);
    void Cleanup();
    bool IsInitialized() const { return m_surface != VK_NULL_HANDLE; }

    // Getters
    VkSurfaceKHR GetSurface() const { return m_surface; }

    // Queries
    VkSurfaceCapabilitiesKHR GetCapabilities() const;
    std::vector<VkSurfaceFormatKHR> GetFormats() const;
    std::vector<VkPresentModeKHR> GetPresentModes() const;
    bool IsDeviceCompatible() const;

    // Deps
    std::shared_ptr<VulkanInstance> GetInstance() const { return m_instance; }
    std::shared_ptr<VulkanDevice> GetDevice() const { return m_device; }
    std::shared_ptr<Window> GetWindow() const { return m_window; }

    // Debug
    std::string GetSurfaceInfo() const;

private:
    // Handles
    std::shared_ptr<VulkanInstance> m_instance;
    std::shared_ptr<VulkanDevice> m_device;
    std::shared_ptr<Window> m_window;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;

    // Logging
    static const Debug::DebugOutput DebugOut;

    // Platform creation
#ifdef _WIN32
    bool CreateWin32Surface();
#endif

#ifdef __linux__
    bool CreateX11Surface(); // TODO. Will be impelemented in future but linux is a pain in the butt :(
#endif

#ifdef __APPLE__
    bool CreateMetalSurface(); // TODO. Will be impelemented in future.
#endif

    // Internals
    bool ValidateDependencies() const;

    // Errors
    void ReportError(const std::string& message) const
    {
        DebugOut.outputDebug("VulkanSurface Error: " + message);
    }

    void ReportWarning(const std::string& message) const
    {
        DebugOut.outputDebug("VulkanSurface Warning: " + message);
    }
};
