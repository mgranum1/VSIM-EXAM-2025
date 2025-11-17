// ModelLoader.cpp - Refactored to only load model data
#include "ModelLoader.h"
#include "../../External/tiny_obj_loader.h"
#include <qdebug.h>
#include <unordered_map>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

namespace std {
template<> struct hash<Vertex> {
    size_t operator()(Vertex const& vertex) const {
        return ((hash<glm::vec3>()(vertex.pos) ^
                 (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
               (hash<glm::vec2>()(vertex.texCoord) << 1);
    }
};
}

namespace bbl
{
// Equality operator for vertices
bool operator==(const Vertex& a, const Vertex& b) {
    return a.pos == b.pos && a.color == b.color && a.texCoord == b.texCoord;
}

// Helper: safe check for index range
static inline bool safe_attrib_vertex_exists(const tinyobj::attrib_t& attrib,
                                             int vertex_index) {
    if (vertex_index < 0) return false;
    size_t required = static_cast<size_t>(3 * vertex_index + 2);
    return required < attrib.vertices.size();
}

static inline bool safe_attrib_texcoord_exists(const tinyobj::attrib_t& attrib,
                                               int texcoord_index) {
    if (texcoord_index < 0) return false;
    size_t required = static_cast<size_t>(2 * texcoord_index + 1);
    return required < attrib.texcoords.size();
}

std::unique_ptr<ModelData> ModelLoader::loadModel(const std::string& modelPath,
                                                  const std::string& customTexturePath)
{
    auto modelData = std::make_unique<ModelData>();
    modelData->modelPath = modelPath;

    // Extract model name from path
    size_t lastSlash = modelPath.find_last_of("/\\");
    size_t lastDot = modelPath.find_last_of(".");
    if (lastSlash != std::string::npos && lastDot != std::string::npos) {
        modelData->name = modelPath.substr(lastSlash + 1, lastDot - lastSlash - 1);
    } else {
        modelData->name = "unnamed_model";
    }

    // Load the model based on file extension
    loadOBJ(modelPath, *modelData, customTexturePath);

    return modelData;
}

void ModelLoader::loadOBJ(const std::string& modelPath,
                          ModelData& modelData,
                          const std::string& customTexturePath)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
                          modelPath.c_str())) {
        qDebug() << "Model loading failed:" << warn.c_str() << err.c_str();
        throw std::runtime_error(warn + err);
    }

    std::string modelDir = modelPath.substr(0, modelPath.find_last_of("/\\") + 1);

    // Convert materials to our format
    for (const auto& mat : materials)
    {
        MaterialData materialData;

        // Set texture paths
        if (!mat.diffuse_texname.empty()) {
            materialData.diffuseTexturePath = modelDir + mat.diffuse_texname;
        }
        if (!mat.normal_texname.empty()) {
            materialData.normalTexturePath = modelDir + mat.normal_texname;
        }
        if (!mat.specular_texname.empty()) {
            materialData.specularTexturePath = modelDir + mat.specular_texname;
        }

        // Set material properties
        materialData.ambient = glm::vec3(mat.ambient[0], mat.ambient[1], mat.ambient[2]);
        materialData.diffuse = glm::vec3(mat.diffuse[0], mat.diffuse[1], mat.diffuse[2]);
        materialData.specular = glm::vec3(mat.specular[0], mat.specular[1], mat.specular[2]);
        materialData.shininess = mat.shininess;

        modelData.materials.push_back(materialData);
    }

    // Override with custom texture if provided
    if (!customTexturePath.empty()) {
        if (modelData.materials.empty()) {
            MaterialData defaultMaterial;
            defaultMaterial.diffuseTexturePath = customTexturePath;
            modelData.materials.push_back(defaultMaterial);
        } else {
            // Apply custom texture to all materials
            for (auto& mat : modelData.materials) {
                mat.diffuseTexturePath = customTexturePath;
            }
        }
    }

    // Process each shape
    for (size_t s = 0; s < shapes.size(); ++s)
    {
        const auto& shape = shapes[s];

        if (shape.mesh.indices.empty()) {
            qDebug() << "Skipping shape" << (int)s << " — empty indices";
            continue;
        }

        MeshData meshData;

        int chosenMat = -1;
        if (!shape.mesh.material_ids.empty()) {
            for (int id : shape.mesh.material_ids) {
                if (id >= 0) { chosenMat = id; break; }
            }
        }
        if (chosenMat < 0 && !modelData.materials.empty()) {
            qDebug() << "Shape" << (int)s << ": no per-face material; falling back to material 0.";
            chosenMat = 0;
        }
        meshData.materialIndex = chosenMat;

        // Build vertices & indices
        std::unordered_map<Vertex, uint32_t> uniqueVertices;

        for (const auto& index : shape.mesh.indices)
        {
            if (!safe_attrib_vertex_exists(attrib, index.vertex_index)) {
                qDebug() << "Warning: skipping index with invalid vertex_index ="
                         << index.vertex_index;
                continue;
            }

            Vertex vertex{};
            int vi = index.vertex_index;
            vertex.pos = {
                attrib.vertices[3 * vi + 0],
                attrib.vertices[3 * vi + 1],
                attrib.vertices[3 * vi + 2]
            };

            if (safe_attrib_texcoord_exists(attrib, index.texcoord_index)) {
                vertex.texCoord = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1] // Flip Y
                };
            } else {
                vertex.texCoord = {0.0f, 0.0f};
            }

            // Check for vertex colors
            if (index.vertex_index >= 0 &&
                attrib.colors.size() > 3 * index.vertex_index + 2) {
                vertex.color = {
                    attrib.colors[3 * index.vertex_index + 0],
                    attrib.colors[3 * index.vertex_index + 1],
                    attrib.colors[3 * index.vertex_index + 2]
                };
            } else {
                vertex.color = {1.0f, 1.0f, 1.0f};
            }

            // Check for existing vertex or add new one
            auto it = uniqueVertices.find(vertex);
            if (it == uniqueVertices.end()) {
                uint32_t newIndex = static_cast<uint32_t>(meshData.vertices.size());
                uniqueVertices[vertex] = newIndex;
                meshData.vertices.push_back(vertex);
                meshData.indices.push_back(newIndex);
            } else {
                meshData.indices.push_back(it->second);
            }
        }

        qDebug() << "Shape" << (int)s
                 << "vertices:" << meshData.vertices.size()
                 << "indices:" << meshData.indices.size()
                 << "material:" << meshData.materialIndex;

        // Only add mesh if it has valid data
        if (!meshData.vertices.empty() && !meshData.indices.empty()) {
            modelData.meshes.push_back(std::move(meshData));
        } else {
            qDebug() << "Skipping shape" << (int)s
                     << " — produced zero vertices or indices";
        }
    }

    qDebug() << "Model loaded successfully:"
             << "Name:" << modelData.name.c_str()
             << "Meshes:" << modelData.meshes.size()
             << "Materials:" << modelData.materials.size();
}

} // namespace bbl
