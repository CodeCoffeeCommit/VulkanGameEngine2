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

// Per-object data via push constants (same as mesh shader)
layout(push_constant) uniform PushConstants {
    mat4 model;
} push;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = ubo.projection * ubo.view * push.model * vec4(inPosition, 1.0);
    fragColor = inColor;
}