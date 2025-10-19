#pragma once

#include <memory>
#include <string>
#include "Model.h"
#include "../DebugOutput/DubugOutput.h"

// Base class for model loaders
class ModelLoader
{
public:
    virtual ~ModelLoader() = default;

    virtual bool Load(const std::string& filepath, Model::ModelMesh& outModel) = 0;

    virtual std::string GetSupportedExtension() const = 0;

    static std::unique_ptr<ModelLoader> CreateLoader(const std::string& filepath);

protected:
    static const Debug::DebugOutput DebugOut; 

    void ReportError(const std::string& message) const
    {
        DebugOut.outputDebug("ModelLoader Error: " + message);
    }

    void ReportWarning(const std::string& message) const
    {
        DebugOut.outputDebug("ModelLoader Warning: " + message);
    }
};
