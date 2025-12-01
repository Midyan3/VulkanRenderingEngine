#define STB_IMAGE_IMPLEMENTATION
#include "../../External/stb_image_header/stb_image.h"

#include "Texture.h"

const Debug::DebugOutput Texture::DebugOut;

VkSamplerCreateInfo SamplerOptions::ToVulkan() const 
{
    VkSamplerCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info.pNext = nullptr;
    info.flags = 0;
    info.magFilter = magFilter;
    info.minFilter = minFilter;
    info.mipmapMode = mipmapMode;
    info.addressModeU = addressModeU;
    info.addressModeV = addressModeV;
    info.addressModeW = addressModeW;
    info.mipLodBias = mipLodBias;
    info.anisotropyEnable = anisotropyEnable;
    info.maxAnisotropy = maxAnisotropy;
    info.compareEnable = compareEnable;
    info.compareOp = compareOp;
    info.minLod = minLod;
    info.maxLod = maxLod;
    info.borderColor = borderColor;
    info.unnormalizedCoordinates = unnormalizedCoordinates;

    return info;
}

Texture::Texture() 
{}

Texture::~Texture() 
{
    Destroy();
}

Texture::Texture(Texture&& other) noexcept
    : m_info(std::move(other.m_info))
    , m_imageManager(other.m_imageManager)
    , m_viewManager(other.m_viewManager)
    , m_device(other.m_device)
{
    other.m_imageManager = nullptr;
    other.m_viewManager = nullptr;
    other.m_device = nullptr;
}

Texture& Texture::operator=(Texture&& other) noexcept {
    if (this != &other) {
        Destroy();

        m_info = std::move(other.m_info);
        m_imageManager = other.m_imageManager;
        m_viewManager = other.m_viewManager;
        m_device = other.m_device;

        other.m_imageManager = nullptr;
        other.m_viewManager = nullptr;
        other.m_device = nullptr;
    }
    return *this;
}

bool Texture::Initialize(
    VulkanImage* imageManager,
    VulkanImageView* viewManager,
    VulkanDevice* device)
{
    if (!imageManager || !viewManager || !device) {
        ReportError("Null manager provided. 0x0013F000");
        return false;
    }

    m_imageManager = imageManager;
    m_viewManager = viewManager;
    m_device = device;

    return true;
}

bool Texture::IsInitialized() const {
    return m_imageManager != nullptr &&
        m_viewManager != nullptr &&
        m_device != nullptr;
}

void Texture::Destroy() 
{
    if (!IsValid()) {
        return;
    }

    if (m_info.sampler != VK_NULL_HANDLE && m_device) {
        vkDestroySampler(m_device->GetDevice(), m_info.sampler, nullptr);
        m_info.sampler = VK_NULL_HANDLE;
    }

    if (m_viewManager) {
        m_viewManager->DestroyView(m_info.imageView);
    }

    if (m_imageManager) {
        m_imageManager->DestroyImage(m_info.image);
    }
}

bool Texture::CreateSampler(const SamplerOptions& options) 
{
    if (!m_device) 
    {
        ReportError("Device not initialized. 0x0013F100");
        return false;
    }

    VkSamplerCreateInfo samplerInfo = options.ToVulkan();

    VkResult result = vkCreateSampler(
        m_device->GetDevice(),
        &samplerInfo,
        nullptr,
        &m_info.sampler
    );

    if (result != VK_SUCCESS) 
    {
        ReportError("Failed to create sampler. 0x0013F110");
        return false;
    }

    return true;
}

bool Texture::LoadFromFile(
    const std::string& filepath,
    VulkanCommandBuffer* cmdBuffer,
    const SamplerOptions& samplerOpts)
{
    if (!IsInitialized()) 
    {
        ReportError("Not initialized. 0x0013F200");
        return false;
    }

    if (!cmdBuffer) 
    {
        ReportError("Invalid command buffer. 0x0013F210");
        return false;
    }

    int width, height, channels;
    unsigned char* pixels = stbi_load(
        filepath.c_str(),
        &width,
        &height,
        &channels,
        STBI_rgb_alpha
    );

    if (!pixels) 
    {
        ReportError("Failed to load image: " + filepath + ". 0x0013F220");
        return false;
    }

    size_t imageSize = width * height * 4;

    ImageOptions imageOpts;
    imageOpts.width = static_cast<uint32_t>(width);
    imageOpts.height = static_cast<uint32_t>(height);
    imageOpts.format = VK_FORMAT_R8G8B8A8_SRGB;

    imageOpts.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
        VK_IMAGE_USAGE_SAMPLED_BIT;
    imageOpts.mipLevels = 1;

    if (!m_imageManager->CreateImage(imageOpts, m_info.image)) 
    {
        ReportError("Failed to create image. 0x0013F230");
        stbi_image_free(pixels);
        return false;
    }

    if (!m_imageManager->UploadData(
        cmdBuffer,
        m_info.image,
        pixels,
        imageSize,
        true))
    {
        ReportError("Failed to upload data. 0x0013F240");
        stbi_image_free(pixels);
        m_imageManager->DestroyImage(m_info.image);
        return false;
    }

    stbi_image_free(pixels);

    ImageViewOptions viewOpts = ImageViewOptions::Default2D();

    if (!m_viewManager->CreateView(m_info.image, m_info.imageView, viewOpts)) 
    {
        ReportError("Failed to create view. 0x0013F250");
        m_imageManager->DestroyImage(m_info.image);
        return false;
    }

    if (!CreateSampler(samplerOpts)) 
    {
        ReportError("Failed to create sampler. 0x0013F260");
        m_viewManager->DestroyView(m_info.imageView);
        m_imageManager->DestroyImage(m_info.image);
        return false;
    }

    return true;
}