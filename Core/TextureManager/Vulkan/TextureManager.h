#pragma once
#include <vulkan/vulkan.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <mutex>
#include "../../Renderer/TextureLoader/Texture.h"
#include "../../Renderer/VulkanImage/VulkanImage.h"
#include "../../Renderer/VulkanImageView/VulkanImageView.h"
#include "../../Renderer/VulkanDevice/VulkanDevice.h"
#include "../../Renderer/VulkanCommandBuffer/VulkanCommandBuffer.h"
#include "../../DebugOutput/DubugOutput.h"

struct TextureManagerStats 
{
    size_t totalTextures = 0;
    size_t cachedTextures = 0;
    size_t loadedThisFrame = 0;
};

class TextureManager 
{
public:
    TextureManager();
    ~TextureManager();

    TextureManager(const TextureManager&) = delete;
    TextureManager& operator=(const TextureManager&) = delete;

    bool Initialize(
        VulkanImage* imageManager,
        VulkanImageView* viewManager,
        VulkanDevice* device,
        VulkanCommandBuffer* cmdBuffer);

    void Cleanup();
    bool IsInitialized() const;

    std::shared_ptr<Texture> GetTexture(
        const std::string& filepath,
        const SamplerOptions& samplerOpts = SamplerOptions::DefaultLinear());

    std::shared_ptr<Texture> GetWhiteTexture() { return m_whiteTexture; }
    std::shared_ptr<Texture> GetBlackTexture() { return m_blackTexture; }
    std::shared_ptr<Texture> GetDefaultNormalTexture() { return m_defaultNormalTexture; }

    void UnloadTexture(const std::string& filepath);
    void UnloadAllTextures();

    bool IsTextureCached(const std::string& filepath) const;
    size_t GetLoadedTextureCount() const;

    TextureManagerStats GetStats() const;
    std::string GetTextureManagerInfo() const;

private:
    VulkanImage* m_imageManager = nullptr;
    VulkanImageView* m_viewManager = nullptr;
    VulkanDevice* m_device = nullptr;
    VulkanCommandBuffer* m_cmdBuffer = nullptr;

    std::unordered_map<std::string, std::shared_ptr<Texture>> m_textureCache;

    std::shared_ptr<Texture> m_whiteTexture;
    std::shared_ptr<Texture> m_blackTexture;
    std::shared_ptr<Texture> m_defaultNormalTexture;

    std::mutex mutable mux_lock;

    static const Debug::DebugOutput DebugOut;

    bool CreateDefaultTextures();
    std::shared_ptr<Texture> CreateTextureFromPixels(
        const std::string& name,
        const unsigned char* pixels,
        uint32_t width,
        uint32_t height);

    bool ValidateDependenices() const; 

    void ReportError(const std::string& message) const {
        DebugOut.outputDebug("TextureManager Error: " + message);
    }

    void ReportWarning(const std::string& message) const {
        DebugOut.outputDebug("TextureManager Warning: " + message);
    }
};