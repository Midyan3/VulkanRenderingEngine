#pragma once

#include "../../Headers/GlmConfig.h"
#include <string>
#include <memory>

class Texture;
class TextureManager;

namespace Material
{
    struct MaterialInfo
    {
        std::string name;
        glm::vec3 kd{ 1.0f };

        std::string diffuseMapPath; 
        std::string normalMapPath;  
    };

    class MaterialHandle
    {
    public:
        MaterialHandle() = default;

        // Convert file info -> runtime textures
        bool Initialize(const MaterialInfo& info, TextureManager& texMan);

        bool IsInitialized() const { return m_isInitialized; }

        // Runtime access (renderer uses these)
        const MaterialInfo& GetInfo() const { return m_materialInfo; }
        const std::shared_ptr<Texture>& GetDiffuse() const { return m_diffuse; }
        const std::shared_ptr<Texture>& GetNormal() const { return m_normal; }

    private:
        MaterialInfo m_materialInfo{};
        bool m_isInitialized = false;

        std::shared_ptr<Texture> m_diffuse;
        std::shared_ptr<Texture> m_normal;
    };
}
