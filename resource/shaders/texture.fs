#version 330 core

layout(location = 0) out vec3 color;
layout(location = 1) out float weight;

in vec2 texCoord;
uniform sampler2D image;
uniform sampler2D depth;
uniform sampler2D position;
uniform sampler2D mask;
uniform float eps;
uniform float angle_threshold;
uniform mat4 view;
uniform mat4 proj;
in vec3 normal;
in vec3 p_pos;

void main()
{
	float offset = texture(position, texCoord).x;
	if(isinf(offset))
		discard;
	vec3 pos = p_pos + offset * normalize(normal);
	vec3 cam_pos = (inverse(view) * vec4(0, 0, 0, 1)).xyz;
	vec3 vdir = cam_pos - pos;
	float cos_val = dot(vdir, normal) / (length(vdir)*length(normal));
	if( cos_val < angle_threshold )
		discard;
	gl_FragDepth = 1 - cos_val;
	vec4 c = proj * view * vec4(pos, 1);

	c = vec4(c.x / c.w, c.y / c.w, c.z / c.w, 1);
	if(c.x < -1 || c.x > 1 || c.y < -1 || c.y > 1 || c.z < -1 || c.z > 1)
		discard;
	
	float d1 = texture(depth, vec2(c.xy/2 + 0.5)).r;
	float d2 = -(view * vec4(pos, 1)).z;

	if(abs(d1 - d2) > eps)
		discard;

	weight = cos_val;
	if(texture(mask, vec2(c.x/2 + 0.5, c.y/2 + 0.5)).x < 0.5)
		discard;
	color = texture(image, vec2(c.x/2 + 0.5, c.y/2 + 0.5)).xyz;
	color = color * weight;

}
