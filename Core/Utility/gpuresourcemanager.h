#ifndef GPURESOURCE_MANAGER_H
#define GPURESOURCE_MANAGER_H

#include <vulkan/vulkan.h>
#include <unordered_map>
#include <memory>
#include <string>
#include "ModelData.h"

namespace bbl
{
// Manages all GPU resources separately from ECS, AS IT SHOULD
class GPUResourceManager
{
public:
    GPUResourceManager(VkDevice device,
                    VkPhysicalDevice physicalDevice,
                    VkCommandPool commandPool,
                    VkQueue graphicsQueue);
    ~GPUResourceManager();

    // Resource IDs for referencing GPU resources
    using MeshResourceID = size_t;
    using TextureResourceID = size_t;

    // Upload model data to GPU and get resource IDs
    MeshResourceID uploadMesh(const MeshData& meshData);
    TextureResourceID uploadTexture(const std::string& texturePath);

    // Get GPU resources by ID
    const MeshGPUResources* getMeshResources(MeshResourceID id) const;
    const TextureGPUResources* getTextureResources(TextureResourceID id) const;

    // Clean up specific resources
    void releaseMeshResources(MeshResourceID id);
    void releaseTextureResources(TextureResourceID id);

    // Clean up all resources
    void cleanup();

private:
    VkDevice mDevice;
    VkPhysicalDevice mPhysicalDevice;
    VkCommandPool mCommandPool;
    VkQueue mGraphicsQueue;

    // Storage for GPU resources
    std::unordered_map<MeshResourceID, std::unique_ptr<MeshGPUResources>> mMeshResources;
    std::unordered_map<TextureResourceID, std::unique_ptr<TextureGPUResources>> mTextureResources;

    // Counter for generating unique IDs
    MeshResourceID mNextMeshID = 0;
    TextureResourceID mNextTextureID = 0;

    // Texture cache to avoid loading the same texture multiple times
    std::unordered_map<std::string, TextureResourceID> mTexturePathCache;

    // Vulkan helper functions (moved from ModelLoader)
    void createVertexBuffer(const std::vector<Vertex>& vertices,
                            VkBuffer& buffer,
                            VkDeviceMemory& memory);
    void createIndexBuffer(const std::vector<uint32_t>& indices,
                           VkBuffer& buffer,
                           VkDeviceMemory& memory);
    void createTextureImage(const std::string& path,
                            VkImage& image,
                            VkDeviceMemory& memory);
    void createTextureImageView(VkImage image, VkImageView& view);
    void createTextureSampler(VkSampler& sampler);

    // Helper functions
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    void transitionImageLayout(VkImage image, VkFormat format,
                               VkImageLayout oldLayout, VkImageLayout newLayout);
    void copyBufferToImage(VkBuffer buffer, VkImage image,
                           uint32_t width, uint32_t height);
    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
};
}

#endif // GPURESOURCE_MANAGER_H
