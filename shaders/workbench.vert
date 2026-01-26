#version 450

// Scene-wide data (must match C++ UniformBufferObject struct!)
layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 projection;
    vec3 lightDir;
    float _pad1;
    vec3 viewPos;
    float _pad2;
} ubo;

// Per-object data (changes every draw call)
layout(push_constant) uniform PushConstants {
    mat4 model;
} push;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragPos;

void main() {
    vec4 worldPos = push.model * vec4(inPosition, 1.0);
    gl_Position = ubo.projection * ubo.view * worldPos;
    
    fragPos = worldPos.xyz;
    fragNormal = mat3(transpose(inverse(push.model))) * inNormal;
    fragColor = inColor;
}