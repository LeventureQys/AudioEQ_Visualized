#version 450
layout(location = 0) in vec2 inPosition;
layout(location = 1) in float inDist;
layout(set = 0, binding = 0) uniform CurveUBO { vec4 color; float lineWidth; } ubo;
layout(location = 0) out float vDist;
layout(location = 1) out vec4 vColor;
void main() {
    gl_Position = vec4(inPosition, 0.0, 1.0);
    vDist = inDist;
    vColor = ubo.color;
}
