#ifndef PHYSICSSYSTEM_H
#define PHYSICSSYSTEM_H
#include "../../ECS/Entity/EntityManager.h"
#include "../../Game/Terrain.h"
#include <glm/glm.hpp>
#include <vector>

namespace bbl
{
class PhysicsSystem
{
public:
    explicit PhysicsSystem(EntityManager* entityManager);
    void update(float deltaTime);
    void setGravity(const glm::vec3& gravity);
    const glm::vec3& getGravity() const;
    void setEntityManager(EntityManager* entityManager) {
        m_entityManager = entityManager;
    }

    // Rolling ball physics
    void setTerrain(Terrain* terrain) { m_terrain = terrain; }
    void enableRollingPhysics(bool enable) { m_rollingPhysicsEnabled = enable; }

    void setFrictionCoefficient(float coefficient);
    float getFrictionCoefficient() const;

private:
    EntityManager* m_entityManager;
    Terrain* m_terrain = nullptr;
    glm::vec3 m_gravity{0.0f, -9.81f, 0.0f};
    float frictionCoefficient;
    bool m_rollingPhysicsEnabled = false;
    glm::vec3 calculateFrictionForce(const glm::vec3& velocity, const glm::vec3& surfaceNormal);

    // Rolling physics methods
    void updateRollingPhysics(EntityID entity, float dt);
    int findCurrentTriangle(const glm::vec3& position);
    bool isPointInTriangle(const glm::vec3& point, const glm::vec3& v0,
                           const glm::vec3& v1, const glm::vec3& v2);
    glm::vec3 calculateSurfaceNormal(int triangleIndex);
    glm::vec3 calculateSurfaceAcceleration(const glm::vec3& normal);

};
}
#endif // PHYSICSSYSTEM_H
