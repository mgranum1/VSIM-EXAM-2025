#version 450
layout(location = 0) out vec4 outColor;
layout(binding = 1) uniform sampler2D texSampler;
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 lightPos;
layout(location = 4) in vec3 viewPos;
layout(location = 5) in vec3 lightDir;
layout(location = 6) in float lightIntensity;

void main()
{
    bool blinn = true;
    vec3 color = texture(texSampler, inTexCoord).rgb;


    vec3 lightDirection = normalize(lightDir);

    // ambient
    vec3 ambient = 0.15 * color;

    // diffuse - using the light direction for directional lighting
    vec3 normal = normalize(inNormal);
    float diff = max(dot(-lightDirection, normal), 0.0);
    vec3 diffuse = diff * color * lightIntensity;

    // specular
    vec3 viewDir = normalize(viewPos - inPos);
    float spec = 0.0;
    if(blinn)
    {
        vec3 halfwayDir = normalize(-lightDirection + viewDir);
        spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
    }
    else
    {
        vec3 reflectDir = reflect(lightDirection, normal);
        spec = pow(max(dot(viewDir, reflectDir), 0.0), 8.0);
    }
    vec3 specular = vec3(0.3) * spec * lightIntensity;

    outColor = vec4(ambient + diffuse + specular, 1.0);
}
