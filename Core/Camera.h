#pragma once
#include <glm/glm.hpp>

struct Plane
{
    glm::vec3 normal;
    float distance;

    void normalize()
    {
        float length = glm::length(normal);
        normal /= length;
        distance /= length;
    }
};

struct Frustum
{
    enum side { LEFT = 0, RIGHT, BOTTOM, TOP, NEAR , FAR};
    Plane planes[6];

    //Checks if point is inside of frustum culling
    bool containsPoint(const glm::vec3& point) const
    {
        for (int i = 0; i < 6; i++)
        {
            if (glm::dot(planes[i].normal, point)+ planes[i].distance < 0)
                return false;
        }
        return true;
    }

    // Check if a sphere is inside the frustum
    bool containsSphere(const glm::vec3& center, float radius) const
    {
        for (int i = 0; i < 6; i++)
        {
            float dist = glm::dot(planes[i].normal, center) + planes[i].distance;
            if (dist < -radius)
                return false;
        }
        return true;
    }

    // checks AABB
    bool containsAABB(const glm::vec3& min, const glm::vec3& max) const
    {
        for (int i = 0; i < 6; i++)
        {
            // Get the positive vertex
            glm::vec3 positiveVertex = min;
            if (planes[i].normal.x >= 0) positiveVertex.x = max.x;
            if (planes[i].normal.y >= 0) positiveVertex.y = max.y;
            if (planes[i].normal.z >= 0) positiveVertex.z = max.z;

            // Check if the positive vertex is outside
            if (glm::dot(planes[i].normal, positiveVertex) + planes[i].distance < 0)
                return false;
        }
        return true;
    }
};


class Camera
{
public:
    Camera();
    Camera(float x, float y, float z);

    void update();

    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix(float aspectRatio, float fov, float nearPlane = 0.1f, float farPlane = 1000.0f) const;

    // Frustum culling
    void updateFrustum(float aspectRatio, float fov, float nearPlane = 0.1f, float farPlane = 1000.0f);
    const Frustum& getFrustum() const { return m_frustum; }

    void moveForward(float delta);
    void moveRight(float delta);
    void moveUp(float delta);
    void addYaw(float delta);
    void addPitch(float delta);
    float getFov() const { return fov; }

    glm::vec3 getPosition() const { return position; }


    //Movement Helper for Camera. To make less clutter in UBO in Renderer
    void processInput(bool w, bool a, bool s, bool d, bool q, bool e, float deltaTime);


private:
    void init();
    void updateVectors();

private:
    glm::vec3 position;
    glm::vec3 forward;
    glm::vec3 right;
    glm::vec3 up;
    glm::vec3 Up;

    float moveSpeed;
    float yaw;
    float pitch;
    float fov = 90;

    Frustum m_frustum;
};
