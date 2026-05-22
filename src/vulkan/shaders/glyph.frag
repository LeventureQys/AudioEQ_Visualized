#version 450

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform sampler2D fontTexture;

layout(set = 0, binding = 2) uniform GlyphParams {
	vec4 textColor;
};

void main()
{
	float a = texture(fontTexture, fragTexCoord).r;
	if (a < 0.05)
		discard;
	outColor = vec4(textColor.rgb, textColor.a * a);
}
