#pragma once

#include <vulkan/vulkan.h>
#include "../../../Headers/GlmConfig.h"
#include <vector>

// Full-featured vertex for model loading
// Supports: positions, normals, texture coordinates, and colors
struct ModelVertex
{
    glm::vec3 position;   // 3D position in space
    glm::vec3 normal;     // Surface normal (for lighting)
    glm::vec2 texCoord;   // Texture UV coordinates
    glm::vec3 color;      // Vertex color (generated or from file)

    // Default constructor
    ModelVertex()
        : position(0.0f)
        , normal(0.0f, 0.0f, 1.0f)  // Default: facing forward (Z+)
        , texCoord(0.0f)
        , color(1.0f, 1.0f, 1.0f)   // Default: white
    {
    }

    // Constructor with position only
    ModelVertex(const glm::vec3& pos)
        : position(pos)
        , normal(0.0f, 0.0f, 1.0f)
        , texCoord(0.0f)
        , color(1.0f, 1.0f, 1.0f)
    {
    }

    // Full constructor
    ModelVertex(const glm::vec3& pos, const glm::vec3& norm,
        const glm::vec2& uv, const glm::vec3& col)
        : position(pos)
        , normal(norm)
        , texCoord(uv)
        , color(col)
    {
    }

    // Vulkan vertex input binding description
    static VkVertexInputBindingDescription GetBindingDescription()
    {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(ModelVertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    // Vulkan vertex attribute descriptions
    static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions()
    {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions(4);

        // Location 0: Position
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(ModelVertex, position);

        // Location 1: Normal
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(ModelVertex, normal);

        // Location 2: TexCoord
        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(ModelVertex, texCoord);

        // Location 3: Color
        attributeDescriptions[3].binding = 0;
        attributeDescriptions[3].location = 3;
        attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[3].offset = offsetof(ModelVertex, color);

        return attributeDescriptions;
    }
};