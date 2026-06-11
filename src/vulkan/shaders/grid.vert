#version 450
layout(location = 0) in vec2 inPosition;
layout(set = 0, binding = 0) uniform GridUBO { vec4 color; float alpha; } ubo;
layout(location = 0) out vec4 fragColor;
void main() {
    gl_Position = vec4(inPosition, 0.0, 1.0);
    fragColor = vec4(ubo.color.rgb, ubo.color.a * ubo.alpha);
}
