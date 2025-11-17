#ifndef TERRAIN_H
#define TERRAIN_H

#include "../Core/Utility/Vertex.h"
#include <vector>
#include <string>
#include <glm/glm.hpp>

class Terrain
{
public:
    Terrain();
    ~Terrain();

    bool loadFromHeightmap(const std::string& filepath,
                           float heightScale = 0.02f,
                           float gridSpacing = 0.2f,
                           float heightPlacement = -5.0f);

    float getHeightAt(float worldX, float worldZ, const glm::vec3& terrainPosition = glm::vec3(0.0f)) const;


    // Get mesh data
    const std::vector<Vertex>& getVertices() const { return m_vertices; }
    const std::vector<uint32_t>& getIndices() const { return m_indices; }

    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    float getGridSpacing() const { return m_gridSpacing; }
    glm::vec3 getCenter() const;

private:
    void generateMesh(unsigned char* textureData);
    void calculateNormals();
    float barycentric(const glm::vec2& p, const glm::vec3& a, const glm::vec3& b, const glm::vec3& c) const;

    // Terrain
    int m_width;
    int m_height;
    int m_channels;
    float m_heightScale;
    float m_gridSpacing;
    float m_heightPlacement;

    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;


    std::vector<float> m_heightData;
};

#endif // TERRAIN_H
