#pragma once
#include <vulkan/vulkan.h>
#include <vector>

namespace VkPropStructs {
    struct VkExtensionProp 
    {
        VkExtensionProp() = default;
        std::vector<VkExtensionProperties> extensionPropArray; 
        uint32_t extensionCount = 0;
    };
    struct VkLayerProp 
    {
        VkLayerProp() = default;
        std::vector<VkLayerProperties> layerPropArray;  
        uint32_t layerCount = 0; 
    };
}