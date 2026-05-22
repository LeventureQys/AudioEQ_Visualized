#version 450

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 texCoord;

layout(set = 0, binding = 0) uniform MVP {
	mat4 mvp;
};

layout(location = 0) out vec2 fragTexCoord;

void main()
{
	gl_Position = mvp * vec4(position, 0.0, 1.0);
	fragTexCoord = texCoord;
}
