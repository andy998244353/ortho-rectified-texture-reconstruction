#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 2) in vec2 texcoord;
out vec2 texc;

void main()
{
    gl_Position = vec4(aPos, 1.0);
    texc = texcoord;
}
