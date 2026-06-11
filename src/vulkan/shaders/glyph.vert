#version 450
layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(set = 0, binding = 0) uniform GlyphUBO { vec4 color; } ubo;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec4 vColor;
void main() {
    gl_Position = vec4(inPosition, 0.0, 1.0);
    vTexCoord = inTexCoord;
    vColor = ubo.color;
}
