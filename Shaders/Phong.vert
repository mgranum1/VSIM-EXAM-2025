#version 450
layout( binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 lightPos;
    vec3 viewPos;
    vec3 lightDir;
    float lightIntensity;
} ubo;

layout(push_constant) uniform PushConstants {
    mat4 model;
} pc;

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

layout(location = 0) out vec3 outPos;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outTexCoords;
layout(location = 3) out vec3 outLightPos;
layout(location = 4) out vec3 outViewPos;
layout(location = 5) out vec3 outLightDir;
layout(location = 6) out float outLightIntensity;

void main()
{
    // Transform position to world space
    vec4 worldPos = ubo.model * vec4(aPos, 1.0);
    outPos = worldPos.xyz;

    // Transform normal to world space
    mat3 normalMatrix = transpose(inverse(mat3(ubo.model)));
    outNormal = normalMatrix * aNormal;

    outTexCoords = aTexCoords;
    outLightPos = ubo.lightPos;
    outViewPos = ubo.viewPos;
    outLightDir = ubo.lightDir;
    outLightIntensity = ubo.lightIntensity;

    gl_Position = ubo.proj * ubo.view * worldPos;
}
