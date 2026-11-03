#version 330 core
out vec3 FragColor;
in vec2 texc;

uniform sampler2D color;
uniform sampler2D weight;
  
void main()
{
	vec3 c = texture(color, texc).xyz;
	float w = texture(weight, texc).x;
	if(w <= 0) discard;
	FragColor = c / w;
}
