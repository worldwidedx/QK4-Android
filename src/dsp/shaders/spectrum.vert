#version 440

layout(location = 0) in vec2 position;
layout(location = 1) in vec4 vertexColor;

layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    vec2 viewportSize;
    vec2 padding;  // Alignment padding
};

void main() {
    vec2 ndc = (position / viewportSize) * 2.0 - 1.0;
    ndc.y = -ndc.y;  // Flip Y for Qt coordinate system
    gl_Position = vec4(ndc, 0.0, 1.0);
    fragColor = vertexColor;
}
