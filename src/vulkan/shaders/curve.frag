#version 450
layout(location = 0) in float vDist;
layout(location = 1) in vec4 vColor;
layout(location = 0) out vec4 outColor;
layout(set = 0, binding = 0) uniform CurveUBO { vec4 color; float lineWidth; } ubo;
void main() {
    float halfWidth = ubo.lineWidth * 0.5;
    float alpha = 1.0 - smoothstep(halfWidth - 1.0, halfWidth, abs(vDist));
    outColor = vec4(vColor.rgb, vColor.a * alpha);
}
