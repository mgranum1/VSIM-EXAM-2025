#include "SceneManager.h"
#include "../Core/Utility/gpuresourcemanager.h"
#include "qDebug"
#include "QFileInfo"

namespace bbl
{



SceneManager::SceneManager(EntityManager* entityManager, GPUResourceManager* gpuResources)
    : mEntityManager(entityManager),mGPUResources(gpuResources)
{
    if (!mEntityManager) {
        qCritical() << "SceneManager created with null EntityManager!";
    }
    if (!mGPUResources) {
        qCritical() << "SceneManager created with null GPUResourceManager!";
    }

    mCurrentScene.name = "Untitled Scene";
    mCurrentScene.filepath = "";
    mCurrentScene.isLoaded = false;
    mCurrentScene.entityCount = 0;
}


bool SceneManager::createNewScene(const std::string& sceneName)
{
    if (!mEntityManager) {
        qCritical() << "Cannot create new scene: EntityManager is null";
        return false;
    }

    if(mOnSceneUnload){
        mOnSceneUnload();
    }

    mEntityManager->clear();
    mEntityNames.clear();

    mCurrentScene.name = sceneName;
    mCurrentScene.filepath = "";
    mCurrentScene.isLoaded = true;
    mCurrentScene.entityCount = 0;
    mHasUnsavedChanges = true;

    qInfo() << "New Scene was Created: " << QString::fromStdString(sceneName);

    if (mOnSceneLoad){
        mOnSceneLoad();
    }

    return true;
}


bool SceneManager::saveCurrentScene(const std::string& filepath)
{
    if (!mEntityManager){
        qCritical() <<"Cannot save the Scene";
        return false;
    }

    qDebug() << "SceneManager: About to call saveScene with" << mEntityManager->getEntityCount() << "entities";


    bool success = mSerializer.saveScene(mEntityManager, filepath, mCurrentScene.name);

    if (success) {

        std::ifstream file(filepath);
        if (!file.is_open()) {
            qCritical() << "Failed to reopen file to add entity names";
            return false;
        }

        json sceneJson;
        file >> sceneJson;
        file.close();


        mSerializer.saveEntityNames(mEntityNames, sceneJson);


        std::ofstream outFile(filepath);
        if (!outFile.is_open()) {
            qCritical() << "Failed to write entity names";
            return false;
        }
        outFile << sceneJson.dump(4);
        outFile.close();

        mCurrentScene.filepath = filepath;
        mCurrentScene.isLoaded = true;
        mHasUnsavedChanges = false;
        updateSceneInfo();

        qInfo() << "Scene saved to:" << QString::fromStdString(filepath);
        return true;
    } else {
        qCritical() << "Failed to save scene:" << QString::fromStdString(mSerializer.getLastError());
        return false;
    }
}

bool SceneManager::saveCurrentScene()
{
    if(mCurrentScene.filepath.empty()){
        qWarning() <<"couldnt save: no file path for current scene";
        qWarning() <<"Use saveCurrentFilePath instead baby";
        return false;
    }

    return saveCurrentScene(mCurrentScene.filepath);
}

bool SceneManager::loadScene(const std::string& filepath, bool clearCurrent)
{
    if (!mEntityManager){
        qCritical()<<"Cannot load scene: EntityManager = NULL";
        return false;
    }

    QFileInfo fileInfo(QString::fromStdString(filepath));
    if (!fileInfo.exists()) {
        qCritical() << "Scene file does not exist:" << QString::fromStdString(filepath);
        return false;
    }

    if (clearCurrent){
        if(mOnSceneUnload){
            mOnSceneUnload();
        }
    }

    bool success = mSerializer.loadScene(mEntityManager, mGPUResources, filepath, &mEntityNames);

    if (success) {
        mCurrentScene.filepath = filepath;
        mCurrentScene.name = fileInfo.baseName().toStdString();
        mCurrentScene.isLoaded = true;
        mHasUnsavedChanges = false;
        updateSceneInfo();

        qInfo() << "Scene loaded from:" << QString::fromStdString(filepath);
        qInfo() << "Scene name:" << QString::fromStdString(mCurrentScene.name);
        qInfo() << "Entities loaded:" << mCurrentScene.entityCount;

        if (mOnSceneLoad) {
            mOnSceneLoad();
        }

        return true;
    } else {
        qCritical() << "Failed to load scene:" << QString::fromStdString(mSerializer.getLastError());
        return false;
    }
}

void SceneManager::unloadCurrentScene()
{
    if(!mEntityManager){
        return;
    }

    if (mOnSceneLoad){
        mOnSceneLoad();
    }

    mEntityManager->clear();
    mEntityNames.clear();

    mCurrentScene.name = "Untitled Scene";
    mCurrentScene.filepath ="";
    mCurrentScene.isLoaded = false;
    mCurrentScene.entityCount = 0;
    mHasUnsavedChanges = false;

    qInfo() <<"Scene unloaded";
}

void SceneManager::setEntityName(EntityID entityID, const std::string& name)
{
    if (!mEntityManager || !mEntityManager->isValidEntity(entityID)) {
        qWarning() << "Cannot set name for invalid entity:" << entityID;
        return;
    }

    mEntityNames[entityID] = name;
    markSceneDirty();
}


std::string SceneManager::getEntityName(EntityID entityID) const
{
    auto it = mEntityNames.find(entityID);
    if(it != mEntityNames.end()){
        return it->second;
    }
    return "Entity_" + std::to_string(entityID);
}

std::string SceneManager::getLastError() const
{
    return mSerializer.getLastError();
}

bool SceneManager::validateSceneFile(const std::string& filepath)
{
    return mSerializer.validateSceneFile(filepath);
}


void SceneManager::updateSceneInfo()
{
    if (mEntityManager) {
        mCurrentScene.entityCount = static_cast<int>(mEntityManager->getEntityCount());
    }
}

}
