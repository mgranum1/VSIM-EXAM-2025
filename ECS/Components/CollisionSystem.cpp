#include "CollisionSystem.h"
#include <algorithm>
#include <qdebug.h>

using namespace bbl;

CollisionSystem::CollisionSystem(EntityManager* entityManager, Terrain* terrain)
    : m_entityManager(entityManager)
    , m_terrain(terrain)
{
}

void CollisionSystem::update(float dt)
{
    if (!m_entityManager) {
        return;
    }

    // Get all entities with collision components
    std::vector<EntityID> collisionEntities = m_entityManager->getEntitiesWith<Collision, Transform>();

    // Reset collision (blud)
    for (EntityID entity : collisionEntities) {
        Collision* collision = m_entityManager->getComponent<Collision>(entity);
        if (collision) {
            collision->isGrounded = false;
            collision->isColliding = false;
        }
    }

    // Check terrain collisions first(Before rest)
    if (m_terrainCollisionEnabled && m_terrain) {
        for (EntityID entity : collisionEntities) {
            Transform* transform = m_entityManager->getComponent<Transform>(entity);
            Collision* collision = m_entityManager->getComponent<Collision>(entity);

            if (transform && collision) {
                checkTerrainCollision(entity, transform, collision);
            }
        }
    }

    if (m_entityCollisionEnabled) {
        checkEntityCollisions();
    }
}

AABB CollisionSystem::calculateAABB(const Transform& transform, const Collision& collision) const
{
    glm::vec3 halfSize = collision.colliderSize * 0.5f * transform.scale;

    AABB aabb;
    aabb.min = transform.position - halfSize;
    aabb.max = transform.position + halfSize;

    return aabb;
}

void CollisionSystem::checkTerrainCollision(EntityID entity, Transform* transform, Collision* collision)
{
    if (!m_terrain || !transform || !collision) {
        return;
    }
    glm::vec3 terrainPosition(0.0f);
    if (m_terrainEntityID != INVALID_ENTITY) {
        if (auto* terrainTransform = m_entityManager->getComponent<Transform>(m_terrainEntityID)) {
            terrainPosition = terrainTransform->position;
        }
    }

    float terrainHeight = m_terrain->getHeightAt(transform->position.x,transform->position.z,  terrainPosition);

    float colliderHalfHeight = (collision->colliderSize.y * transform->scale.y) * 0.5f;
    float entityBottom = transform->position.y - colliderHalfHeight;

    if (entityBottom <= terrainHeight) {
        collision->isGrounded = true;
        collision->isColliding = true;

        // Snap entity to terrain surface
        transform->position.y = terrainHeight + colliderHalfHeight;

        Physics* physics = m_entityManager->getComponent<Physics>(entity);
        if (physics && physics->velocity.y < 0.0f) {
            physics->velocity.y = 0.0f;
        }
    }
    // Check if close to ground
    else if (entityBottom <= terrainHeight + m_groundCheckDistance) {
        collision->isGrounded = true;
    }
}

void CollisionSystem::checkEntityCollisions()
{
    if (!m_entityManager) {
        return;
    }

    std::vector<EntityID> collisionEntities = m_entityManager->getEntitiesWith<Collision, Transform>();

    // Check all pairs of entities
    for (size_t i = 0; i < collisionEntities.size(); ++i) {
        EntityID entityA = collisionEntities[i];
        Transform* transformA = m_entityManager->getComponent<Transform>(entityA);
        Collision* collisionA = m_entityManager->getComponent<Collision>(entityA);

        if (!transformA || !collisionA) {
            continue;
        }

        AABB aabbA = calculateAABB(*transformA, *collisionA);

        for (size_t j = i + 1; j < collisionEntities.size(); ++j) {
            EntityID entityB = collisionEntities[j];
            Transform* transformB = m_entityManager->getComponent<Transform>(entityB);
            Collision* collisionB = m_entityManager->getComponent<Collision>(entityB);

            if (!transformB || !collisionB) {
                continue;
            }

            AABB aabbB = calculateAABB(*transformB, *collisionB);

            // Check if AABBs overlap
            if (aabbA.intersects(aabbB)) {
                collisionA->isColliding = true;
                collisionB->isColliding = true;


                if (collisionA->isTrigger || collisionB->isTrigger) {
                    continue;
                }


                resolveCollision(entityA, entityB, transformA, transformB);
            }
        }
    }
}

void CollisionSystem::resolveCollision(EntityID entityA, EntityID entityB,
                                       Transform* transformA, Transform* transformB)
{
    // Calculate overlap
    glm::vec3 centerA = transformA->position;
    glm::vec3 centerB = transformB->position;
    glm::vec3 delta = centerB - centerA;

    Collision* collisionA = m_entityManager->getComponent<Collision>(entityA);
    Collision* collisionB = m_entityManager->getComponent<Collision>(entityB);

    glm::vec3 halfSizeA = collisionA->colliderSize * 0.5f * transformA->scale;
    glm::vec3 halfSizeB = collisionB->colliderSize * 0.5f * transformB->scale;
    glm::vec3 totalHalfSize = halfSizeA + halfSizeB;

    // Calculate overlap distances
    float overlapX = totalHalfSize.x - std::abs(delta.x);
    float overlapY = totalHalfSize.y - std::abs(delta.y);
    float overlapZ = totalHalfSize.z - std::abs(delta.z);


    glm::vec3 separation(0.0f);

    if (overlapX < overlapY && overlapX < overlapZ) {

        separation.x = (delta.x > 0) ? overlapX : -overlapX;
    } else if (overlapY < overlapZ) {

        separation.y = (delta.y > 0) ? overlapY : -overlapY;
    } else {
        // Separate on Z axis
        separation.z = (delta.z > 0) ? overlapZ : -overlapZ;
    }

    //push the entitys apart
    transformA->position -= separation * 0.5f;
    transformB->position += separation * 0.5f;

    // Stop their velocities in the collision direction
    Physics* physicsA = m_entityManager->getComponent<Physics>(entityA);
    Physics* physicsB = m_entityManager->getComponent<Physics>(entityB);

    if (physicsA && physicsB) {
        glm::vec3 normal = glm::normalize(separation);

        // Remove velocity component along collision normal
        float velA = glm::dot(physicsA->velocity, normal);
        float velB = glm::dot(physicsB->velocity, normal);

        if (velA < 0.0f) physicsA->velocity -= normal * velA;
        if (velB > 0.0f) physicsB->velocity -= normal * velB;
    }
}
