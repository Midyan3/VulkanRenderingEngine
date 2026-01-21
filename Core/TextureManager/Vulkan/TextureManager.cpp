#include "TextureManager.h"
#include <print>
#include <filesystem>

namespace fs = std::filesystem; 

const Debug::DebugOutput TextureManager::DebugOut;

TextureManager::TextureManager()
{}

TextureManager::~TextureManager()
{
    Cleanup();
}

bool TextureManager::ValidateDependenices() const
{
    if (!m_device)
    {
        ReportError("Device is null. 0x0000E000");
        return false;
    }

    if (!m_device->IsInitialized())
    {
        ReportError("Device not initialized. 0x0000E010");
        return false;
    }

    if (!m_imageManager)
    {
        ReportError("Image manager is null. 0x0000E020");
        return false;
    }

    if (!m_imageManager->IsInitialized())
    {
        ReportError("Image manager not initialized. 0x0000E030");
        return false;
    }

    if (!m_viewManager)
    {
        ReportError("View manager is null. 0x0000E040");
        return false;
    }

    if (!m_viewManager->IsInitialized())
    {
        ReportError("View manager not initialized. 0x0000E050");
        return false;
    }

    if (!m_cmdBuffer)
    {
        ReportError("Command buffer is null. 0x0000E060");
        return false;
    }

    if (!m_cmdBuffer->IsInitialized())
    {
        ReportError("Command buffer not initialized. 0x0000E070");
        return false;
    }

    return true;
}

bool TextureManager::Initialize(
    VulkanImage* imageManager,
    VulkanImageView* viewManager,
    VulkanDevice* device,
    VulkanCommandBuffer* cmdBuffer)
{
    try
    {
        m_imageManager = imageManager;
        m_viewManager = viewManager;
        m_device = device;
        m_cmdBuffer = cmdBuffer;

        if (!ValidateDependenices())
        {
            m_imageManager = nullptr;
            m_viewManager = nullptr;
            m_device = nullptr;
            m_cmdBuffer = nullptr;
            return false;
        }

        if (!CreateDefaultTextures())
        {
            ReportError("Failed to create default textures. 0x0000E100");
            return false;
        }

        return true;
    }
    catch (const std::exception& e)
    {
        ReportError("Exception during initialization: " + std::string(e.what()) + ". 0x0000E110");
        return false;
    }
    catch (...)
    {
        ReportError("Unknown exception during initialization. 0x0000E120");
        return false;
    }
}

bool TextureManager::IsInitialized() const
{
    return m_imageManager != nullptr &&
        m_viewManager != nullptr &&
        m_device != nullptr &&
        m_cmdBuffer != nullptr;
}

void TextureManager::Cleanup()
{
    std::lock_guard<std::mutex> lock(mux_lock);

    UnloadAllTextures();

    m_whiteTexture.reset();
    m_blackTexture.reset();
    m_defaultNormalTexture.reset();

    m_imageManager = nullptr;
    m_viewManager = nullptr;
    m_device = nullptr;
    m_cmdBuffer = nullptr;
}

std::shared_ptr<Texture> TextureManager::CreateTextureFromPixels(
    const std::string& name,
    const unsigned char* pixels,
    uint32_t width,
    uint32_t height)
{
    if (!IsInitialized())
    {
        ReportError("Not initialized. 0x0000E200");
        return nullptr;
    }

    auto texture = std::make_shared<Texture>();

    if (!texture->Initialize(m_imageManager, m_viewManager, m_device))
    {
        ReportError("Failed to initialize texture: " + name + ". 0x0000E210");
        return nullptr;
    }

    ImageOptions imageOpts;
    imageOpts.width = width;
    imageOpts.height = height;
    imageOpts.format = VK_FORMAT_R8G8B8A8_SRGB;
    imageOpts.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageOpts.mipLevels = 1;

    TextureInfo& info = const_cast<TextureInfo&>(texture->GetInfo());

    if (!m_imageManager->CreateImage(imageOpts, info.image))
    {
        ReportError("Failed to create image for: " + name + ". 0x0000E220");
        return nullptr;
    }

    size_t imageSize = width * height * 4;
    if (!m_imageManager->UploadData(m_cmdBuffer, info.image, pixels, imageSize, true))
    {
        ReportError("Failed to upload data for: " + name + ". 0x0000E230");
        m_imageManager->DestroyImage(info.image);
        return nullptr;
    }

    ImageViewOptions viewOpts = ImageViewOptions::Default2D();
    if (!m_viewManager->CreateView(info.image, info.imageView, viewOpts))
    {
        ReportError("Failed to create view for: " + name + ". 0x0000E240");
        m_imageManager->DestroyImage(info.image);
        return nullptr;
    }

    SamplerOptions samplerOpts = SamplerOptions::DefaultLinear();
    VkSamplerCreateInfo samplerInfo = samplerOpts.ToVulkan();

    VkResult result = vkCreateSampler(m_device->GetDevice(), &samplerInfo, nullptr, &info.sampler);
    if (result != VK_SUCCESS)
    {
        ReportError("Failed to create sampler for: " + name + ". 0x0000E250");
        m_viewManager->DestroyView(info.imageView);
        m_imageManager->DestroyImage(info.image);
        return nullptr;
    }

    return texture;
}

static bool FindTexture(const std::string& nameOfFile, const std::string& currentDir, std::string& foundLocation)
{
    if (nameOfFile.empty() || currentDir.empty())
        return false;

    for (const auto& entry : fs::directory_iterator(currentDir))
    {
        if (entry.is_regular_file() && entry.path().filename().string() == nameOfFile)
        {
            foundLocation = entry.path().string();
            return true;
        }

        if (entry.is_directory())
        {
            if (FindTexture(nameOfFile, entry.path().string(), foundLocation))
                return true;
        }
    }

    return false;
}

bool TextureManager::LoadTexture(const std::string& path)
{
    if (!IsInitialized())
    {
        ReportWarning("Failed to load texture: TextureManager not initialized. 0x0000E500");
        return false;
    }

    if (path.empty())
    {
        ReportWarning("Failed to load texture: path empty. 0x0000E505");
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(mux_lock);
        auto it = m_textureCache.find(path);
        if (it != m_textureCache.end())
            return true;
    }

    std::string resolved = path;

    std::replace(resolved.begin(), resolved.end(), '\\', '/');

    std::string foundLocation;

    if (fs::exists(resolved) && fs::is_regular_file(resolved))
    {
        foundLocation = resolved;
    }
    else
    {
        std::string filename = fs::path(resolved).filename().string();

        if (!FindTexture(filename, ".", foundLocation))
        {
            ReportWarning("Failed to load texture: Unable to find texture: " + path + ". 0x0000E515");
            return false;
        }
    }

    auto temp = std::make_shared<Texture>();
    if (!temp->Initialize(m_imageManager, m_viewManager, m_device))
    {
        ReportWarning("Failed to load texture: Failed to init Texture. 0x0000E510");
        return false;
    }

    if (!temp->LoadFromFile(foundLocation, m_cmdBuffer))
    {
        ReportWarning("Failed to load texture: LoadFromFile failed: " + foundLocation + ". 0x0000E520");
        return false;
    }

    std::println("Successfully loaded path : {} (Maybe resolved path)", foundLocation); 

    {
        std::lock_guard<std::mutex> lock(mux_lock);
        m_textureCache[path] = temp;
        m_textureCache[foundLocation] = temp;
    }

    return true;
}

bool TextureManager::CreateDefaultTextures()
{
    unsigned char whitePixels[4] = { 255, 255, 255, 255 };
    m_whiteTexture = CreateTextureFromPixels("__white__", whitePixels, 1, 1);
    if (!m_whiteTexture)
    {
        ReportError("Failed to create white texture. 0x0000E300");
        return false;
    }
    
    {
        std::lock_guard<std::mutex> lock(mux_lock);
        m_textureCache["__white__"] = m_whiteTexture;
    }

    unsigned char blackPixels[4] = { 0, 0, 0, 255 };
    m_blackTexture = CreateTextureFromPixels("__black__", blackPixels, 1, 1);
    if (!m_blackTexture)
    {
        ReportError("Failed to create black texture. 0x0000E310");
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(mux_lock);
        m_textureCache["__black__"] = m_blackTexture;
    }


    unsigned char normalPixels[4] = { 128, 128, 255, 255 };
    m_defaultNormalTexture = CreateTextureFromPixels("__normal__", normalPixels, 1, 1);
    if (!m_defaultNormalTexture)
    {
        ReportError("Failed to create normal texture. 0x0000E320");
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(mux_lock);
        m_textureCache["__normal__"] = m_defaultNormalTexture;
    }


    return true;
}

std::shared_ptr<Texture> TextureManager::GetTexture(
    const std::string& filepath,
    const SamplerOptions& samplerOpts)
{
    if (!IsInitialized())
    {
        ReportError("Not initialized. 0x0000E400");
        return m_whiteTexture;
    }

    {
        std::lock_guard<std::mutex> lock(mux_lock);

        auto it = m_textureCache.find(filepath);
        if (it != m_textureCache.end())
        {
            return it->second;
        }
    }

    auto texture = std::make_shared<Texture>();

    if (!texture->Initialize(m_imageManager, m_viewManager, m_device))
    {
        ReportError("Failed to initialize texture. 0x0000E410");
        return m_whiteTexture;
    }

    if (!texture->LoadFromFile(filepath, m_cmdBuffer, samplerOpts))
    {
        ReportWarning("Failed to load texture: " + filepath + ". 0x0000E420");
        return m_whiteTexture;
    }

    {
        std::lock_guard<std::mutex> lock(mux_lock);
        m_textureCache[filepath] = texture;
    }

    return texture;
}

void TextureManager::UnloadTexture(const std::string& filepath)
{
    if (!IsInitialized())
    {
        return;
    }

    std::lock_guard<std::mutex> lock(mux_lock);

    auto it = m_textureCache.find(filepath);
    if (it != m_textureCache.end())
    {
        m_textureCache.erase(it);
    }
}

void TextureManager::UnloadAllTextures()
{
    std::lock_guard<std::mutex> lock(mux_lock);
    m_textureCache.clear();
}

bool TextureManager::IsTextureCached(const std::string& filepath) const
{
    std::lock_guard<std::mutex> lock(mux_lock);
    return m_textureCache.find(filepath) != m_textureCache.end();
}

size_t TextureManager::GetLoadedTextureCount() const
{
    std::lock_guard<std::mutex> lock(mux_lock);
    return m_textureCache.size();
}

TextureManagerStats TextureManager::GetStats() const
{
    std::lock_guard<std::mutex> lock(mux_lock);

    TextureManagerStats stats;
    stats.totalTextures = m_textureCache.size() + 3;
    stats.cachedTextures = m_textureCache.size();
    return stats;
}

std::string TextureManager::GetTextureManagerInfo() const
{
    if (!IsInitialized())
    {
        return "TextureManager not initialized";
    }

    std::lock_guard<std::mutex> lock(mux_lock);

    std::string info = "TextureManager Info:\n";
    info += "  Loaded Textures: " + std::to_string(m_textureCache.size()) + "\n";
    info += "  Default Textures: 3 (white, black, normal)\n";

    return info;
}
