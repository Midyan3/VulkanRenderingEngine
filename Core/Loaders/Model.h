#pragma once

#include <vector>
#include <string>
#include <map>
#include <tuple>
#include "../../Headers/GlmConfig.h"
#include "../MaterialHandler/Material.h"
#include "../Renderer/VertexTypes/ModelVertex.h"
#include <unordered_map>
#include <iostream>
#include <algorithm>

namespace Model
{
    struct ModelData
    {
        std::vector<glm::vec3> positions;
        std::vector<glm::vec3> normals;
        std::vector<glm::vec2> texCoords;
        std::vector<glm::vec3> colors;
        std::vector<int32_t> materialIndexPerTriangle;
        std::vector<Material::MaterialInfo> materials; 
   
        struct FaceVertex
        {
            int positionIndex = -1;
            int texCoordIndex = -1;
            int normalIndex = -1;
            
        };

        std::vector<FaceVertex> faceVertices;
        std::vector<uint32_t> indices;

        std::string name;
        bool hasNormals = false;
        bool hasTexCoords = false;
        bool hasColors = false;


        void Clear()
        {
            positions.clear();
            normals.clear();
            texCoords.clear();
            colors.clear();
            faceVertices.clear();
            indices.clear();
            materials.clear(); 
            name.clear();
            materialIndexPerTriangle.clear();
            hasNormals = false;
            hasTexCoords = false;
            hasColors = false;
        }

        bool IsValid() const
        {
            if (positions.empty()) return false;
            if (faceVertices.empty()) return false;
            if ((faceVertices.size() % 3) != 0) return false;

            if (!materialIndexPerTriangle.empty())
            {
                if (materialIndexPerTriangle.size() != (faceVertices.size() / 3))
                    return false;
            }

            return true;
        }

        size_t GetVertexCount() const { return positions.size(); }
        size_t GetIndexCount() const { return indices.size(); }
        size_t GetTriangleCount() const { return faceVertices.size() / 3; }
        size_t GetFaceVertexCount() const { return faceVertices.size(); }
    };

    struct ModelMesh
    {
        struct SubMesh
        {
            uint32_t offset = 0; 
            uint32_t indexCount = 0; 
            int32_t material = -1; 
        };

        std::vector<ModelVertex> vertices;
        std::vector<SubMesh> subMeshes; 
        std::vector<uint32_t> indices;
        std::string name;

        void BuildFromData(const ModelData& data)
        {
            Clear();
            name = data.name;

            // SAFE: include material in vertex key (avoids cross-material vertex sharing issues)
            std::map<std::tuple<int, int, int, int>, uint32_t> vertexMap;

            // materialIndex -> list of indices
            std::unordered_map<int32_t, std::vector<uint32_t>> indicesByItsMaterial;


            const size_t triCount = data.faceVertices.size() / 3;

            for (size_t t = 0; t < triCount; ++t)
            {
                const int32_t mat =
                    (data.materialIndexPerTriangle.empty())
                    ? -1
                    : data.materialIndexPerTriangle[t];

                // corners 0,1,2 of triangle t
                for (int corner = 0; corner < 3; ++corner)
                {
                    const auto& fv = data.faceVertices[t * 3 + corner];

                    auto key = std::make_tuple(
                        fv.positionIndex,
                        fv.texCoordIndex,
                        fv.normalIndex,
                        mat
                    );

                    auto it = vertexMap.find(key);

                    if (it != vertexMap.end())
                    {
                        indicesByItsMaterial[mat].push_back(it->second);
                    }
                    else
                    {
                        ModelVertex v{};

                        if (fv.positionIndex >= 0 && fv.positionIndex < (int)data.positions.size())
                            v.position = data.positions[fv.positionIndex];
                        else
                            v.position = glm::vec3(0.0f);

                        if (fv.normalIndex >= 0 && fv.normalIndex < (int)data.normals.size())
                            v.normal = data.normals[fv.normalIndex];
                        else
                            v.normal = glm::vec3(0.0f, 0.0f, 1.0f);

                        if (fv.texCoordIndex >= 0 && fv.texCoordIndex < (int)data.texCoords.size())
                            v.texCoord = data.texCoords[fv.texCoordIndex];
                        else
                            v.texCoord = glm::vec2(0.0f);

                        v.color = GenerateColorFromPosition(v.position);

                        uint32_t newIndex = (uint32_t)vertices.size();
                        vertices.push_back(v);

                        vertexMap[key] = newIndex;
                        indicesByItsMaterial[mat].push_back(newIndex);
                    }
                }
            }

            indices.clear();
            subMeshes.clear();

            std::vector<int32_t> mats;
            mats.reserve(indicesByItsMaterial.size());
            for (auto& kv : indicesByItsMaterial) mats.push_back(kv.first);
            std::sort(mats.begin(), mats.end());

            for (int32_t mat : mats)
            {
                auto& bucket = indicesByItsMaterial[mat];
                if (bucket.empty())
                    continue;

                SubMesh sm;
                sm.material = mat;
                sm.offset = (uint32_t)indices.size();
                sm.indexCount = (uint32_t)bucket.size();

                indices.insert(indices.end(), bucket.begin(), bucket.end());
                subMeshes.push_back(sm);
            }
        }

        static glm::vec3 GenerateColorFromPosition(const glm::vec3& pos)
        {
            return glm::vec3(
                glm::clamp(/*(pos.x + 1.0f) **/ 0.5f, 0.0f, 1.0f),
                glm::clamp(/*(pos.y + 1.0f) **/ 0.5f, 0.0f, 1.0f),
                glm::clamp(/*(pos.z + 1.0f) * */0.5f, 0.0f, 1.0f)
            );
        }

        void Clear()
        {
            vertices.clear();
            indices.clear();
            name.clear();
        }

        bool IsValid() const
        {
            return !vertices.empty() &&
                !indices.empty() &&
                (indices.size() % 3 == 0);
        }

        size_t GetVertexCount() const { return vertices.size(); }
        size_t GetIndexCount() const { return indices.size(); }
        size_t GetTriangleCount() const { return indices.size() / 3; }
        size_t GetVertexBufferSize() const { return vertices.size() * sizeof(ModelVertex); }
        size_t GetIndexBufferSize() const { return indices.size() * sizeof(uint32_t); }

    };
}
