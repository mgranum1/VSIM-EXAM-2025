#include "Camera.h"
#include "../Editor/MainWindow.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

Camera::Camera()
{
    init();
}

Camera::Camera(float x, float y, float z)
{
    init();
    position = glm::vec3(x, y, z);
}

void Camera::init()
{
    position = glm::vec3(0.0f, 0.0f, 5.0f);
    Up = glm::vec3(0.0f, 1.0f, 0.0f);
    yaw   = -90.0f;
    pitch =  0.0f;
    moveSpeed = 15.0f;
    fov = 70;
    updateVectors();
}

void Camera::update()
{
    updateVectors();
}

void Camera::updateVectors()
{
    float radYaw   = glm::radians(yaw);
    float radPitch = glm::radians(pitch);

    glm::vec3 dir;
    dir.x = cos(radYaw) * cos(radPitch);
    dir.y = sin(radPitch);
    dir.z = sin(radYaw) * cos(radPitch);

    forward = glm::normalize(dir);

    right = glm::normalize(glm::cross(forward, Up));
    up    = glm::normalize(glm::cross(right, forward));

}

glm::mat4 Camera::getViewMatrix() const
{

    return glm::lookAt(position, position + forward, up);
}

glm::mat4 Camera::getProjectionMatrix(float aspectRatio, float fov, float nearPlane, float farPlane) const
{
    return glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
}

void Camera::updateFrustum(float aspectRatio, float fov, float nearPlane, float farPlane)
{

    glm::mat4 projection = getProjectionMatrix(aspectRatio, fov, nearPlane, farPlane);
    glm::mat4 view = getViewMatrix();
    glm::mat4 viewProj = projection * view;

    // Extracting frustum planes baby
    // Left plane
    m_frustum.planes[Frustum::LEFT].normal.x = viewProj[0][3] + viewProj[0][0];
    m_frustum.planes[Frustum::LEFT].normal.y = viewProj[1][3] + viewProj[1][0];
    m_frustum.planes[Frustum::LEFT].normal.z = viewProj[2][3] + viewProj[2][0];
    m_frustum.planes[Frustum::LEFT].distance = viewProj[3][3] + viewProj[3][0];

    // Right plane
    m_frustum.planes[Frustum::RIGHT].normal.x = viewProj[0][3] - viewProj[0][0];
    m_frustum.planes[Frustum::RIGHT].normal.y = viewProj[1][3] - viewProj[1][0];
    m_frustum.planes[Frustum::RIGHT].normal.z = viewProj[2][3] - viewProj[2][0];
    m_frustum.planes[Frustum::RIGHT].distance = viewProj[3][3] - viewProj[3][0];

    // Bottom plane
    m_frustum.planes[Frustum::BOTTOM].normal.x = viewProj[0][3] + viewProj[0][1];
    m_frustum.planes[Frustum::BOTTOM].normal.y = viewProj[1][3] + viewProj[1][1];
    m_frustum.planes[Frustum::BOTTOM].normal.z = viewProj[2][3] + viewProj[2][1];
    m_frustum.planes[Frustum::BOTTOM].distance = viewProj[3][3] + viewProj[3][1];

    // Top plane
    m_frustum.planes[Frustum::TOP].normal.x = viewProj[0][3] - viewProj[0][1];
    m_frustum.planes[Frustum::TOP].normal.y = viewProj[1][3] - viewProj[1][1];
    m_frustum.planes[Frustum::TOP].normal.z = viewProj[2][3] - viewProj[2][1];
    m_frustum.planes[Frustum::TOP].distance = viewProj[3][3] - viewProj[3][1];

    // Near plane
    m_frustum.planes[Frustum::NEAR].normal.x = viewProj[0][3] + viewProj[0][2];
    m_frustum.planes[Frustum::NEAR].normal.y = viewProj[1][3] + viewProj[1][2];
    m_frustum.planes[Frustum::NEAR].normal.z = viewProj[2][3] + viewProj[2][2];
    m_frustum.planes[Frustum::NEAR].distance = viewProj[3][3] + viewProj[3][2];

    // Far plane
    m_frustum.planes[Frustum::FAR].normal.x = viewProj[0][3] - viewProj[0][2];
    m_frustum.planes[Frustum::FAR].normal.y = viewProj[1][3] - viewProj[1][2];
    m_frustum.planes[Frustum::FAR].normal.z = viewProj[2][3] - viewProj[2][2];
    m_frustum.planes[Frustum::FAR].distance = viewProj[3][3] - viewProj[3][2];

    // Normalize all planes
    for (int i = 0; i < 6; i++)
    {
        m_frustum.planes[i].normalize();
    }
}

void Camera::processInput(bool w, bool a, bool s, bool d, bool q, bool e, float deltaTime)
{
    float speed = moveSpeed * deltaTime;

    if (w) moveForward(speed);
    if (s) moveForward(-speed);
    if (a) moveRight(-speed);
    if (d) moveRight(speed);
    if (e) moveUp(speed);
    if (q) moveUp(-speed);

    update();
}

// Movement helpers
void Camera::moveForward(float delta)
{
    position += forward * delta;
}

void Camera::moveRight(float delta)
{
    position += right * delta;
}

void Camera::moveUp(float delta)
{
    position += Up * delta;
}

void Camera::addYaw(float delta)
{
    yaw += delta;
}

void Camera::addPitch(float delta)
{
    pitch += delta;

    // Clamp pitch so we don’t flip
    const float limit = 89.0f;
    if (pitch >  limit) pitch =  limit;
    if (pitch < -limit) pitch = -limit;
}
