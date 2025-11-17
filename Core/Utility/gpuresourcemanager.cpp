#include "gpuresourcemanager.h"
#include "../../External/stb_image.h"
#include <iostream>
#include <ostream>
#include <qdebug.h>
#include <cstring>
#include <stdexcept>

namespace bbl
{
GPUResourceManager::GPUResourceManager(VkDevice device,
                                 VkPhysicalDevice physicalDevice,
                                 VkCommandPool commandPool,
                                 VkQueue graphicsQueue)
    : mDevice(device)
    , mPhysicalDevice(physicalDevice)
    , mCommandPool(commandPool)
    , mGraphicsQueue(graphicsQueue)
{
}

GPUResourceManager::~GPUResourceManager()
{
    // cleanup();
    // qDebug() << "Destroyed GPUResourceManager";
}

GPUResourceManager::MeshResourceID GPUResourceManager::uploadMesh(const MeshData& meshData)
{
    if (meshData.vertices.empty() || meshData.indices.empty()) {
        qDebug() << "uploadMesh: Empty mesh data provided";
        return 0;
    }

    auto meshResources = std::make_unique<MeshGPUResources>();

    // Create vertex buffer
    createVertexBuffer(meshData.vertices,
                       meshResources->vertexBuffer,
                       meshResources->vertexBufferMemory);

    // Create index buffer
    createIndexBuffer(meshData.indices,
                      meshResources->indexBuffer,
                      meshResources->indexBufferMemory);

    meshResources->vertexCount = meshData.vertices.size();
    meshResources->indexCount = meshData.indices.size();

    // Generate unique ID and store
    MeshResourceID id = mNextMeshID++;
    mMeshResources[id] = std::move(meshResources);

    qDebug() << "Uploaded mesh with ID:" << id
             << "Vertices:" << meshData.vertices.size()
             << "Indices:" << meshData.indices.size();

    return id;
}

GPUResourceManager::TextureResourceID GPUResourceManager::uploadTexture(const std::string& texturePath)
{
    // Check cache first
    auto cacheIt = mTexturePathCache.find(texturePath);
    if (cacheIt != mTexturePathCache.end()) {
        qDebug() << "Texture already loaded, returning cached ID:" << cacheIt->second;
        return cacheIt->second;
    }

    auto textureResources = std::make_unique<TextureGPUResources>();

    // Create texture image
    createTextureImage(texturePath,
                       textureResources->textureImage,
                       textureResources->textureImageMemory);

    if (textureResources->textureImage != VK_NULL_HANDLE) {
        // Create image view
        createTextureImageView(textureResources->textureImage,
                               textureResources->textureImageView);

        // Create sampler
        createTextureSampler(textureResources->textureSampler);
    }

    // Generate unique ID and store
    TextureResourceID id = mNextTextureID++;
    mTextureResources[id] = std::move(textureResources);

    // Cache the path
    mTexturePathCache[texturePath] = id;

    qDebug() << "Uploaded texture with ID:" << id << "Path:" << texturePath.c_str();

    return id;
}

const MeshGPUResources* GPUResourceManager::getMeshResources(MeshResourceID id) const
{
    auto it = mMeshResources.find(id);
    if (it != mMeshResources.end()) {
        return it->second.get();
    }
    return nullptr;
}

const TextureGPUResources* GPUResourceManager::getTextureResources(TextureResourceID id) const
{
    auto it = mTextureResources.find(id);
    if (it != mTextureResources.end()) {
        return it->second.get();
    }
    return nullptr;
}

void GPUResourceManager::releaseMeshResources(MeshResourceID id)
{
    auto it = mMeshResources.find(id);
    if (it != mMeshResources.end()) {
        auto& resources = it->second;

        if (resources->indexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(mDevice, resources->indexBuffer, nullptr);
        }
        if (resources->indexBufferMemory != VK_NULL_HANDLE) {
            vkFreeMemory(mDevice, resources->indexBufferMemory, nullptr);
        }
        if (resources->vertexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(mDevice, resources->vertexBuffer, nullptr);
        }
        if (resources->vertexBufferMemory != VK_NULL_HANDLE) {
            vkFreeMemory(mDevice, resources->vertexBufferMemory, nullptr);
        }

        mMeshResources.erase(it);
        qDebug() << "Released mesh resources for ID:" << id;
    }
}

void GPUResourceManager::releaseTextureResources(TextureResourceID id)
{
    auto it = mTextureResources.find(id);
    if (it != mTextureResources.end()) {
        auto& resources = it->second;

        if (resources->textureSampler != VK_NULL_HANDLE) {
            vkDestroySampler(mDevice, resources->textureSampler, nullptr);
        }
        if (resources->textureImageView != VK_NULL_HANDLE) {
            vkDestroyImageView(mDevice, resources->textureImageView, nullptr);
        }
        if (resources->textureImage != VK_NULL_HANDLE) {
            vkDestroyImage(mDevice, resources->textureImage, nullptr);
        }
        if (resources->textureImageMemory != VK_NULL_HANDLE) {
            vkFreeMemory(mDevice, resources->textureImageMemory, nullptr);
        }

        // Remove from cache
        for (auto cacheIt = mTexturePathCache.begin(); cacheIt != mTexturePathCache.end(); ++cacheIt) {
            if (cacheIt->second == id) {
                mTexturePathCache.erase(cacheIt);
                break;
            }
        }

        mTextureResources.erase(it);
        qDebug() << "Released texture resources for ID:" << id;
    }
}

void GPUResourceManager::cleanup()
{
    std::cout << "GPUResourceManager::cleanup() called" << std::endl;
    std::cout << "Mesh resources count: " << mMeshResources.size() << std::endl;
    std::cout << "Texture resources count: " << mTextureResources.size() << std::endl;

    // Clean up all mesh resources
    for (auto& pair : mMeshResources) {
        auto& resources = pair.second;
        std::cout << "Cleaning up mesh ID: " << pair.first << std::endl;

        if (resources->indexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(mDevice, resources->indexBuffer, nullptr);
            std::cout << "  Destroyed index buffer" << std::endl;
        }
        if (resources->indexBufferMemory != VK_NULL_HANDLE) {
            vkFreeMemory(mDevice, resources->indexBufferMemory, nullptr);
            std::cout << "  Freed index buffer memory" << std::endl;
        }
        if (resources->vertexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(mDevice, resources->vertexBuffer, nullptr);
            std::cout << "  Destroyed vertex buffer" << std::endl;
        }
        if (resources->vertexBufferMemory != VK_NULL_HANDLE) {
            vkFreeMemory(mDevice, resources->vertexBufferMemory, nullptr);
            std::cout << "  Freed vertex buffer memory" << std::endl;
        }
    }
    mMeshResources.clear();

    //Clean up all texture resources
    for (auto& pair : mTextureResources) {
        auto& resources = pair.second;
        std::cout << "Cleaning up texture ID: " << pair.first << std::endl;

        if (resources->textureSampler != VK_NULL_HANDLE) {
            vkDestroySampler(mDevice, resources->textureSampler, nullptr);
            std::cout << "  Destroyed sampler" << std::endl;
        }
        if (resources->textureImageView != VK_NULL_HANDLE) {
            vkDestroyImageView(mDevice, resources->textureImageView, nullptr);
            std::cout << "  Destroyed image view" << std::endl;
        }
        if (resources->textureImage != VK_NULL_HANDLE) {
            vkDestroyImage(mDevice, resources->textureImage, nullptr);
            std::cout << "  Destroyed image" << std::endl;  // THIS IS IMPORTANT
        }
        if (resources->textureImageMemory != VK_NULL_HANDLE) {
            vkFreeMemory(mDevice, resources->textureImageMemory, nullptr);
            std::cout << "  Freed image memory" << std::endl;  // THIS IS IMPORTANT
        }
    }

    std::cout << "GPUResourceManager cleanup complete" << std::endl;
}

// ============= Vulkan Helper Functions (moved from ModelLoader) =============

void GPUResourceManager::createVertexBuffer(const std::vector<Vertex>& vertices,
                                         VkBuffer& buffer,
                                         VkDeviceMemory& memory)
{
    if (vertices.empty()) {
        qDebug() << "createVertexBuffer: empty vertex array, skipping buffer creation.";
        buffer = VK_NULL_HANDLE;
        memory = VK_NULL_HANDLE;
        return;
    }

    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
    if (bufferSize == 0) {
        qDebug() << "createVertexBuffer: computed bufferSize == 0, skipping.";
        buffer = VK_NULL_HANDLE;
        memory = VK_NULL_HANDLE;
        return;
    }

    qDebug() << "createVertexBuffer: bufferSize =" << static_cast<qulonglong>(bufferSize);

    // Create staging buffer
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(mDevice, &bufferInfo, nullptr, &stagingBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create staging buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(mDevice, stagingBuffer, &memRequirements);
    qDebug() << "createVertexBuffer: staging memRequirements.size ="
             << static_cast<qulonglong>(memRequirements.size);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
                                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(mDevice, &allocInfo, nullptr, &stagingBufferMemory) != VK_SUCCESS) {
        vkDestroyBuffer(mDevice, stagingBuffer, nullptr);
        throw std::runtime_error("failed to allocate staging buffer memory!");
    }

    vkBindBufferMemory(mDevice, stagingBuffer, stagingBufferMemory, 0);

    // Copy vertex data to staging buffer
    void* data = nullptr;
    vkMapMemory(mDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(mDevice, stagingBufferMemory);

    // Create device local vertex buffer
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    if (vkCreateBuffer(mDevice, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        vkDestroyBuffer(mDevice, stagingBuffer, nullptr);
        vkFreeMemory(mDevice, stagingBufferMemory, nullptr);
        throw std::runtime_error("failed to create vertex buffer!");
    }

    vkGetBufferMemoryRequirements(mDevice, buffer, &memRequirements);
    qDebug() << "createVertexBuffer: vertex memRequirements.size ="
             << static_cast<qulonglong>(memRequirements.size);
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
                                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(mDevice, &allocInfo, nullptr, &memory) != VK_SUCCESS) {
        vkDestroyBuffer(mDevice, buffer, nullptr);
        vkDestroyBuffer(mDevice, stagingBuffer, nullptr);
        vkFreeMemory(mDevice, stagingBufferMemory, nullptr);
        throw std::runtime_error("failed to allocate vertex buffer memory!");
    }

    vkBindBufferMemory(mDevice, buffer, memory, 0);

    // Copy from staging to device local buffer
    copyBuffer(stagingBuffer, buffer, bufferSize);

    // Clean up staging buffer
    vkDestroyBuffer(mDevice, stagingBuffer, nullptr);
    vkFreeMemory(mDevice, stagingBufferMemory, nullptr);
}

void GPUResourceManager::createIndexBuffer(const std::vector<uint32_t>& indices,
                                        VkBuffer& buffer,
                                        VkDeviceMemory& memory)
{
    if (indices.empty()) {
        qDebug() << "createIndexBuffer: empty index array, skipping buffer creation.";
        buffer = VK_NULL_HANDLE;
        memory = VK_NULL_HANDLE;
        return;
    }

    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
    if (bufferSize == 0) {
        qDebug() << "createIndexBuffer: computed bufferSize == 0, skipping.";
        buffer = VK_NULL_HANDLE;
        memory = VK_NULL_HANDLE;
        return;
    }

    qDebug() << "createIndexBuffer: bufferSize =" << static_cast<qulonglong>(bufferSize);

    // Create staging buffer
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(mDevice, &bufferInfo, nullptr, &stagingBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create staging buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(mDevice, stagingBuffer, &memRequirements);
    qDebug() << "createIndexBuffer: staging memRequirements.size ="
             << static_cast<qulonglong>(memRequirements.size);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
                                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(mDevice, &allocInfo, nullptr, &stagingBufferMemory) != VK_SUCCESS) {
        vkDestroyBuffer(mDevice, stagingBuffer, nullptr);
        throw std::runtime_error("failed to allocate staging buffer memory!");
    }

    vkBindBufferMemory(mDevice, stagingBuffer, stagingBufferMemory, 0);

    // Copy index data to staging buffer
    void* data = nullptr;
    vkMapMemory(mDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, indices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(mDevice, stagingBufferMemory);

    // Create device local index buffer
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    if (vkCreateBuffer(mDevice, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        vkDestroyBuffer(mDevice, stagingBuffer, nullptr);
        vkFreeMemory(mDevice, stagingBufferMemory, nullptr);
        throw std::runtime_error("failed to create index buffer!");
    }

    vkGetBufferMemoryRequirements(mDevice, buffer, &memRequirements);
    qDebug() << "createIndexBuffer: index memRequirements.size ="
             << static_cast<qulonglong>(memRequirements.size);
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
                                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(mDevice, &allocInfo, nullptr, &memory) != VK_SUCCESS) {
        vkDestroyBuffer(mDevice, buffer, nullptr);
        vkDestroyBuffer(mDevice, stagingBuffer, nullptr);
        vkFreeMemory(mDevice, stagingBufferMemory, nullptr);
        throw std::runtime_error("failed to allocate index buffer memory!");
    }

    vkBindBufferMemory(mDevice, buffer, memory, 0);

    // Copy from staging to device local buffer
    copyBuffer(stagingBuffer, buffer, bufferSize);

    // Clean up staging buffer
    vkDestroyBuffer(mDevice, stagingBuffer, nullptr);
    vkFreeMemory(mDevice, stagingBufferMemory, nullptr);
}

void GPUResourceManager::createTextureImage(const std::string& path,
                                         VkImage& image,
                                         VkDeviceMemory& memory)
{
    image = VK_NULL_HANDLE;
    memory = VK_NULL_HANDLE;

    int texWidth = 0, texHeight = 0, texChannels = 0;
    stbi_uc* pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    if (!pixels || texWidth <= 0 || texHeight <= 0) {
        qDebug() << "createTextureImage: Failed to load texture or invalid dimensions for"
                 << path.c_str();
        if (pixels) stbi_image_free(pixels);
        return;
    }

    VkDeviceSize imageSize = static_cast<VkDeviceSize>(texWidth) *
                             static_cast<VkDeviceSize>(texHeight) * 4;
    if (imageSize == 0) {
        qDebug() << "createTextureImage: imageSize is zero, skipping texture for"
                 << path.c_str();
        stbi_image_free(pixels);
        return;
    }

    qDebug() << "createTextureImage: imageSize =" << static_cast<qulonglong>(imageSize)
             << "for" << path.c_str();

    // Create staging buffer
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = imageSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(mDevice, &bufferInfo, nullptr, &stagingBuffer) != VK_SUCCESS) {
        stbi_image_free(pixels);
        throw std::runtime_error("failed to create staging buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(mDevice, stagingBuffer, &memRequirements);
    qDebug() << "createTextureImage: staging memRequirements.size ="
             << static_cast<qulonglong>(memRequirements.size);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
                                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(mDevice, &allocInfo, nullptr, &stagingBufferMemory) != VK_SUCCESS) {
        vkDestroyBuffer(mDevice, stagingBuffer, nullptr);
        stbi_image_free(pixels);
        throw std::runtime_error("failed to allocate staging buffer memory!");
    }

    vkBindBufferMemory(mDevice, stagingBuffer, stagingBufferMemory, 0);

    // Copy pixel data to staging buffer
    void* data = nullptr;
    vkMapMemory(mDevice, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(mDevice, stagingBufferMemory);

    stbi_image_free(pixels);

    // Create image
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = static_cast<uint32_t>(texWidth);
    imageInfo.extent.height = static_cast<uint32_t>(texHeight);
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

    if (vkCreateImage(mDevice, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        vkDestroyBuffer(mDevice, stagingBuffer, nullptr);
        vkFreeMemory(mDevice, stagingBufferMemory, nullptr);
        throw std::runtime_error("failed to create image!");
    }

    vkGetImageMemoryRequirements(mDevice, image, &memRequirements);
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
                                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(mDevice, &allocInfo, nullptr, &memory) != VK_SUCCESS) {
        vkDestroyImage(mDevice, image, nullptr);
        vkDestroyBuffer(mDevice, stagingBuffer, nullptr);
        vkFreeMemory(mDevice, stagingBufferMemory, nullptr);
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(mDevice, image, memory, 0);

    // Transition image layout and copy buffer to image
    transitionImageLayout(image, VK_FORMAT_R8G8B8A8_SRGB,
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copyBufferToImage(stagingBuffer, image,
                      static_cast<uint32_t>(texWidth),
                      static_cast<uint32_t>(texHeight));
    transitionImageLayout(image, VK_FORMAT_R8G8B8A8_SRGB,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    // Clean up staging buffer
    vkDestroyBuffer(mDevice, stagingBuffer, nullptr);
    vkFreeMemory(mDevice, stagingBufferMemory, nullptr);
}

void GPUResourceManager::createTextureImageView(VkImage image, VkImageView& view)
{
    view = VK_NULL_HANDLE;
    if (image == VK_NULL_HANDLE) {
        qDebug() << "createTextureImageView: image is VK_NULL_HANDLE, skipping view creation.";
        return;
    }

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(mDevice, &viewInfo, nullptr, &view) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture image view!");
    }
}

void GPUResourceManager::createTextureSampler(VkSampler& sampler)
{
    sampler = VK_NULL_HANDLE;

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(mPhysicalDevice, &properties);
    float maxAniso = properties.limits.maxSamplerAnisotropy > 0.0f ?
                         properties.limits.maxSamplerAnisotropy : 1.0f;
    samplerInfo.maxAnisotropy = maxAniso;

    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    if (vkCreateSampler(mDevice, &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }
}

void GPUResourceManager::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    if (srcBuffer == VK_NULL_HANDLE || dstBuffer == VK_NULL_HANDLE || size == 0) {
        qDebug() << "copyBuffer: invalid arguments, skipping copy";
        return;
    }

    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    endSingleTimeCommands(commandBuffer);
}

void GPUResourceManager::transitionImageLayout(VkImage image, VkFormat format,
                                            VkImageLayout oldLayout,
                                            VkImageLayout newLayout)
{
    if (image == VK_NULL_HANDLE) {
        qDebug() << "transitionImageLayout: image is VK_NULL_HANDLE, skipping.";
        return;
    }

    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
        newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
               newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        throw std::invalid_argument("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
        );

    endSingleTimeCommands(commandBuffer);
}

void GPUResourceManager::copyBufferToImage(VkBuffer buffer, VkImage image,
                                        uint32_t width, uint32_t height)
{
    if (buffer == VK_NULL_HANDLE || image == VK_NULL_HANDLE || width == 0 || height == 0) {
        qDebug() << "copyBufferToImage: invalid arguments, skipping.";
        return;
    }

    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};

    vkCmdCopyBufferToImage(
        commandBuffer,
        buffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region
        );

    endSingleTimeCommands(commandBuffer);
}

VkCommandBuffer GPUResourceManager::beginSingleTimeCommands()
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = mCommandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    vkAllocateCommandBuffers(mDevice, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    return commandBuffer;
}

void GPUResourceManager::endSingleTimeCommands(VkCommandBuffer commandBuffer)
{
    if (commandBuffer == VK_NULL_HANDLE) return;

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(mGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(mGraphicsQueue);

    vkFreeCommandBuffers(mDevice, mCommandPool, 1, &commandBuffer);
}

uint32_t GPUResourceManager::findMemoryType(uint32_t typeFilter,
                                         VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(mPhysicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

} // namespace bbl
