#include "Physics.h"

#include <qdebug.h>

using namespace bbl;

PhysicsSystem::PhysicsSystem(EntityManager* entityManager)
    : m_entityManager(entityManager)
{

    frictionCoefficient = 0.2f; // juster basert på hvor mye friksjon vi vil ha
                                // 0.1f for veldig glatt, 0.3f for vanlig, 0.7f for mye friksjon f. eks
    zone_frictionCoefficient = 1.f;

}

void PhysicsSystem::update(float dt)
{
    if (!m_entityManager) {
        return;
    }

    std::vector<bbl::EntityID> physicsEntities = m_entityManager->getEntitiesWith<bbl::Physics, bbl::Transform>();

    for (EntityID entity : physicsEntities)
    {
        bbl::Physics* physics = m_entityManager->getComponent<bbl::Physics>(entity);
        bbl::Transform* transform = m_entityManager->getComponent<bbl::Transform>(entity);

        if (!physics || !transform)
        {
            continue;
        }

        bbl::Collision* collision = m_entityManager->getComponent<bbl::Collision>(entity);

        // Sjekker først om vår entity kan bruke rulle fysikk
        bool useRollingPhysics = m_rollingPhysicsEnabled && collision && collision->isGrounded && m_terrain;

        if (useRollingPhysics)
        {
            // Bruk rulling av ball fysikk fra Algoritme 9.6
            updateRollingPhysics(entity, dt);
        }
        else
        {
            // Bruk gravitasjon hvis det er påskrudd og entitien ikke er isGrounded
            if (physics->useGravity && collision && !collision->isGrounded)
            {
                physics->acceleration += m_gravity;
            }

            else if (collision && collision->isGrounded)
            {
                // Tilbakestiller vertical velocity når grounded
                physics->acceleration.y = 0.0f;
                physics->velocity.y = 0.0f;
            }

            // Oppdaterer hastighet: v = v + a * dt
            physics->velocity += physics->acceleration * dt;

            // Oppdaterer position: p = p + v * dt
            transform->position += physics->velocity * dt;

            // Tilbakestiller akselerasjon for neste frame
            physics->acceleration = glm::vec3(0.0f);
        }
    }
}

void PhysicsSystem::updateRollingPhysics(EntityID entity, float dt)
{
    bbl::Physics* physics = m_entityManager->getComponent<bbl::Physics>(entity);
    bbl::Transform* transform = m_entityManager->getComponent<bbl::Transform>(entity);

    if (!physics || !transform || !m_terrain)
    {
        return;
    }

    // Steg 1: Finn hvilken trekant ballen er på (Algoritme 9.6, steg 1)
    int triangleIndex = findCurrentTriangle(transform->position);

    if (triangleIndex == -1)
    {
        // Ingen trekant funnet, går tilbake til standard fysikk
        if (physics->useGravity)
        {
            physics->acceleration += m_gravity;
        }

        physics->velocity += physics->acceleration * dt;
        transform->position += physics->velocity * dt;
        physics->acceleration = glm::vec3(0.0f);

        return;
    }

    // Steg 2: Beregn normalvektoren i kontaktpunktet med underlaget (Algoritme 9.6, steg 2)
    glm::vec3 surfaceNormal = calculateSurfaceNormal(triangleIndex);

    // Steg 3: Beregn akselerasjonvektoren til ballen etter ligning 9.14
    glm::vec3 surfaceAcceleration = calculateSurfaceAcceleration(surfaceNormal);

    // Steg 4: Oppdater ballens hastighet (Algoritme 9.6, steg 4 - ligning 9.16)
    // Legger også til friksjon, samt sjekker posisjonen til ballen for å oppdatere friksjon
    glm::vec3 frictionForce = calculateFrictionForce(physics->velocity, surfaceNormal, transform->position);

    // Legger de to sammen
    glm::vec3 totalAcceleration = surfaceAcceleration + frictionForce;

    physics->velocity += totalAcceleration * dt;


    // Steg 5: Oppdater ballens posisjon (Algoritme 9.6, steg 5 - ligning 9.17)
    transform->position += physics->velocity * dt;

    // Har ikke implementert rotering av ballen

    // Tilbakestill akselerasjon for neste frame
    physics->acceleration = glm::vec3(0.0f);
}

glm::vec3 PhysicsSystem::calculateFrictionForce(const glm::vec3& velocity, const glm::vec3& surfaceNormal, const glm::vec3& position)
{
    // Beregn hastighet parallell med overflaten (fjern normal komponenten)
    glm::vec3 velocityParallel = velocity - glm::dot(velocity, surfaceNormal) * surfaceNormal;

    // Hvis objektet ikke beveger seg, ingen friksjon
    float velocityMagnitude = glm::length(velocityParallel);
    if (velocityMagnitude < 0.001f)
    {
        return glm::vec3(0.0f);
    }

    float currentFriction = getFrictionAtPosition(position);

    // Friksjonskraften virker i motsatt retning av bevegelsen
    glm::vec3 frictionDirection = -glm::normalize(velocityParallel);

    // Beregn normal kraften (komponent av gravitasjon vinkelrett på overflaten)
    float normalForce = abs(glm::dot(-m_gravity, surfaceNormal));

    // Friksjonskraft = μ * N
    float frictionMagnitude = currentFriction * normalForce;

    // Returner friksjonskraften som akselerasjon (F = ma, så a = F/m, antatt m=1)
    return frictionDirection * frictionMagnitude;
}

int PhysicsSystem::findCurrentTriangle(const glm::vec3& position)
{
    if (!m_terrain)
    {
        return -1;
    }

    // Henter triangulert punktsky mesh data fra Terrain klassen
    const auto& vertices = m_terrain->getVertices();
    const auto& indices = m_terrain->getIndices();

    if (vertices.empty() || indices.empty()) {
        return -1;
    }

    // Sjekker hver trekant for å se om ballen er på den
    for (size_t i = 0; i < indices.size(); i += 3)
    {
        glm::vec3 v0 = vertices[indices[i]].pos;  // Access .pos from Vertex struct
        glm::vec3 v1 = vertices[indices[i + 1]].pos;
        glm::vec3 v2 = vertices[indices[i + 2]].pos;

        if (isPointInTriangle(position, v0, v1, v2))
        {
            return static_cast<int>(i / 3); // Returner trekant index
        }
    }

    return -1; // Ingen trekant funnet
}

bool PhysicsSystem::isPointInTriangle(const glm::vec3& point, const glm::vec3& v0,
                                      const glm::vec3& v1, const glm::vec3& v2)
{
    // Kalkulerer barysentriske koordinater (Algorithm 9.6 uses barycentric coordinates)
    glm::vec3 v0v1 = v1 - v0;
    glm::vec3 v0v2 = v2 - v0;
    glm::vec3 normal = glm::normalize(glm::cross(v0v1, v0v2));

    // Projiserer til 2D ved å fjerne komponenten med den største normalen
    int maxComponent = 0;
    if (abs(normal.y) > abs(normal.x)) maxComponent = 1;
    if (abs(normal.z) > abs(normal[maxComponent])) maxComponent = 2;

    glm::vec2 p, a, b, c;
    if (maxComponent == 0)
    {
        p = glm::vec2(point.y, point.z);
        a = glm::vec2(v0.y, v0.z);
        b = glm::vec2(v1.y, v1.z);
        c = glm::vec2(v2.y, v2.z);
    }

    else if (maxComponent == 1)
    {
        p = glm::vec2(point.x, point.z);
        a = glm::vec2(v0.x, v0.z);
        b = glm::vec2(v1.x, v1.z);
        c = glm::vec2(v2.x, v2.z);
    }

    else
    {
        p = glm::vec2(point.x, point.y);
        a = glm::vec2(v0.x, v0.y);
        b = glm::vec2(v1.x, v1.y);
        c = glm::vec2(v2.x, v2.y);
    }

    // Barysentrisk koordinat test
    glm::vec2 v0_2d = c - a;
    glm::vec2 v1_2d = b - a;
    glm::vec2 v2_2d = p - a;

    float dot00 = glm::dot(v0_2d, v0_2d);
    float dot01 = glm::dot(v0_2d, v1_2d);
    float dot02 = glm::dot(v0_2d, v2_2d);
    float dot11 = glm::dot(v1_2d, v1_2d);
    float dot12 = glm::dot(v1_2d, v2_2d);

    float invDenom = 1.0f / (dot00 * dot11 - dot01 * dot01);
    float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
    float v = (dot00 * dot12 - dot01 * dot02) * invDenom;

    return (u >= 0) && (v >= 0) && (u + v <= 1);
}

glm::vec3 PhysicsSystem::calculateSurfaceNormal(int triangleIndex)
{
    if (!m_terrain)
    {
        return glm::vec3(0.0f, 1.0f, 0.0f); // Vektor oppover
    }

    // Henter triangulert punktsky mesh data fra Terrain klassen
    const auto& vertices = m_terrain->getVertices();
    const auto& indices = m_terrain->getIndices();

    if (triangleIndex < 0 || static_cast<size_t>(triangleIndex * 3 + 2) >= indices.size())
    {
        return glm::vec3(0.0f, 1.0f, 0.0f);
    }

    // Henter trekant vertices
    int baseIdx = triangleIndex * 3;
    glm::vec3 v0 = vertices[indices[baseIdx]].pos;      // Tilgang til .pos fra Vertex struct
    glm::vec3 v1 = vertices[indices[baseIdx + 1]].pos;
    glm::vec3 v2 = vertices[indices[baseIdx + 2]].pos;

    // Beregner normal ved å bruke kryss produkt (Algoritme 9.6, steg 2)
    glm::vec3 edge1 = v1 - v0;
    glm::vec3 edge2 = v2 - v0;
    glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));

    return normal;
}

glm::vec3 PhysicsSystem::calculateSurfaceAcceleration(const glm::vec3& normal)
{
    // Beregn akselerasjon i henhold til ligning 9.14
    // Prosjiserer gravitasjon på planet

    // Gravitasjons komponenten parallel med planet
    glm::vec3 gravityParallel = m_gravity - glm::dot(m_gravity, normal) * normal;

    return gravityParallel;
}


void PhysicsSystem::setGravity(const glm::vec3& gravity)
{
    m_gravity = gravity;
}

const glm::vec3& PhysicsSystem::getGravity() const
{
    return m_gravity;
}

void PhysicsSystem::setFrictionCoefficient(float coefficient)
{
    frictionCoefficient = coefficient;
}

float PhysicsSystem::getFrictionCoefficient() const
{
    return frictionCoefficient;
}

float PhysicsSystem::getFrictionAtPosition(const glm::vec3& position)
{
    // Ballens posisjon vil påvirke hvor mye friksjon som virker på den
    float distanceFromCenter = glm::length(position - zone_frictionCenter);

    if (distanceFromCenter <= zone_frictionRadius)
    {
        return zone_frictionCoefficient;
    }

    return frictionCoefficient;
}
