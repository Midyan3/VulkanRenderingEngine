#include "ModelLoader.h"
#include "OBJLoader/OBJLoader.h"
#include <algorithm>

const Debug::DebugOutput ModelLoader::DebugOut;

std::unique_ptr<ModelLoader> ModelLoader::CreateLoader(const std::string& filepath)
{
    size_t dotPos = filepath.find_last_of('.');
    if (dotPos == std::string::npos)
    {
        DebugOut.outputDebug("ModelLoader: No file extension found in: " + filepath + ". 0x0000F000");
        return nullptr;
    }

    std::string extension = filepath.substr(dotPos + 1);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower); 

    if (extension == "obj")
    {
        return std::make_unique<OBJLoader>();
    }
    // More will be added soon
    else
    {
        DebugOut.outputDebug("ModelLoader: Unsupported file format: ." + extension + ". 0x0000F010");
        return nullptr;
    }
}
