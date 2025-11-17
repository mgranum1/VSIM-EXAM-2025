// #ifndef COLLISIONSYSTEM_H
// #define COLLISIONSYSTEM_H

#include "../Entity/EntityManager.h"
#include "../../Game/Terrain.h"
#include <glm/glm.hpp>
#include <vector>

namespace bbl
{

struct AABB
{
    glm::vec3 min;
    glm::vec3 max;

    bool intersects(const AABB& other) const
    {
        return (min.x <= other.max.x && max.x >= other.min.x) &&
               (min.y <= other.max.y && max.y >= other.min.y) &&
               (min.z <= other.max.z && max.z >= other.min.z);
    }
};

class CollisionSystem
{
public:
    CollisionSystem(EntityManager* entityManager, Terrain* terrain);

    void update(float dt);

    // Settings
    void setTerrainCollisionEnabled(bool enabled) { m_terrainCollisionEnabled = enabled; }
    void setEntityCollisionEnabled(bool enabled) { m_entityCollisionEnabled = enabled; }
    void setGroundCheckDistance(float distance) { m_groundCheckDistance = distance; }
    void setTerrainEntity(EntityID terrainID) { m_terrainEntityID = terrainID; }

private:
    EntityManager* m_entityManager;
    Terrain* m_terrain;
    EntityID m_terrainEntityID = INVALID_ENTITY;

    bool m_terrainCollisionEnabled{true};
    bool m_entityCollisionEnabled{true};
    float m_groundCheckDistance{0.1f};


    AABB calculateAABB(const Transform& transform, const Collision& collision) const;
    void checkTerrainCollision(EntityID entity, Transform* transform, Collision* collision);
    void checkEntityCollisions();
    void resolveCollision(EntityID entityA, EntityID entityB,Transform* transformA, Transform* transformB);
};

} // namespace bbl
// #endif // COLLISIONSYSTEM_H
