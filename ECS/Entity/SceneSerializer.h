#ifndef SCENESERIALIZER_H
#define SCENESERIALIZER_H

#include "Entity.h"
#include "EntityManager.h"
#include "../Components/Components.h"
#include "json.hpp"
#include <string>
#include <fstream>
#include <memory>

namespace bbl
{
class GPUResourceManager;

using json = nlohmann::json;

class SceneSerializer
{
public:
    SceneSerializer() = default;
    ~SceneSerializer() = default;

    bool saveScene(const EntityManager* entityManager,const std::string& filepath,const std::string& sceneName);

    void saveEntityNames(const std::unordered_map<EntityID, std::string>& entityNames,json& sceneJson);

    bool loadScene(EntityManager* entityManager,
                   GPUResourceManager* gpuResources,
                   const std::string& filepath,
                   std::unordered_map<EntityID, std::string>* outEntityNames);

    std::string getLastError() const { return mLastError; }

    bool validateSceneFile(const std::string& filepath);

private:
    std::string mLastError;

    json serializeTransform(const Transform& transform);
    json serializeMesh(const Mesh& mesh);
    json serializeTexture(const Texture& texture);
    json serializeRender(const Render& render);
    json serializeAudio(const Audio& audio);
    json serializePhysics(const Physics& physics);
    json serializeCollision(const Collision& collision);

    Transform deserializeTransform(const json& j);
    Mesh deserializeMesh(const json& j);
    Texture deserializeTexture(const json& j);
    Render deserializeRender(const json& j);
    Audio deserializeAudio(const json& j);
    Physics deserializePhysics(const json& j);
    Collision deserializeCollision(const json& j);


    json serializeEntity(const EntityManager* entityManager, EntityID entityID);


    void deserializeEntity(EntityManager* entityManager, const json& entityJson);

    void setError(const std::string& error) { mLastError = error; }
    void clearError() { mLastError.clear(); }
};

}
#endif
