#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out vec2 texCoord;
out vec3 normal;
out vec3 p_pos;

void main()
{
    gl_Position = vec4(aTexCoord*2 - 1, 0, 1);
	texCoord = aTexCoord;
	normal = aNormal;
	p_pos = aPos;
}
