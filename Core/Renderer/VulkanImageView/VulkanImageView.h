#pragma once 

#include "vulkan/vulkan.h"
#include <vk_mem_alloc.h>
#include <memory>
#include <string>
#include "../VulkanInstance/VulkanInstance.h"
#include "../VulkanDevice/VulkanDevice.h"
#include "../VulkanMemoryAllocator/VulkanMemoryAllocator.h"
#include "../VulkanCommandBuffer/VulkanCommandBuffer.h"
#include "../VulkanImage/VulkanImage.h"
#include "../../DebugOutput/DubugOutput.h"


struct AllocatedImageView
{
	VkImageView view = VK_NULL_HANDLE; 
	VkImageViewType type = VK_IMAGE_VIEW_TYPE_2D; 
	VkFormat format = VK_FORMAT_UNDEFINED;
	VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; 
	uint32_t baseMipLevel = 0;
	uint32_t levelCount = 1;
	uint32_t baseArrayLayer = 0;
	uint32_t layerCount = 1;

	bool IsValid() const { return view != VK_NULL_HANDLE; }
};

struct ImageViewOptions
{
	VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D; 
	VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	uint32_t baseMipLevel = 0;
	uint32_t levelCount = VK_REMAINING_MIP_LEVELS;
	uint32_t baseArrayLayer = 0;
	uint32_t layerCount = 1;

	static ImageViewOptions Default2D()
	{
		return ImageViewOptions{};
	}

	static ImageViewOptions Cubemap()
	{
		ImageViewOptions opt{}; 
		opt.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY; 
		opt.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
		opt.layerCount = 6;
		return opt;
	}; 

	static ImageViewOptions Array2D(uint32_t layerCount)
	{
		ImageViewOptions opts;
		opts.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
		opts.layerCount = layerCount;
		return opts;
	}
};

struct ImageViewCreateInfo 
{
	VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D;
	VkFormat format = VK_FORMAT_UNDEFINED;  
	VkComponentMapping components = {
		VK_COMPONENT_SWIZZLE_IDENTITY,
		VK_COMPONENT_SWIZZLE_IDENTITY,
		VK_COMPONENT_SWIZZLE_IDENTITY,
		VK_COMPONENT_SWIZZLE_IDENTITY
	};
	VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	uint32_t baseMipLevel = 0;
	uint32_t levelCount = VK_REMAINING_MIP_LEVELS;
	uint32_t baseArrayLayer = 0;
	uint32_t layerCount = 1;

	static ImageViewCreateInfo FromOptions(const ImageViewOptions& opts) {
		ImageViewCreateInfo info;
		info.viewType = opts.viewType;
		info.aspectMask = opts.aspectMask;
		info.baseMipLevel = opts.baseMipLevel;
		info.levelCount = opts.levelCount;
		info.baseArrayLayer = opts.baseArrayLayer;
		info.layerCount = opts.layerCount;
		return info;
	}

	VkImageViewCreateInfo ToVulkan(VkImage image, VkFormat imageFormat) const;
};

class VulkanImageView 
{
public:
	VulkanImageView();
	~VulkanImageView();

	VulkanImageView(const VulkanImageView&) = delete;
	VulkanImageView& operator=(const VulkanImageView&) = delete;
	VulkanImageView(VulkanImageView&& other) noexcept;
	VulkanImageView& operator=(VulkanImageView&& other) noexcept;

	bool Initialize(
		std::shared_ptr<VulkanInstance> instance,
		std::shared_ptr<VulkanDevice> device);

	void Cleanup();
	bool IsInitialized() const;

	bool CreateView(
		const AllocatedImage& image,
		AllocatedImageView& outView,
		const ImageViewOptions& options = ImageViewOptions::Default2D());

	bool CreateView(
		const AllocatedImage& image,
		AllocatedImageView& outView,
		const ImageViewCreateInfo& createInfo);

	void DestroyView(AllocatedImageView& view);

private:
	std::shared_ptr<VulkanInstance> m_instance;
	std::shared_ptr<VulkanDevice> m_device;

	static const Debug::DebugOutput DebugOut;

	bool ValidateDependencies() const;
	bool ValidateImageViewCreateInfo(
		const AllocatedImage& image,
		const ImageViewCreateInfo& createInfo) const;

	void ReportError(const std::string& message) const {
		DebugOut.outputDebug("VulkanImageView Error: " + message);
	}

	void ReportWarning(const std::string& message) const {
		DebugOut.outputDebug("VulkanImageView Warning: " + message);
	}
};