#version 450
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec4 vColor;
layout(location = 0) out vec4 outColor;
layout(set = 0, binding = 1) uniform sampler2D fontAtlas;
void main() {
    float alpha = texture(fontAtlas, vTexCoord).r;
    outColor = vec4(vColor.rgb, vColor.a * alpha);
}
