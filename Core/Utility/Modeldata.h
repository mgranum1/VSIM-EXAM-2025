#ifndef MODEL_DATA_H
#define MODEL_DATA_H

#include <vector>
#include <string>
#include <vulkan/vulkan.h>
#include "Vertex.h"
#include <glm/glm.hpp>

namespace bbl
{
// Pure data structure for a loaded mesh (no Vulkan resources)
struct MeshData
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    int materialIndex = -1;  // Index into materials array if applicable
};

// Pure data structure for material/texture paths
struct MaterialData
{
    std::string diffuseTexturePath;
    std::string normalTexturePath;  // For future use
    std::string specularTexturePath; // For future use
    glm::vec3 ambient{1.0f};        //for future use
    glm::vec3 diffuse{1.0f};        //for future use
    glm::vec3 specular{1.0f};       //for future use
    float shininess = 32.0f;       //for future use
};

// Container for a complete loaded model
struct ModelData
{
    std::vector<MeshData> meshes;
    std::vector<MaterialData> materials;
    std::string modelPath;
    std::string name;
};

// Vulkan resources that will be managed separately from components
struct MeshGPUResources
{
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    VkBuffer indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;
    size_t indexCount = 0;
    size_t vertexCount = 0;
};

struct TextureGPUResources
{
    VkImage textureImage = VK_NULL_HANDLE;
    VkDeviceMemory textureImageMemory = VK_NULL_HANDLE;
    VkImageView textureImageView = VK_NULL_HANDLE;
    VkSampler textureSampler = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
};
}

#endif // MODEL_DATA_H
