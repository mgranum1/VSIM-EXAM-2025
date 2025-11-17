#include "GameWorld.h"
#include "../Editor/MainWindow.h"

bbl::GameWorld::GameWorld()
{
    m_terrain = std::make_unique<Terrain>();
}


void bbl::GameWorld::Setup()
{
    if (m_terrain->loadFromHeightmap("../../Assets/Textures/heightmap.jpg", 0.15f, 1.f, 0.0f))
    {
        m_terrainLoaded = true;
        qDebug() << "Terrain loaded successfully!";
    }
    else
    {
        qWarning() << "Failed to load terrain!";
    }

}

void bbl::GameWorld::initializeSystems(EntityManager* entityManager)
{
    if (!entityManager) {
        qWarning() << "Cannot initialize systems: EntityManager is null!";
        return;
    }

    // Physics System
    m_physicsSystem = std::make_unique<PhysicsSystem>(entityManager);
    m_physicsSystem->setGravity(glm::vec3(0.0f, -9.81f, 0.0f));

    // Collision System
    m_collisionSystem = std::make_unique<CollisionSystem>(entityManager, m_terrain.get());
    m_collisionSystem->setTerrainCollisionEnabled(true);
    m_collisionSystem->setEntityCollisionEnabled(true);
}

void bbl::GameWorld::update(float dt)
{
    if (m_collisionSystem) {
        m_collisionSystem->update(dt);
    }
    if (m_physicsSystem) {
        m_physicsSystem->update(dt);
    }

}
