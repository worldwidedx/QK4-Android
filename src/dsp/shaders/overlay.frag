#version 440

layout(location = 0) out vec4 outColor;

layout(std140, binding = 0) uniform buf {
    vec2 viewportSize;
    vec2 padding;
    vec4 color;
};

void main() {
    outColor = color;
}
