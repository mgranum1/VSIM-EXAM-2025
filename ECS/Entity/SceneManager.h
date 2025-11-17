#ifndef SCENEMANAGER_H
#define SCENEMANAGER_H

#include "Entity.h"
#include "EntityManager.h"
#include "SceneSerializer.h"
#include <string>
#include <memory>
#include <unordered_map>
#include <functional>
namespace bbl
{


struct sceneInfo
{
    std::string name;
    std::string filepath;
    bool isLoaded {false};
    int entityCount{0};
};
class GPUResourceManager;
class SceneManager
{
public:
    explicit SceneManager(EntityManager* entityManager, GPUResourceManager* gpuResources);
    ~SceneManager() = default;

    bool createNewScene(const std::string& sceneName);
    bool saveCurrentScene(const std::string& filepath);
    bool saveCurrentScene();
    bool loadScene(const std::string& filepath, bool clearCurrent = true);

    bool reloadCurrentScene();
    void unloadCurrentScene();


    const sceneInfo& getCurrentSceneInfo() const {return mCurrentScene; }
    std::string getCurrentSceneName() const {return mCurrentScene.name; }
    std::string getCurrentSceneFilepath() const {return mCurrentScene.filepath; }
    bool hasSceneLoaded() const {return mCurrentScene.isLoaded; }
    bool hasUnsavedChanges() const {return mHasUnsavedChanges; }


    void markSceneDirty() {mHasUnsavedChanges = true; }


    void markSceneClean() {mHasUnsavedChanges = false; }

    void setEntityName(EntityID entityID, const std::string& name);
    std::string getEntityName(EntityID entityID) const;

    const std::unordered_map<EntityID, std::string>& getEntityNames() const {
        return mEntityNames;
    }

    std::unordered_map<EntityID, std::string>& getEntityNamesMap(){
        return mEntityNames;
    }

    void setOnSceneCallback(std::function<void()> callback){
        mOnSceneLoad = callback;
    }

    void setOnSceneLoadCallback(std::function<void()> callback) {
        mOnSceneLoad = callback;
    }

    std::string getLastError() const;

    bool validateSceneFile(const std::string& filepath);

private:
    EntityManager* mEntityManager = nullptr;
    GPUResourceManager* mGPUResources = nullptr;
    SceneSerializer mSerializer;
    sceneInfo mCurrentScene;
    bool mHasUnsavedChanges{false};

    std::unordered_map<EntityID, std::string> mEntityNames;

    std::function<void()> mOnSceneUnload;
    std::function<void()> mOnSceneLoad;

    void updateSceneInfo();

};
}

#endif // SCENEMANAGER_H
