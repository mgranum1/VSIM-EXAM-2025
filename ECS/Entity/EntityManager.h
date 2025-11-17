#ifndef ENTITYMANAGER_H
#define ENTITYMANAGER_H

#include "Entity.h"
#include "../Components/Components.h"
#include "../../Core/Utility/gpuresourcemanager.h"
#include "../../Core/Utility/ModelData.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <algorithm>
#include <memory>
#include <type_traits>

namespace bbl
{
class EntityManager
{
public:
    explicit EntityManager(GPUResourceManager* gpuResources = nullptr)
        : mResourceManager(gpuResources)
    {
    }

    ~EntityManager() = default;

    // ===== ENTITY MANAGEMENT =====

    EntityID createEntity()
    {
        EntityID entity = EntityIDGenerator::generateID();
        mActiveEntities.insert(entity);
        return entity;
    }

    //added for saving/loading
    EntityID createEntityWithID(EntityID desiredID)
    {
        EntityID entity = EntityIDGenerator::generateSpecificID(desiredID);
        mActiveEntities.insert(entity);
        return entity;
    }

    EntityID createEntityFromMesh(const MeshData& meshData,
                                  const glm::vec3& position = glm::vec3(0.0f))
    {
        EntityID entity = createEntity();

        // Add transform component
        addComponent(entity, Transform{position, glm::vec3(0.0f), glm::vec3(1.0f)});

        // Add mesh and render components if we have valid mesh data and resource manager
        if (mResourceManager && !meshData.vertices.empty() && !meshData.indices.empty()) {
            // Upload mesh to GPU
            auto meshResourceID = mResourceManager->uploadMesh(meshData);

            // Create mesh component
            addComponent(entity, Mesh{meshResourceID});

            // Create render component
            addComponent(entity, Render{meshResourceID, 0, true, false, 1.0f});
        }

        return entity;
    }

    void destroyEntity(EntityID entity) {
        if (!isValidEntity(entity)) {
            return;
        }

        // Clean up GPU resources if we have them
        if (mResourceManager) {
            // Check for mesh component
            if (auto* meshComp = getComponent<Mesh>(entity)) {
                mResourceManager->releaseMeshResources(meshComp->meshResourceID);
            }

            // Check for texture component
            if (auto* texComp = getComponent<Texture>(entity)) {
                mResourceManager->releaseTextureResources(texComp->textureResourceID);
            }

            // Check for render component (might have both mesh and texture)
            if (auto* renderComp = getComponent<Render>(entity)) {
                mResourceManager->releaseMeshResources(renderComp->meshResourceID);
                if (renderComp->textureResourceID != 0) {
                    mResourceManager->releaseTextureResources(renderComp->textureResourceID);
                }
            }
        }

        // Remove all components for this entity
        removeAllComponents(entity);

        // Remove from active entities
        mActiveEntities.erase(entity);
    }

    bool isValidEntity(EntityID entity) const {
        return entity != INVALID_ENTITY && mActiveEntities.count(entity) > 0;
    }

    // ===== COMPONENT MANAGEMENT =====

    template<typename T>
    void addComponent(EntityID entity, const T& component)
    {
        if (!isValidEntity(entity)) {
            return;
        }
        getComponentMap<T>()[entity] = component;
    }

    template<typename T>
    void removeComponent(EntityID entity)
    {
        if (!isValidEntity(entity)) {
            return;
        }
        getComponentMap<T>().erase(entity);
    }

    template<typename T>
    T* getComponent(EntityID entity)
    {
        if (!isValidEntity(entity)) {
            return nullptr;
        }

        auto& map = getComponentMap<T>();
        auto it = map.find(entity);
        return it != map.end() ? &it->second : nullptr;
    }

    template<typename T>
    const T* getComponent(EntityID entity) const
    {
        if (!isValidEntity(entity)) {
            return nullptr;
        }

        const auto& map = getComponentMap<T>();
        auto it = map.find(entity);
        return it != map.end() ? &it->second : nullptr;
    }

    template<typename T>
    bool hasComponent(EntityID entity) const
    {
        if (!isValidEntity(entity)) {
            return false;
        }
        return getComponentMap<T>().count(entity) > 0;
    }

    // ===== SYSTEM SUPPORT - GET ALL COMPONENTS OF A TYPE =====

    template<typename T>
    std::unordered_map<EntityID, T>& getComponentMap();

    template<typename T>
    const std::unordered_map<EntityID, T>& getComponentMap() const {
        return const_cast<EntityManager*>(this)->getComponentMap<T>();
    }

    // ===== UTILITY FUNCTIONS =====

    size_t getEntityCount() const {
        return mActiveEntities.size();
    }

    std::vector<EntityID> getAllEntities() const {
        return std::vector<EntityID>(mActiveEntities.begin(), mActiveEntities.end());
    }

    // Get entities that have specific component combinations
    template<typename... Components>
    std::vector<EntityID> getEntitiesWith() const {
        std::vector<EntityID> result;

        for (EntityID entity : mActiveEntities) {
            if ((hasComponent<Components>(entity) && ...)) {
                result.push_back(entity);
            }
        }

        return result;
    }

    void setGPUResourceManager(GPUResourceManager* rm) {
        mResourceManager = rm;
    }

    GPUResourceManager* getGPUResourceManager() const {
        return mResourceManager;
    }

    // Clear all entities and components
    void clear() {
        // Clean up GPU resources for all entities
        if (mResourceManager) {
            for (EntityID entity : mActiveEntities) {
                // Clean up mesh resources
                if (auto* meshComp = getComponent<Mesh>(entity)) {
                    mResourceManager->releaseMeshResources(meshComp->meshResourceID);
                }
                if (auto* texComp = getComponent<Texture>(entity)) {
                    mResourceManager->releaseTextureResources(texComp->textureResourceID);
                }
                if (auto* renderComp = getComponent<Render>(entity)) {
                    mResourceManager->releaseMeshResources(renderComp->meshResourceID);
                    if (renderComp->textureResourceID != 0) {
                        mResourceManager->releaseTextureResources(renderComp->textureResourceID);
                    }
                }
            }
        }

        // Clear all component storage
        mTransforms.clear();
        mMeshes.clear();
        mTextures.clear();
        mRenders.clear();
        mAudios.clear();
        mPhysicsComponents.clear();
        mCollisions.clear();
        //mInputs.clear();

        mActiveEntities.clear();
    }

private:
    // ===== COMPONENT STORAGE =====
    // Each component type gets its own map from EntityID to component
    std::unordered_map<EntityID, Transform> mTransforms;
    std::unordered_map<EntityID, Mesh> mMeshes;
    std::unordered_map<EntityID, Texture> mTextures;
    std::unordered_map<EntityID, Render> mRenders;
    std::unordered_map<EntityID, Audio> mAudios;
    std::unordered_map<EntityID, Physics> mPhysicsComponents;
    std::unordered_map<EntityID, Collision> mCollisions;
    //std::unordered_map<EntityID, Input> mInputs;

    // Track active entities
    std::unordered_set<EntityID> mActiveEntities;

    // GPU resource manager
    GPUResourceManager* mResourceManager;

    void removeAllComponents(EntityID entity) {
        mTransforms.erase(entity);
        mMeshes.erase(entity);
        mTextures.erase(entity);
        mRenders.erase(entity);
        mAudios.erase(entity);
        mPhysicsComponents.erase(entity);
        mCollisions.erase(entity);
        //mInputs.erase(entity);
    }
};

// ===== TEMPLATE SPECIALIZATIONS =====
// Define these outside the class to avoid compilation issues

template<>
inline std::unordered_map<EntityID, Transform>& EntityManager::getComponentMap<Transform>() {
    return mTransforms;
}

template<>
inline std::unordered_map<EntityID, Mesh>& EntityManager::getComponentMap<Mesh>() {
    return mMeshes;
}

template<>
inline std::unordered_map<EntityID, Texture>& EntityManager::getComponentMap<Texture>() {
    return mTextures;
}

template<>
inline std::unordered_map<EntityID, Render>& EntityManager::getComponentMap<Render>() {
    return mRenders;
}

template<>
inline std::unordered_map<EntityID, Audio>& EntityManager::getComponentMap<Audio>() {
    return mAudios;
}

template<>
inline std::unordered_map<EntityID, Physics>& EntityManager::getComponentMap<Physics>() {
    return mPhysicsComponents;
}

template<>
inline std::unordered_map<EntityID, Collision>& EntityManager::getComponentMap<Collision>() {
    return mCollisions;
}

// template<>
// inline std::unordered_map<EntityID, Input>& EntityManager::getComponentMap<Input>() {
//     return mInputs;
// }

} // namespace bbl

#endif // ENTITYMANAGER_H
