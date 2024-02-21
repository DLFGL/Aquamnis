#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;    
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragPosWorld;
layout(location = 3) out vec3 fragNormalWorld;

struct PointLight
{
    vec4 color;
    vec3 position;
};

layout(set = 0, binding = 0) 
uniform GlobalUBO
{
    PointLight pointLights[10];
    mat4 view;
    mat4 invView;
    mat4 proj;
    vec4 ambientColor;
    vec3 directLightDirection;
    int numLights;
} globalUBO;

layout(set = 1, binding = 0)
uniform EntityUBO
{
    mat4 transform;
    mat4 normalMat;
	vec4 color;
	float radius;
} entityUBO;

layout(push_constant)
uniform Push
{
    mat4 normalMat;
    mat4 transform;
} push;

void main() 
{
    vec4 worldPosition = push.transform * vec4(inPosition, 1.0);
    gl_Position = push.normalMat * worldPosition; // globalUBO.proj * globalUBO.view * worldPosition;

    fragNormalWorld = normalize(mat3(push.normalMat) * inNormal);
    fragPosWorld = worldPosition.xyz;
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}