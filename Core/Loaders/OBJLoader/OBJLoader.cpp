#include "OBJLoader.h"
#include <filesystem>
#include <climits>
#include <print>
#include "../../MaterialHandler/Material.h"

namespace fs = std::filesystem;  


static bool ParseMTLFile(
    const fs::path& mtlpath,
    std::vector<Material::MaterialInfo>& outMaterials,
    std::unordered_map<std::string, int32_t>& outName
)
{
    std::ifstream file; 
    
    file.open(mtlpath); 

    if (!file.is_open())
    {
        return false; 
    }

    bool has_current = false; 

    std::string line; 
    int numOfLines{ 0 };    

    Material::MaterialInfo curr{};

    auto flushCurr = [&]()
        {
            if (!has_current)
            {
                return; 
            }
            
            if (!outName.contains(curr.name))
            {
                int32_t idx = (int32_t)(outMaterials.size()); 
                outMaterials.emplace_back(curr);
                outName[curr.name] = idx; 
            }

            curr = {}; 
            has_current = false;
     };

    while (std::getline(file, line))
    {
        ++numOfLines; 

        if (line.empty() || line[0] == '#')
            continue;

        std::istringstream ls(line);
        std::string tag;
        ls >> tag;

        if (tag == "newmtl")
        {
            flushCurr(); 
            ls >> curr.name; 
            has_current = !curr.name.empty(); 
        }
        else if (tag == "Kd" && has_current)
        {
            float r, g, b; 
            if (ls >> r >> g >> b)
            {
                curr.kd = glm::vec3(r, g, b); 
            }
        }
        else if (tag == "map_Kd" && has_current)
        {
            std::getline(ls, curr.diffuseMapPath);

            curr.diffuseMapPath.erase(
                curr.diffuseMapPath.begin(),
                std::find_if(curr.diffuseMapPath.begin(), curr.diffuseMapPath.end(),
                    [](unsigned char ch) { return !std::isspace(ch); })
            );

            std::replace(curr.diffuseMapPath.begin(), curr.diffuseMapPath.end(), '\\', '/'); 
        }
        else if ((tag == "map_Bump" || tag == "bump") && has_current)
        {
            std::getline(ls, curr.normalMapPath);

            curr.normalMapPath.erase(
                curr.normalMapPath.begin(),
                std::find_if(curr.normalMapPath.begin(), curr.normalMapPath.end(),
                    [](unsigned char ch) { return !std::isspace(ch); })
            );
            std::replace(curr.normalMapPath.begin(), curr.normalMapPath.end(), '\\', '/');
        }
    }

    flushCurr(); 

    return true; 
}

bool OBJLoader::Load(const std::string& filepath, Model::ModelMesh& outModel, Model::ModelData* outData)
{
    Model::ModelData data;
    data.name = filepath;

    if (!ParseOBJ(filepath, data))
        return false;

    if (!ValidateModelData(data))
        return false;

    outModel.BuildFromData(data);

    if (outData)
    {
        *outData = data; 
    }

    return true;
}

bool OBJLoader::ParseVertexIndex(
    const std::string& vertexStr,
    int& posIndex,
    int& texIndex,
    int& normIndex)
{
    if (vertexStr.empty())
        return false;

    posIndex = INT_MIN;
    texIndex = INT_MIN;
    normIndex = INT_MIN;

    size_t firstSlash = vertexStr.find('/');

    if (firstSlash == std::string::npos)
    {
        try
        {
            posIndex = std::stoi(vertexStr);
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    try
    {
        std::string posStr = vertexStr.substr(0, firstSlash);
        posIndex = std::stoi(posStr);
    }
    catch (...)
    {
        return false;
    }

    size_t secondSlash = vertexStr.find('/', firstSlash + 1);

    if (secondSlash == std::string::npos)
    {
        try
        {
            std::string texStr = vertexStr.substr(firstSlash + 1);
            if (!texStr.empty())
                texIndex = std::stoi(texStr);
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    if (secondSlash > firstSlash + 1)
    {
        try
        {
            std::string texStr = vertexStr.substr(firstSlash + 1, secondSlash - firstSlash - 1);
            texIndex = std::stoi(texStr);
        }
        catch (...)
        {
        }
    }

    if (secondSlash + 1 < vertexStr.length())
    {
        try
        {
            std::string normStr = vertexStr.substr(secondSlash + 1);
            normIndex = std::stoi(normStr);
        }
        catch (...)
        {
            return false;
        }
    }

    return true;
}

void OBJLoader::FixIndices(int& posIndex, int& texIndex, int& normIndex,
    const int posSize, const int texSize, const int normSize)
{
    if (posIndex > 0)
        posIndex--;
    else if (posIndex < 0)
        posIndex = posSize + posIndex;

    if (texIndex == INT_MIN)
    {
        texIndex = -1;
    }
    else if (texIndex > 0)
    {
        texIndex--;
    }
    else if (texIndex < 0)
    {
        texIndex = texSize + texIndex;
    }

    if (normIndex == INT_MIN)
    {
        normIndex = -1;
    }
    else if (normIndex > 0)
    {
        normIndex--;
    }
    else if (normIndex < 0)
    {
        normIndex = normSize + normIndex;
    }
}

static bool FindMTLFileInDir(std::string location, std::string& MTLName, std::string& FoundLocation)
{
    if (!fs::exists(location) || !fs::is_directory(location))
        return false;

    for (auto const& direct : fs::directory_iterator(location))
    {
        if (direct.is_regular_file() &&
            direct.path().extension() == ".mtl" &&
            direct.path().filename() == MTLName
            )
        {
            FoundLocation = direct.path().string();
            return true;
        }
        if (direct.is_directory())
        {
            if (FindMTLFileInDir(direct.path().string(), MTLName, FoundLocation))
            {
                return true;
            }
        }

    }

    return false;
}


bool OBJLoader::ParseOBJ(const std::string& filepath, Model::ModelData& data)
{
    std::ifstream file;
    file.open(filepath);

    if (!file.is_open())
    {
        ReportError("Failed to open: " + filepath);
        return false;
    }

    std::string line;
    int lineNum = 0;
    int vertexCount = 0;
    int normalCount = 0;
    int texCoordCount = 0;
    int faceCount = 0;
    int currentMaterial = -1; 
    std::unordered_map<std::string, int32_t> lookup; 

    while (std::getline(file, line))
    {
        ++lineNum;

        if (line.empty() || line[0] == '#')
            continue;

        std::istringstream ls(line);
        std::string tag;
        ls >> tag;

        if(tag == "mtllib")
        {
            std::string filepath; 
            ls >> filepath; 
            if (filepath.size() < 4 || filepath.substr(filepath.size() - 4) != ".mtl")
                filepath += ".mtl";
            
            std::string locationOfMTL; 
            if (FindMTLFileInDir("./Models", filepath, locationOfMTL))
            {
                if (ParseMTLFile(locationOfMTL, data.materials, lookup))
                {
                    std::println("Object Loader Loaded MTL File Successfully: {}", locationOfMTL); 
                }
            }
        }
        else if (tag == "usemtl")
        {
            std::string name; 
            ls >> name; 
            currentMaterial = (lookup.contains(name)) ? lookup[name] : -1; 
        }
        else if (tag == "v")
        {
            float x, y, z;
            ls >> x >> y >> z;
            data.positions.emplace_back(x, y, z);
            vertexCount++;
        }
        else if (tag == "vn")
        {
            float x, y, z;
            if (ls >> x >> y >> z)
            {
                data.normals.emplace_back(x, y, z);
                data.hasNormals = true;
                normalCount++;
            }
        }
        else if (tag == "vt")
        {
            float u, v;
            if (ls >> u >> v)
            {
                data.texCoords.emplace_back(u, 1.0f - v);
                data.hasTexCoords = true;
                texCoordCount++;
            }
        }
        else if (tag == "f")
        {
            faceCount++;
            std::string each;
            std::vector<Model::ModelData::FaceVertex> faceVerts;

            while (ls >> each)
            {
                int a, b, c; a = b = c = INT_MIN;

                if (ParseVertexIndex(each, a, b, c))
                {
                    if (faceCount == 1)
                    {
                        std::cout << "Face 1 parsing: " << each
                            << " -> pos=" << a << " tex=" << b << " norm=" << c << "\n";
                    }

                    FixIndices(a, b, c, data.positions.size(), data.texCoords.size(), data.normals.size());

                    if (faceCount == 1)
                    {
                        std::cout << "After FixIndices: pos=" << a << " tex=" << b << " norm=" << c << "\n";
                    }

                    Model::ModelData::FaceVertex face;
                    face.positionIndex = a;
                    face.texCoordIndex = b;
                    face.normalIndex = c;

                    faceVerts.push_back(face);
                }
            }

            if (faceVerts.size() >= 3)
            {
                for (size_t i = 1; i < faceVerts.size() - 1; ++i)
                {
                    data.faceVertices.push_back(faceVerts[0]);
                    data.faceVertices.push_back(faceVerts[i]);
                    data.faceVertices.push_back(faceVerts[i + 1]);

                    data.materialIndexPerTriangle.push_back(currentMaterial); 
                }
            }
        }
    }

    file.close();

    std::cout << "Loaded OBJ: \n";
    std::cout << "Vertices: " << vertexCount << "\n";
    std::cout << "Normals: " << normalCount << "\n";
    std::cout << "TexCoords: " << texCoordCount << "\n";
    std::cout << "Faces: " << faceCount << "\n";
    std::cout << "Face vertices: " << data.faceVertices.size() << "\n";

    return true;
}

bool OBJLoader::ValidateModelData(const Model::ModelData& data) const
{
    if (data.positions.empty())
    {
        ReportError("No vertices found. 0x0000F140");
        return false;
    }

    if (data.faceVertices.empty())
    {
        ReportError("No faces found. 0x0000F150");
        return false;
    }

    if (data.faceVertices.size() % 3 != 0)
    {
        ReportError("Face vertex count not divisible by 3. 0x0000F160");
        return false;
    }

    for (const auto& fv : data.faceVertices)
    {
        if (fv.positionIndex < 0 || fv.positionIndex >= data.positions.size())
        {
            ReportError("Face vertex position index " + std::to_string(fv.positionIndex) +
                " out of bounds (max: " + std::to_string(data.positions.size() - 1) + "). 0x0000F170");
            return false;
        }
    }

    return true;
}
