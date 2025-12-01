#pragma once

#include "../Headers/GlmConfig.h"
#include "../Core/Window/Window.h"
#include "../Core/Window/OS-Windows/Win32/Win32Window.h"
#include "../Core/Window/OS-Windows/Win32/WindowManager/WindowManager.h"
#include "../Core/Renderer/VulkanInstance/VulkanInstance.h"
#include "../Core/Renderer/VulkanDevice/VulkanDevice.h"
#include "../Core/Renderer/VulkanSurface/VulkanSurface.h"
#include "../Core/Renderer/VulkanSwapchain/VulkanSwapchain.h"
#include "../Core/Renderer/VulkanRenderPass/VulkanRenderPass.h"
#include "../Core/Renderer/VulkanFrameBuffer/VulkanFrameBuffer.h"
#include "../Core/Renderer/VulkanGraphicsPipeline/VulkanGraphicsPipeline.h"
#include "../Core/Renderer/VulkanCommandBuffer/VulkanCommandBuffer.h"
#include "../Core/Renderer/VulkanMemoryAllocator/VulkanMemoryAllocator.h"
#include "../Core/Renderer/VulkanSynchronization/VulkanSynchronization.h"
#include "../Core/Application/Application.h"
#include "../Core/Application/WindowSpec/WindowSpec.h"
#include "../Core/Renderer/VertexTypes/Vertex.h"
#include "../Core/Camera/Camera.h"
#include "../Core/Renderer/VulkanDescriptor/VulkanDescriptor.h"
#include "../Core/TextureManager/Vulkan/TextureManager.h"
#include "../Core/Loaders/ModelLoader.h"
#include "../Core/Input/Input.h"
#include <chrono>
#include <algorithm>
#include <print>
#include <cmath>
#include "imgui.h"
#include "imgui_impl_vulkan.h"
#include "imgui_impl_win32.h"

#ifdef _WIN32
#include <windows.h>
#include <vulkan/vulkan_win32.h>
#endif

#include <iostream>
