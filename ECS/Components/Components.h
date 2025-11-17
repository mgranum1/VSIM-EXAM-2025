#ifndef COMPONENT_H
#define COMPONENT_H

#include "AL/al.h"
#include <qobject.h>
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <vector>

namespace bbl
{
using EntityID = uint32_t;

enum class ComponentType
{
    Transform,
    Mesh,
    Texture,
    Audio,
    Physics,
    Collision,
    Input,
    Render
};

// COMPONENT STRUCTS
// All components are pure data - no behavior, no entity IDs stored inside

struct Transform
{
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    glm::vec3 rotation{0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f, 1.0f, 1.0f};

    // Helper method to compute model matrix
    glm::mat4 getModelMatrix() const {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, position);
        model = glm::rotate(model, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, scale);
        return model;
    }
};

struct Mesh
{
    size_t meshResourceID{0};
    std::string modelPath = "";
    size_t meshIndex = 0;
};

struct Texture
{
    size_t textureResourceID{0};
    std::string texturePath;
};

struct Render
{
    size_t meshResourceID{0};
    size_t textureResourceID{0};
    bool visible{true};
    bool usePhong{false};
    float opacity{1.0f};
};

struct Audio
{
    float volume{1.0f};
    bool muted{false};
    bool looping{false};

    std::string attackSound = "../../Assets/Sounds/";
    std::string deathSound = "../../Assets/Sounds/";

    // OpenAL source and buffer
    ALuint attackBuffer = 0;
    ALuint attackSource = 0;

    ALuint deathBuffer = 0;
    ALuint deathSource = 0;
};

struct Collision
{
    glm::vec3 colliderSize{1.0f, 1.0f, 1.0f};
    bool isGrounded{false};
    bool isColliding{false};
    bool isTrigger{false};
    bool isStatic{false};
};

struct Physics
{
    glm::vec3 velocity{0.0f, 0.0f, 0.0f};
    glm::vec3 acceleration{0.0f, 0.0f, 0.0f};
    float mass{1.0f};
    bool useGravity{true};
};

// struct Input
// {
//     // Movement keys
//     bool W{false};
//     bool A{false};
//     bool S{false};
//     bool D{false};

//     // Arrow keys
//     bool UP{false};
//     bool DOWN{false};
//     bool LEFT{false};
//     bool RIGHT{false};

//     // Action keys
//     bool Q{false};
//     bool E{false};
//     bool R{false};
//     bool C{false};

//     // Modifier keys
//     bool LSHIFT{false};
//     bool LCTRL{false};
//     bool SPACE{false};

//     // Mouse buttons
//     bool LMB{false};   // Left Mouse Button
//     bool RMB{false};   // Right Mouse Button
//     bool MMB{false};   // Middle Mouse Button
//     bool MB4{false};   // Mouse Button 4
//     bool MB5{false};   // Mouse Button 5

//     // Mouse data
//     float mouseWheelDelta{0.0f};
//     int mouseX{0};
//     int mouseY{0};
//     int mouseDeltaX{0};  // Added for mouse movement
//     int mouseDeltaY{0};  // Added for mouse movement
// };

} // namespace bbl

#endif // COMPONENT_H
