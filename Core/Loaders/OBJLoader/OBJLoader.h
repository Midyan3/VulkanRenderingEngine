#pragma once

#include "../ModelLoader.h"
#include "../Model.h"
#include <sstream>
#include <fstream>

class OBJLoader final : public ModelLoader
{
public:
    OBJLoader() = default;
    ~OBJLoader() override = default;

    bool Load(const std::string& filepath, Model::ModelMesh& outModel, Model::ModelData* out = nullptr) override;
    std::string GetSupportedExtension() const override { return "obj"; }

private:
    bool ParseOBJ(const std::string& filepath, Model::ModelData& data);

    bool ParseVertexIndex(const std::string& vertexStr,
        int& posIndex,
        int& texIndex,
        int& normIndex);

    void FixIndices(int& posIndex, int& texIndex, int& normIndex,
        int posSize, int texSize, int normSize);

    bool ValidateModelData(const Model::ModelData& data) const;
};