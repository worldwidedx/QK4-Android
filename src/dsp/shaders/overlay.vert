#version 440

layout(location = 0) in vec2 position;

layout(std140, binding = 0) uniform buf {
    vec2 viewportSize;
    vec2 padding;
    vec4 color;
};

void main() {
    vec2 ndc = (position / viewportSize) * 2.0 - 1.0;
    ndc.y = -ndc.y;  // Flip Y for Qt coordinate system
    gl_Position = vec4(ndc, 0.0, 1.0);
}
