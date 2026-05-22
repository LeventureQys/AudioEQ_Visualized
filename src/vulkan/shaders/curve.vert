#version 450

layout(location = 0) in vec2 position;
layout(location = 1) in float edgeDistance;

layout(location = 0) out float fragEdgeDist;

layout(set = 0, binding = 0) uniform MVP {
	mat4 mvp;
};

void main()
{
	gl_Position = mvp * vec4(position, 0.0, 1.0);
	fragEdgeDist = edgeDistance;
}
