#include "GameWorld.h"
#include "../Editor/MainWindow.h"

bbl::GameWorld::GameWorld()
{
    m_terrain = std::make_unique<Terrain>();
}


void bbl::GameWorld::Setup()
{
    if (m_terrain->loadFromOBJ("../../Assets/Models/PointcloudTriangulated_Final.obj"))
    {
        m_terrainLoaded = true;
        qDebug() << "Terrain loaded successfully!";
    }
    else
    {
        qWarning() << "Failed to load terrain!";
    }

}

void bbl::GameWorld::setupFrictionZone()
{
    if (!m_terrain) {
        qWarning() << "Cannot setup friction zone: Terrain is null!";
        return;
    }

    // Beregner senteret av det triangulerte pointmeshet
    glm::vec3 minBounds, maxBounds;
    glm::vec3 terrainCenter = m_terrain->calculateBounds(minBounds, maxBounds);

    // Bruker det senteret
    m_physicsSystem->zone_frictionCenter = terrainCenter;

    // Basert på størrelsen av terrenget, lager vi en radius som da
    // blir en sone med høyere friksjon
    glm::vec3 terrainSize = maxBounds - minBounds;
    float terrainExtent = glm::length(terrainSize) * 0.5f;
    m_physicsSystem->zone_frictionRadius = terrainExtent * 0.2f;

    qDebug() << "Terrain bounds - Min:" << minBounds.x << minBounds.y << minBounds.z
             << "Max:" << maxBounds.x << maxBounds.y << maxBounds.z;
    qDebug() << "Terrain center:" << terrainCenter.x << terrainCenter.y << terrainCenter.z;
    qDebug() << "Friction zone radius:" << m_physicsSystem->zone_frictionRadius;

    glm::vec3 frictionZoneColor(0.9f, 0.3f, 0.1f);

    m_terrain->applyFrictionZoneColoring(m_physicsSystem->zone_frictionCenter, m_physicsSystem->zone_frictionRadius, frictionZoneColor);

    qDebug() << "3D friction zone applied successfully";
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
    m_physicsSystem->setTerrain(m_terrain.get());
    m_physicsSystem->enableRollingPhysics(true);
    setupFrictionZone();

    // Collision System
    m_collisionSystem = std::make_unique<CollisionSystem>(entityManager, m_terrain.get());
    m_collisionSystem->setTerrainCollisionEnabled(true);
    m_collisionSystem->setEntityCollisionEnabled(true);


}

void bbl::GameWorld::update(float dt)
{
    if (mPaused)
    {
        return;
    }

    if (m_collisionSystem) {
        m_collisionSystem->update(dt);
    }
    if (m_physicsSystem) {
        m_physicsSystem->update(dt);

    }

}
