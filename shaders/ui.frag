#version 450

layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec4 fragColor;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D fontTexture;

void main() {
    if (fragUV.x < 0.0) {
        // Solid color (rectangles)
        outColor = fragColor;
    } else {
        // Text - sample font texture for alpha
        float alpha = texture(fontTexture, fragUV).r;
        outColor = vec4(fragColor.rgb, fragColor.a * alpha);
    }
}