#include "SceneSerializer.h"
#include <QDebug>
#include <fstream>
#include "../Core/Utility/modelloader.h"
#include "../Game/Terrain.h"

namespace bbl
{

bool SceneSerializer::saveScene(const EntityManager* entityManager,
                                const std::string& filepath,
                                const std::string& sceneName)
{
    clearError();

    if (!entityManager) {
        setError("EntityManager is null");
        return false;
    }

    try {
        json sceneJson;

        sceneJson["scene_metadata"] = {
            {"name", sceneName},
            {"version", "1.0"},
            {"engine", "BBL Engine"},
            {"entity_count", entityManager->getEntityCount()}
        };

        json entitiesArray = json::array();

        auto allEntities = entityManager->getAllEntities();
        qDebug() << "DEBUG: Entities to save:" << allEntities.size();
        for (EntityID entityID : allEntities) {
            qDebug() << "DEBUG: Serializing entity" << entityID;
            json entityJson = serializeEntity(entityManager, entityID);
            entitiesArray.push_back(entityJson);
        }

        sceneJson["entities"] = entitiesArray;

        std::ofstream file(filepath);
        if (!file.is_open()) {
            setError("Failed to open file for writing: " + filepath);
            return false;
        }

        file << sceneJson.dump(4);
        file.close();

        qInfo() << "Scene saved successfully:" << QString::fromStdString(filepath);
        qInfo() << "Entities saved:" << entityManager->getEntityCount();

        return true;

    } catch (const std::exception& e) {
        setError(std::string("Exception during scene save: ") + e.what());
        qCritical() << QString::fromStdString(mLastError);
        return false;
    }
}

void SceneSerializer::saveEntityNames(const std::unordered_map<EntityID, std::string>& entityNames,
                                      json& sceneJson)
{
    json namesJson = json::object();
    for (const auto& [entityID, name] : entityNames) {
        namesJson[std::to_string(entityID)] = name;
    }
    sceneJson["entity_names"] = namesJson;
}

bool SceneSerializer::loadScene(EntityManager* entityManager,
                                GPUResourceManager* gpuResources,
                                const std::string& filepath,
                                std::unordered_map<EntityID, std::string>* outEntityNames)
{
    clearError();

    if (!entityManager) {
        setError("EntityManager is null");
        return false;
    }

    if (!gpuResources) {
        qWarning() << "GPUResourceManager is null - textures won't be reloaded";
    }

    try {
        // Read File
        std::ifstream file(filepath);
        if (!file.is_open()) {
            setError("Failed to open file for reading: " + filepath);
            return false;
        }

        json sceneJson;
        file >> sceneJson;
        file.close();

        if (!sceneJson.contains("scene_metadata") || !sceneJson.contains("entities")) {
            setError("Invalid scene file format");
            return false;
        }

        if (outEntityNames) {
            outEntityNames->clear();
        }

        auto metadata = sceneJson["scene_metadata"];
        std::string sceneName = metadata.value("name", "Untitled");
        int entityCount = metadata.value("entity_count", 0);

        qInfo() << "Loading scene:" << QString::fromStdString(sceneName);
        qInfo() << "Expected entities:" << entityCount;

        if (outEntityNames && sceneJson.contains("entity_names")) {
            json namesJson = sceneJson["entity_names"];
            for (auto& [key, value] : namesJson.items()) {
                EntityID entityID = std::stoull(key);
                std::string name = value.get<std::string>();
                (*outEntityNames)[entityID] = name;
            }
        }

        json entitiesArray = sceneJson["entities"];
        int loadedCount = 0;

        for (const auto& entityJson : entitiesArray) {
            deserializeEntity(entityManager, entityJson);

            EntityID entityID = entityJson["entity_id"].get<EntityID>();

            if (gpuResources && entityJson.contains("Mesh")) {
                std::string meshPath = entityJson["Mesh"].value("modelPath", "");
                size_t meshIndex = entityJson["Mesh"].value("meshIndex", 0);

                if (!meshPath.empty()) {
                    qDebug() << "Reloading mesh from:" << QString::fromStdString(meshPath);

                    if (meshPath.find("heightmap") != std::string::npos) {

                        qDebug() << "Detected terrain heightmap - regenerating terrain mesh";

                        // Regenerate terrain from heightmap so that we can load in terrain with the right heightmap
                        Terrain tempTerrain;
                        if (tempTerrain.loadFromHeightmap(meshPath, 0.15f, 1.f, 0.0f)) {

                            bbl::MeshData terrainMeshData;
                            terrainMeshData.vertices = tempTerrain.getVertices();
                            terrainMeshData.indices = tempTerrain.getIndices();
                            terrainMeshData.materialIndex = -1;

                            size_t newMeshID = gpuResources->uploadMesh(terrainMeshData);

                            // Update mesh component
                            if (auto* mesh = entityManager->getComponent<Mesh>(entityID)) {
                                mesh->meshResourceID = newMeshID;
                            }
                            // Update render component
                            if (auto* render = entityManager->getComponent<Render>(entityID)) {
                                render->meshResourceID = newMeshID;
                            }

                            qDebug() << "Terrain mesh regenerated successfully!";
                        } else {
                            qWarning() << "Failed to regenerate terrain from heightmap";
                        }

                    } else {

                        bbl::ModelLoader loader;
                        auto model = loader.loadModel(meshPath, "");

                        if (model && meshIndex < model->meshes.size()) {

                            const auto& meshData = model->meshes[meshIndex];
                            size_t newMeshID = gpuResources->uploadMesh(meshData);

                            // Update mesh component
                            if (auto* mesh = entityManager->getComponent<Mesh>(entityID)) {
                                mesh->meshResourceID = newMeshID;
                            }
                            // Update render component
                            if (auto* render = entityManager->getComponent<Render>(entityID)) {
                                render->meshResourceID = newMeshID;
                            }
                        } else {
                            qWarning() << "Failed to reload mesh from" << QString::fromStdString(meshPath);
                        }
                    }
                }
            }
            if (gpuResources && entityJson.contains("Texture")) {
                std::string texPath = entityJson["Texture"].value("texturePath", "");
                if (!texPath.empty()) {
                    qDebug() << "Reloading texture:" << QString::fromStdString(texPath);
                    size_t newTexID = gpuResources->uploadTexture(texPath);

                    // Update texture component
                    if (auto* tex = entityManager->getComponent<Texture>(entityID)) {
                        tex->textureResourceID = newTexID;
                    }
                    // Update render component
                    if (auto* render = entityManager->getComponent<Render>(entityID)) {
                        render->textureResourceID = newTexID;
                    }
                }
            }
            if (entityJson.contains("Audio")) {
                if (auto* audio = entityManager->getComponent<Audio>(entityID)) {

                    if (!audio->attackSound.empty() && audio->attackSound != "../../Assets/Sounds/") {
                        qDebug() << "Audio component has attack sound:" << QString::fromStdString(audio->attackSound);
                    }
                    if (!audio->deathSound.empty() && audio->deathSound != "../../Assets/Sounds/") {
                        qDebug() << "Audio component has death sound:" << QString::fromStdString(audio->deathSound);
                    }
                }
            }

            loadedCount++;
        }

        qInfo() << "Scene loaded successfully!";
        qInfo() << "Entities loaded:" << loadedCount;

        return true;

    } catch (const std::exception& e) {
        setError(std::string("Exception during scene load: ") + e.what());
        qCritical() << QString::fromStdString(mLastError);
        return false;
    }
}

bool SceneSerializer::validateSceneFile(const std::string& filepath)
{
    try {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            return false;
        }

        json sceneJson;
        file >> sceneJson;
        file.close();

        return sceneJson.contains("scene_metadata") &&sceneJson.contains("entities");

    } catch (...) {
        return false;
    }
}

json SceneSerializer::serializeEntity(const EntityManager* entityManager, EntityID entityID)
{
    json entityJson;
    entityJson["entity_id"] = entityID;

    if (entityManager->hasComponent<Transform>(entityID)) {
        const auto* comp = entityManager->getComponent<Transform>(entityID);
        entityJson["Transform"] = serializeTransform(*comp);
    }

    if (entityManager->hasComponent<Mesh>(entityID)) {
        const auto* comp = entityManager->getComponent<Mesh>(entityID);
        entityJson["Mesh"] = serializeMesh(*comp);
    }

    if (entityManager->hasComponent<Texture>(entityID)) {
        const auto* comp = entityManager->getComponent<Texture>(entityID);
        entityJson["Texture"] = serializeTexture(*comp);
    }

    if (entityManager->hasComponent<Render>(entityID)) {
        const auto* comp = entityManager->getComponent<Render>(entityID);
        entityJson["Render"] = serializeRender(*comp);
    }

    if (entityManager->hasComponent<Audio>(entityID)) {
        const auto* comp = entityManager->getComponent<Audio>(entityID);
        entityJson["Audio"] = serializeAudio(*comp);
    }

    if (entityManager->hasComponent<Physics>(entityID)) {
        const auto* comp = entityManager->getComponent<Physics>(entityID);
        entityJson["Physics"] = serializePhysics(*comp);
    }

    if (entityManager->hasComponent<Collision>(entityID)) {
        const auto* comp = entityManager->getComponent<Collision>(entityID);
        entityJson["Collision"] = serializeCollision(*comp);
    }

    return entityJson;
}

void SceneSerializer::deserializeEntity(EntityManager* entityManager, const json& entityJson)
{

    EntityID savedID = entityJson["entity_id"].get<EntityID>();


    EntityID newEntity = entityManager->createEntityWithID(savedID);

    if (entityJson.contains("Transform")) {
        Transform transform = deserializeTransform(entityJson["Transform"]);
        entityManager->addComponent(newEntity, transform);
    }

    if (entityJson.contains("Mesh")) {
        Mesh mesh = deserializeMesh(entityJson["Mesh"]);
        entityManager->addComponent(newEntity, mesh);
    }

    if (entityJson.contains("Texture")) {
        Texture texture = deserializeTexture(entityJson["Texture"]);
        entityManager->addComponent(newEntity, texture);
    }

    if (entityJson.contains("Render")) {
        Render render = deserializeRender(entityJson["Render"]);
        entityManager->addComponent(newEntity, render);
    }

    if (entityJson.contains("Audio")) {
        Audio audio = deserializeAudio(entityJson["Audio"]);
        entityManager->addComponent(newEntity, audio);
    }

    if (entityJson.contains("Physics")) {
        Physics physics = deserializePhysics(entityJson["Physics"]);
        entityManager->addComponent(newEntity, physics);
    }

    if (entityJson.contains("Collision")) {
        Collision collision = deserializeCollision(entityJson["Collision"]);
        entityManager->addComponent(newEntity, collision);
    }
}

json SceneSerializer::serializeTransform(const Transform& transform)
{
    return {
        {"position", {transform.position.x, transform.position.y, transform.position.z}},
        {"rotation", {transform.rotation.x, transform.rotation.y, transform.rotation.z}},
        {"scale", {transform.scale.x, transform.scale.y, transform.scale.z}}
    };
}

json SceneSerializer::serializeMesh(const Mesh& mesh)
{
    return {
        {"meshResourceID", mesh.meshResourceID},
        {"modelPath", mesh.modelPath},
        {"meshIndex", mesh.meshIndex}
    };
}

json SceneSerializer::serializeTexture(const Texture& texture)
{
    return {
        {"textureResourceID", texture.textureResourceID},
        {"texturePath", texture.texturePath}
    };
}

json SceneSerializer::serializeRender(const Render& render)
{
    return {
        {"meshResourceID", render.meshResourceID},
        {"textureResourceID", render.textureResourceID},
        {"visible", render.visible},
        {"usePhong", render.usePhong},
        {"opacity", render.opacity}
    };
}

json SceneSerializer::serializeAudio(const Audio& audio)
{
    return {
        {"volume", audio.volume},
        {"muted", audio.muted},
        {"looping", audio.looping},
        {"attackSound", audio.attackSound},
        {"deathSound", audio.deathSound}
    };
}

json SceneSerializer::serializePhysics(const Physics& physics)
{
    return {
        {"velocity", {physics.velocity.x, physics.velocity.y, physics.velocity.z}},
        {"acceleration", {physics.acceleration.x, physics.acceleration.y, physics.acceleration.z}},
        {"mass", physics.mass},
        {"useGravity", physics.useGravity}
    };
}

json SceneSerializer::serializeCollision(const Collision& collision)
{
    return {
        {"colliderSize", {collision.colliderSize.x, collision.colliderSize.y, collision.colliderSize.z}},
        {"isGrounded", collision.isGrounded},
        {"isColliding", collision.isColliding},
        {"isTrigger", collision.isTrigger},
        {"isStatic", collision.isStatic}
    };
}

Transform SceneSerializer::deserializeTransform(const json& j)
{
    Transform transform;
    auto pos = j["position"];
    auto rot = j["rotation"];
    auto scl = j["scale"];

    transform.position = glm::vec3(pos[0], pos[1], pos[2]);
    transform.rotation = glm::vec3(rot[0], rot[1], rot[2]);
    transform.scale = glm::vec3(scl[0], scl[1], scl[2]);

    return transform;
}

Mesh SceneSerializer::deserializeMesh(const json& j)
{
    Mesh mesh;
    mesh.meshResourceID = j["meshResourceID"].get<size_t>();
    mesh.modelPath = j.value("modelPath", "");
    mesh.meshIndex = j.value("meshIndex", 0);
    return mesh;
}
Texture SceneSerializer::deserializeTexture(const json& j)
{
    Texture texture;
    texture.textureResourceID = j["textureResourceID"].get<size_t>();
    texture.texturePath = j.value("texturePath", "");
    return texture;
}

Render SceneSerializer::deserializeRender(const json& j)
{
    Render render;
    render.meshResourceID = j["meshResourceID"].get<size_t>();
    render.textureResourceID = j["textureResourceID"].get<size_t>();
    render.visible = j["visible"].get<bool>();
    render.usePhong = j["usePhong"].get<bool>();
    render.opacity = j["opacity"].get<float>();
    return render;
}

Audio SceneSerializer::deserializeAudio(const json& j)
{
    Audio audio;
    audio.volume = j["volume"].get<float>();
    audio.muted = j["muted"].get<bool>();
    audio.looping = j["looping"].get<bool>();
    audio.attackSound = j.value("attackSound", "../../Assets/Sounds/");
    audio.deathSound = j.value("deathSound", "../../Assets/Sounds/");
    return audio;
}

Physics SceneSerializer::deserializePhysics(const json& j)
{
    Physics physics;
    auto vel = j["velocity"];
    auto acc = j["acceleration"];

    physics.velocity = glm::vec3(vel[0], vel[1], vel[2]);
    physics.acceleration = glm::vec3(acc[0], acc[1], acc[2]);
    physics.mass = j["mass"].get<float>();
    physics.useGravity = j["useGravity"].get<bool>();

    return physics;
}

Collision SceneSerializer::deserializeCollision(const json& j)
{
    Collision collision;
    auto size = j["colliderSize"];

    collision.colliderSize = glm::vec3(size[0], size[1], size[2]);
    collision.isGrounded = j["isGrounded"].get<bool>();
    collision.isColliding = j["isColliding"].get<bool>();
    collision.isTrigger = j["isTrigger"].get<bool>();
    collision.isStatic = j["isStatic"].get<bool>();

    return collision;
}
}
