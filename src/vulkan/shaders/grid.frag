#version 450

layout(location = 0) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform GridParams {
	float alpha;
};

void main()
{
	outColor = vec4(fragColor, alpha);
}
