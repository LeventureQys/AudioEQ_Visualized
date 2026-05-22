#version 450

layout(location = 0) in vec2 position;
layout(location = 1) in vec3 color;

layout(set = 0, binding = 0) uniform MVP {
	mat4 mvp;
};

layout(location = 0) out vec3 fragColor;

void main()
{
	gl_Position = mvp * vec4(position, 0.0, 1.0);
	fragColor = color;
}
