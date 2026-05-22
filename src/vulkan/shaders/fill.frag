#version 450

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform FillParams {
	vec4 fillColor;
};

void main()
{
	outColor = fillColor;
}
