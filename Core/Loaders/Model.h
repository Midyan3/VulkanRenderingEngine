#pragma once

#include <vector>
#include <string>
#include <map>
#include <tuple>
#include "../../Headers/GlmConfig.h"
#include "../Renderer/VertexTypes/ModelVertex.h"
#include <iostream>

namespace Model
{
    struct ModelData
    {
        std::vector<glm::vec3> positions;
        std::vector<glm::vec3> normals;
        std::vector<glm::vec2> texCoords;
        std::vector<glm::vec3> colors;

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
            name.clear();
            hasNormals = false;
            hasTexCoords = false;
            hasColors = false;
        }

        bool IsValid() const
        {
            return !positions.empty() &&
                !indices.empty() &&
                (indices.size() % 3 == 0);
        }

        size_t GetVertexCount() const { return positions.size(); }
        size_t GetIndexCount() const { return indices.size(); }
        size_t GetTriangleCount() const { return indices.size() / 3; }
    };

    struct ModelMesh
    {
        std::vector<ModelVertex> vertices;
        std::vector<uint32_t> indices;
        std::string name;

        void BuildFromData(const ModelData& data)
        {

            name = data.name;
            vertices.clear();
            indices.clear();

            std::map<std::tuple<int, int, int>, uint32_t> vertexMap;

            for (size_t i = 0; i < data.faceVertices.size(); ++i)
            {
                const auto& fv = data.faceVertices[i];

                if (i == 0)
                {
                    std::cout << "First FaceVertex: pos=" << fv.positionIndex
                        << " tex=" << fv.texCoordIndex
                        << " norm=" << fv.normalIndex << "\n";
                }

                auto key = std::make_tuple(fv.positionIndex, fv.texCoordIndex, fv.normalIndex);
                auto it = vertexMap.find(key);

                if (it != vertexMap.end())
                {
                    indices.push_back(it->second);
                }
                else
                {
                    ModelVertex v;

                    if (fv.positionIndex >= 0 && fv.positionIndex < data.positions.size())
                    {
                        v.position = data.positions[fv.positionIndex];
                    }
                    else
                    {
                        std::cout << "Invalid position index " << fv.positionIndex << "\n";
                        v.position = glm::vec3(0.0f);
                    }

                    if (fv.normalIndex >= 0 && fv.normalIndex < data.normals.size())
                        v.normal = data.normals[fv.normalIndex];
                    else
                        v.normal = glm::vec3(0.0f, 0.0f, 1.0f);

                    if (fv.texCoordIndex >= 0 && fv.texCoordIndex < data.texCoords.size())
                        v.texCoord = data.texCoords[fv.texCoordIndex];
                    else
                        v.texCoord = glm::vec2(0.0f);

                    v.color = GenerateColorFromPosition(v.position);

                    if (vertices.size() == 0)
                    {
                        std::cout << "First vertex created: pos={"
                            << v.position.x << ", " << v.position.y << ", " << v.position.z
                            << "} color={" << v.color.r << ", " << v.color.g << ", " << v.color.b << "}\n";
                    }

                    uint32_t newIndex = static_cast<uint32_t>(vertices.size());
                    vertices.push_back(v);
                    indices.push_back(newIndex);
                    vertexMap[key] = newIndex;
                }
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
