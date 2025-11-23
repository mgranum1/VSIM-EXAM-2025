#ifndef GAMEWORLD_H
#define GAMEWORLD_H

#include "../Game/Terrain.h"
#include "../ECS/Components/Physics.h"
#include "../ECS/Components/CollisionSystem.h"
#include "../ECS/Entity/EntityManager.h"
#include "../ECS/Components/trackingsystemclass.h"
#include <memory>

class Renderer;

namespace bbl
{

class GameWorld
{
public:
    GameWorld();

    void Setup();
    void update(float dt);

    void setPaused(bool paused) { mPaused = paused; }
    bool isPaused() const { return mPaused; }

    Terrain* getTerrain() const { return m_terrain.get(); }
    bool isTerrainLoaded() const { return m_terrainLoaded; }
    void setTerrainEntity(EntityID terrainID)
    {
        if (m_collisionSystem) {
            m_collisionSystem->setTerrainEntity(terrainID);
        }
    }


    void initializeSystems(EntityManager* entityManager, Renderer* renderer);

    void setupFrictionZone();


    TrackingSystemClass* getTrackingSystem() const { return m_trackingsystem.get(); }


private:
    std::unique_ptr<Terrain> m_terrain;
    std::unique_ptr<PhysicsSystem> m_physicsSystem;
    std::unique_ptr<CollisionSystem> m_collisionSystem;
    std::unique_ptr<TrackingSystemClass> m_trackingsystem;
    Renderer* m_renderer = nullptr;

    bool m_terrainLoaded{false};
    bool mPaused{true};
};

} // namespace bbl

#endif // GAMEWORLD_H
