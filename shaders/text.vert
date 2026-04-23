// Vertex shader for text rendering
#version 450

layout(location = 0) in vec2 inPos;    // position in NDC
layout(location = 1) in vec2 inUV;     // texture coordinate

layout(location = 0) out vec2 fragUV;

layout(push_constant) uniform PushConsts {
    float scaleX;
    float scaleY;
} pc;

void main() {
    vec2 scaled = inPos;
    scaled.x *= pc.scaleX;
    scaled.y *= pc.scaleY;
    gl_Position = vec4(scaled, 0.0, 1.0);
    fragUV = inUV;
}
