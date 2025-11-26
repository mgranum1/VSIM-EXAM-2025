#include "../Game/Terrain.h"
#include "../External/stb_image.h"
#include "../Core/Utility/modelloader.h"
#include <iostream>
#include <cmath>
#include <QDebug>

Terrain::Terrain() : m_width (0), m_height(0), m_channels(0), m_heightScale(0.02f), m_gridSpacing(0.2f), m_heightPlacement(-5.0f)
{

}

Terrain::~Terrain()
{
}

// Laste inn punktsky data
bool Terrain::loadFromOBJ(const std::string& filepath)
{
    bbl::ModelLoader loader;
    std::unique_ptr<bbl::ModelData> modelData = loader.loadModel(filepath);
    if (!modelData || modelData->meshes.empty())
    {
        std::cerr << "Failed to load OBJ model: " << filepath << std::endl;
        return false;
    }

    m_vertices.clear();
    m_indices.clear();

    const bbl::MeshData& mesh = modelData->meshes[0];

    m_vertices.reserve(mesh.vertices.size());
    for (const Vertex& v : mesh.vertices)
    {
        Vertex vertex = v;
        vertex.color = glm::vec3(0.0f, 0.0f, 0.0f);
        m_vertices.push_back(vertex);
    }

    m_vertices.reserve(mesh.vertices.size());
    for (const Vertex& v : mesh.vertices)
    {
        m_vertices.push_back(v);
    }

    m_indices.reserve(mesh.indices.size());
    for (unsigned int index : mesh.indices)
    {
        m_indices.push_back(index);
    }


    // Rekalkuler normal hvis det skulle være nødvendig
    calculateNormals();

    // Lagre min og maks høyde for kollisjon handling
    float minHeight = FLT_MAX;
    float maxHeight = -FLT_MAX;
    for (const Vertex& v : m_vertices) {
        if (v.pos.y < minHeight) minHeight = v.pos.y;
        if (v.pos.y > maxHeight) maxHeight = v.pos.y;
    }
    qDebug() << "Loaded OBJ terrain with heights from" << minHeight << "to" << maxHeight;

    return true;
}

void Terrain::calculateNormals()
{
    // Sett normaler til å være null
    for (Vertex& vertex : m_vertices) {
        vertex.normal = glm::vec3(0.0f, 0.0f, 0.0f);
    }

    // Kalkuler normaler for hver trekant
    for (size_t i = 0; i < m_indices.size(); i += 3) {
        uint32_t idx0 = m_indices[i];
        uint32_t idx1 = m_indices[i + 1];
        uint32_t idx2 = m_indices[i + 2];

        glm::vec3 v0 = m_vertices[idx0].pos;
        glm::vec3 v1 = m_vertices[idx1].pos;
        glm::vec3 v2 = m_vertices[idx2].pos;

        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;
        glm::vec3 normal = glm::cross(edge1, edge2);

        m_vertices[idx0].normal += normal;
        m_vertices[idx1].normal += normal;
        m_vertices[idx2].normal += normal;
    }

    // Normaliser normaler
    for (Vertex& vertex : m_vertices)
    {
        if (glm::length(vertex.normal) > 0.0f)
        {
            vertex.normal = glm::normalize(vertex.normal);
        }

        else
        {
            vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }
    }
}

float Terrain::barycentric(const glm::vec2& p, const glm::vec3& a, const glm::vec3& b, const glm::vec3& c) const
{
    glm::vec2 a2D(a.x, a.z);
    glm::vec2 b2D(b.x, b.z);
    glm::vec2 c2D(c.x, c.z);

    glm::vec2 v0 = b2D - a2D;
    glm::vec2 v1 = c2D - a2D;
    glm::vec2 v2 = p - a2D;

    float d00 = glm::dot(v0, v0);
    float d01 = glm::dot(v0, v1);
    float d11 = glm::dot(v1, v1);
    float d20 = glm::dot(v2, v0);
    float d21 = glm::dot(v2, v1);

    float denom = d00 * d11 - d01 * d01;
    if (denom == 0.0f)
        return a.y;

    float v = (d11 * d20 - d01 * d21) / denom;
    float w = (d00 * d21 - d01 * d20) / denom;
    float u = 1.0f - v - w;

    return u * a.y + v * b.y + w * c.y;
}

float Terrain::getHeightAt(float worldX, float worldZ, const glm::vec3& terrainPosition) const
{
    if (m_vertices.empty() || m_indices.empty())
        return m_heightPlacement;

    float localWorldX = worldX - terrainPosition.x;
    float localWorldZ = worldZ - terrainPosition.z;

    // Finn trekanten som inneholder dette punktet
    for (size_t i = 0; i < m_indices.size(); i += 3) {
        const Vertex& v0 = m_vertices[m_indices[i]];
        const Vertex& v1 = m_vertices[m_indices[i + 1]];
        const Vertex& v2 = m_vertices[m_indices[i + 2]];

        // Sjekk om punktet er inni triangelen med 2D projeksjon
        if (isPointInTriangleXZ(glm::vec2(localWorldX, localWorldZ),
                              glm::vec2(v0.pos.x, v0.pos.z),
                              glm::vec2(v1.pos.x, v1.pos.z),
                              glm::vec2(v2.pos.x, v2.pos.z))) {

            // Kalkuler høyden ved hjelp av barysentriske koordinater
            return barycentric(glm::vec2(localWorldX, localWorldZ), v0.pos, v1.pos, v2.pos);
        }
    }

    return m_heightPlacement; // Punktet er ikke funnet i noen trekant
}

bool Terrain::isPointInTriangleXZ(const glm::vec2& p, const glm::vec2& a, const glm::vec2& b, const glm::vec2& c) const
{
    // Bruker barysentriske koordinater for å finne ut om punktet er innen for en trekant
    glm::vec2 v0 = c - a;
    glm::vec2 v1 = b - a;
    glm::vec2 v2 = p - a;

    float dot00 = glm::dot(v0, v0);
    float dot01 = glm::dot(v0, v1);
    float dot02 = glm::dot(v0, v2);
    float dot11 = glm::dot(v1, v1);
    float dot12 = glm::dot(v1, v2);

    float invDenom = 1.0f / (dot00 * dot11 - dot01 * dot01);
    float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
    float v = (dot00 * dot12 - dot01 * dot02) * invDenom;

    return (u >= 0) && (v >= 0) && (u + v <= 1);
}

glm::vec3 Terrain::getCenter() const
{
    return glm::vec3(0.0f, 0.0f, 0.0f);
}

// Skravere området rødt hvor det er ekstra mye friksjon
void Terrain::applyFrictionZoneColoring(const glm::vec3& zoneCenter, float radius, const glm::vec3& zoneColor)
{
    for (Vertex& vertex : m_vertices)
    {
        float distance = glm::length(vertex.pos - zoneCenter);

        // Alt innenfor radiusen hvor det er friksjonsone vil bli farget rødt
        if (distance <= radius)
        {

            vertex.color = glm::vec3(1.0f, 0.0f, 0.0f);
        }

        // Alt utenfor vil bli uberørt
        else
        {
            vertex.color = glm::vec3(0.0f, 0.0f, 0.0f);
        }
    }
}

// Brukes for å sette et friksjons område, tar inn hele terrainet og returner midt punktet av det
glm::vec3 Terrain::calculateBounds(glm::vec3& minBounds, glm::vec3& maxBounds) const
{
    if (m_vertices.empty())
    {
        minBounds = maxBounds = glm::vec3(0.0f);
        return glm::vec3(0.0f);
    }

    minBounds = maxBounds = m_vertices[0].pos;

    for (const Vertex& vertex : m_vertices)
    {
        minBounds.x = std::min(minBounds.x, vertex.pos.x);
        minBounds.y = std::min(minBounds.y, vertex.pos.y);
        minBounds.z = std::min(minBounds.z, vertex.pos.z);

        maxBounds.x = std::max(maxBounds.x, vertex.pos.x);
        maxBounds.y = std::max(maxBounds.y, vertex.pos.y);
        maxBounds.z = std::max(maxBounds.z, vertex.pos.z);
    }

    return (minBounds + maxBounds) * 0.5f; // Center point
}


