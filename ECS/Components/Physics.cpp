#include "Physics.h"

using namespace bbl;

PhysicsSystem::PhysicsSystem(EntityManager* entityManager)
    : m_entityManager(entityManager)
{
}

void PhysicsSystem::update(float dt)
{
    if (!m_entityManager) {
        return;
    }

    std::vector<bbl::EntityID> physicsEntities = m_entityManager->getEntitiesWith<bbl::Physics, bbl::Transform>();

    for (EntityID entity : physicsEntities) {

        bbl::Physics* physics = m_entityManager->getComponent<bbl::Physics>(entity);
        bbl::Transform* transform = m_entityManager->getComponent<bbl::Transform>(entity);

        if (!physics || !transform) {
            continue;
        }

        bbl::Collision* collision = m_entityManager->getComponent<bbl::Collision>(entity);

        // Apply gravity if enabled and not grounded
        if (physics->useGravity && collision && !collision->isGrounded)
        {
            physics->acceleration += m_gravity;
        }
        else if (collision && collision->isGrounded)
        {
            // Reset vertical velocity when grounded
            physics->acceleration.y = 0.0f;
            physics->velocity.y = 0.0f;
        }


        // Update velocity: v = v + a * dt
        physics->velocity += physics->acceleration * dt;

        // Update position: p = p + v * dt
        transform->position += physics->velocity * dt;

        // Reset acceleration for next frame
        physics->acceleration = glm::vec3(0.0f);
    }
}

void PhysicsSystem::setGravity(const glm::vec3& gravity) {
    m_gravity = gravity;
}

const glm::vec3& PhysicsSystem::getGravity() const {
    return m_gravity;
}
