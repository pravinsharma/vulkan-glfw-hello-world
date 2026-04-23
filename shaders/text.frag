// Fragment shader for text rendering
#version 450

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D tex;

void main() {
    float alpha = texture(tex, fragUV).r;
    // Orange color for Hello World!
    outColor = vec4(1.0, 0.5, 0.2, alpha);
}
