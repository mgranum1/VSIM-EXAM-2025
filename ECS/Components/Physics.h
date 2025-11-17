#ifndef PHYSICSSYSTEM_H
#define PHYSICSSYSTEM_H

#include "../../ECS/Entity/EntityManager.h"
#include <glm/glm.hpp>

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

private:
    EntityManager* m_entityManager;
    glm::vec3 m_gravity{0.0f, -9.81f, 0.0f};
};
}

#endif // PHYSICSSYSTEM_H
