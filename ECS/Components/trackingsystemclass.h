#ifndef TRACKINGSYSTEM_CLASS_H
#define TRACKINGSYSTEM_CLASS_H

#include "../ECS/Entity/EntityManager.h"
#include "../Core/Utility/gpuresourcemanager.h"
#include "../Core/Utility/Vertex.h"
#include "../ECS/Components/trackingsystem.h"
#include <memory>

namespace bbl
{

class TrackingSystemClass
{
public:
    explicit TrackingSystemClass(EntityManager* entityManager)
        : mEntityManager(entityManager)
    {
    }

    void update(float deltaTime)
    {
        if (!mEntityManager) return;

        // Get all entities with both Transform and Tracking components
        std::vector<EntityID> trackedEntities = mEntityManager->getEntitiesWith<Transform, Tracking>();

        for (EntityID entity : trackedEntities) {
            bbl::Transform* transform = mEntityManager->getComponent<Transform>(entity);
            bbl::Tracking* tracking = mEntityManager->getComponent<Tracking>(entity);

            if (transform && tracking && tracking->isTracking) {
                updateTracking(entity, *transform, *tracking);
            }
        }
    }

    // Legger til tracking for en Entity, hvis ikke den har component blir den lagt til
    void enableTracking(EntityID entity, float samplingInterval = 0.1f,
                        const glm::vec3& color = glm::vec3(1.0f, 0.0f, 0.0f))
    {
        if (!mEntityManager->hasComponent<Tracking>(entity)) {
            Tracking tracking;
            tracking.samplingInterval = samplingInterval;
            tracking.traceColor = color;
            tracking.isTracking = true;
            mEntityManager->addComponent(entity, tracking);
        } else {
            auto* tracking = mEntityManager->getComponent<Tracking>(entity);
            if (tracking) {
                tracking->isTracking = true;
                tracking->samplingInterval = samplingInterval;
                tracking->traceColor = color;
            }
        }
    }

    // Skrur av tracking for Entity
    void disableTracking(EntityID entity)
    {
        bbl::Tracking* tracking = mEntityManager->getComponent<Tracking>(entity);
        if (tracking)
        {
            tracking->isTracking = false;
        }
    }

    // Slett tracking data for en Entity
    void clearTracking(EntityID entity)
    {
        bbl::Tracking* tracking = mEntityManager->getComponent<Tracking>(entity);
        if (tracking) {
            TrackingSystem::clearTrace(*tracking);
        }
    }

    // Linje vertices for rendering
    std::vector<Vertex> getTrackingVertices(EntityID entity) const
    {
        std::vector<Vertex> vertices;

        bbl::Tracking* tracking = mEntityManager->getComponent<Tracking>(entity);
        if (!tracking || tracking->curvePoints.empty()) {
            return vertices;
        }

        // Konverter kurve punkter til linjer
        for (const auto& point : tracking->curvePoints)
        {
            Vertex vertex;
            vertex.pos = point;
            vertex.color = tracking->traceColor;
            // Set defaults for other vertex attributes
            vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
            vertex.texCoord = glm::vec2(0.0f, 0.0f);

            vertices.push_back(vertex);
        }

        return vertices;
    }

    // Få verticene til en tracked Entity
    std::vector<Vertex> getAllTrackingVertices() const
    {
        std::vector<Vertex> allVertices;

        std::vector<EntityID> trackedEntities = mEntityManager->getEntitiesWith<Transform, Tracking>();

        for (EntityID entity : trackedEntities) {
            std::vector<Vertex> entityVertices = getTrackingVertices(entity);
            allVertices.insert(allVertices.end(), entityVertices.begin(), entityVertices.end());
        }

        return allVertices;
    }

    // Sett tracking parametere for en Entity
    void setTrackingParameters(EntityID entity, float samplingInterval,
                               size_t maxControlPoints, size_t curveResolution)
    {
        bbl::Tracking* tracking = mEntityManager->getComponent<Tracking>(entity);
        if (tracking) {
            tracking->samplingInterval = samplingInterval;
            tracking->maxControlPoints = maxControlPoints;
            tracking->curveResolution = curveResolution;
        }
    }

    // Sett tracking farge
    void setTrackingColor(EntityID entity, const glm::vec3& color)
    {
        bbl::Tracking* tracking = mEntityManager->getComponent<Tracking>(entity);
        if (tracking)
        {
            tracking->traceColor = color;
        }
    }

    void updateTraceRenderData()
    {
        if (!mEntityManager) return;

        auto trackedEntities = mEntityManager->getEntitiesWith<Transform, Tracking>();

        for (EntityID entity : trackedEntities) {
            bbl::Tracking* tracking = mEntityManager->getComponent<Tracking>(entity);
            if (!tracking || !tracking->isTracking || tracking->curvePoints.empty()) {
                continue;
            }

            // Lag eller oppdater trace Entity for rendering
            updateTraceEntity(entity, *tracking);
        }
    }


private:
    EntityManager* mEntityManager;

    void updateTracking(EntityID entity, const Transform& transform, Tracking& tracking)
    {
        // Sjekk for om vi burde prøve et nytt punkt
        if (TrackingSystem::shouldSample(tracking))
        {
            // Legg til nåværende posisjon som et kontroll punkt
            TrackingSystem::addControlPoint(tracking, transform.position);
            TrackingSystem::updateSampleTime(tracking);
        }
    }

    void updateTraceEntity(EntityID ballEntity, const Tracking& tracking)
    {
        EntityID traceEntity = ballEntity + 10000; // La til offset for å unngå feil

        // Sjekker først om traceEntity eksisterer
        if (!mEntityManager->isValidEntity(traceEntity))
        {
            traceEntity = mEntityManager->createEntityWithID(traceEntity);

            // Legger til en transform component
            mEntityManager->addComponent(traceEntity, Transform{});
        }

        // Konverterer punktene til Ballen til mesh data
        if (tracking.curvePoints.size() >= 2) {
            bbl::MeshData meshData = createLineMeshFromPoints(tracking.curvePoints, tracking.traceColor);

            // Laster opp mesh data til GPUen og oppdaterer/lager Render component
            if (GPUResourceManager* gpuResources = mEntityManager->getGPUResourceManager())
            {
                auto meshResourceID = gpuResources->uploadMesh(meshData);

                // Oppdaterer eller legger til Render component
                if (!mEntityManager->hasComponent<Render>(traceEntity)) {
                    mEntityManager->addComponent(traceEntity, Render{});
                }

                // Setter default verdier
                auto* renderComp = mEntityManager->getComponent<Render>(traceEntity);
                if (renderComp)
                {
                    renderComp->meshResourceID = meshResourceID;
                    renderComp->textureResourceID = 0; // Trenger ingen tekstur for linjer
                    renderComp->visible = true;
                    renderComp->useLine = true; // Viktig at vi bruker denne pipelinen
                    renderComp->usePhong = false;
                    renderComp->usePoint = false;
                }
            }
        }
    }

    bbl::MeshData createLineMeshFromPoints(const std::vector<glm::vec3>& points, const glm::vec3& color)
    {
        bbl::MeshData meshData;

        // Lager vertices for linje segmenter
        for (size_t i = 0; i < points.size(); ++i)
        {
            Vertex vertex;
            vertex.pos = points[i];
            vertex.color = color;
            vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f); // Blir ikke brukt for linjer
            vertex.texCoord = glm::vec2(0.0f, 0.0f);     // Blir ikke brukt for linjer
            meshData.vertices.push_back(vertex);
        }

        // Lager indices for linje segmenter, lager forbindelse mellom sammenhengende punkter
        for (size_t i = 0; i < points.size() - 1; ++i)
        {
            meshData.indices.push_back(static_cast<uint32_t>(i));
            meshData.indices.push_back(static_cast<uint32_t>(i + 1));
        }

        return meshData;
    }


};

} // namespace bbl

#endif // TRACKINGSYSTEM_CLASS_H
