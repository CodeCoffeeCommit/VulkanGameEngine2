#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 projection;
    vec3 lightDir;
    vec3 viewPos;
} ubo;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragPos;

layout(location = 0) out vec4 outColor;

void main() {
    // Blender workbench style lighting
    vec3 normal = normalize(fragNormal);
    
    // Studio lighting setup (3 lights like Blender)
    vec3 light1Dir = normalize(vec3(0.5, 0.7, 0.5));
    vec3 light2Dir = normalize(vec3(-0.5, 0.3, -0.5));
    vec3 light3Dir = normalize(vec3(0.0, -1.0, 0.0));
    
    // Light colors
    vec3 light1Color = vec3(1.0, 0.98, 0.95);   // Warm key light
    vec3 light2Color = vec3(0.6, 0.7, 0.8);      // Cool fill light
    vec3 light3Color = vec3(0.3, 0.3, 0.3);      // Rim light
    
    // Diffuse lighting
    float diff1 = max(dot(normal, light1Dir), 0.0);
    float diff2 = max(dot(normal, light2Dir), 0.0);
    float diff3 = max(dot(normal, -light3Dir), 0.0);
    
    // Ambient
    vec3 ambient = vec3(0.15);
    
    // Combine lights
    vec3 lighting = ambient;
    lighting += diff1 * light1Color * 0.6;
    lighting += diff2 * light2Color * 0.25;
    lighting += diff3 * light3Color * 0.15;
    
    // Final color
    vec3 result = fragColor * lighting;
    
    // Slight rim highlight for depth
    vec3 viewDir = normalize(ubo.viewPos - fragPos);
    float rim = 1.0 - max(dot(viewDir, normal), 0.0);
    rim = pow(rim, 3.0) * 0.1;
    result += vec3(rim);
    
    outColor = vec4(result, 1.0);
}