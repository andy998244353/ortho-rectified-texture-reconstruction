#version 330 core

in vec3 pos;
in float offset;

layout(location = 0) out vec3 fragOut;
layout(location = 1) out float fragOut1;

uniform vec3 n;
uniform int width;
uniform int height;
uniform vec3 p1;
uniform vec3 p2;
uniform vec3 p3;
uniform vec2 uv1; // u, v \in [-1, 1]
uniform vec2 uv2;
uniform vec2 uv3;
uniform float point_radius;
uniform float proxy_tolerance;

uniform sampler2D tolerance_map;

void main()
{
	vec2 uv = vec2((gl_FragCoord.x / width)*2 - 1, (gl_FragCoord.y / height)*2 - 1);

	if( offset > texture(tolerance_map, (uv+1) / 2).x )
		discard;

	float t1, t2, t3;
	t3 = ((uv1.y - uv2.y)*uv.x + (uv2.x - uv1.x)*uv.y + uv1.x*uv2.y - uv2.x*uv1.y) / ((uv1.y - uv2.y)*uv3.x + (uv2.x - uv1.x)*uv3.y + uv1.x*uv2.y - uv2.x*uv1.y);
	t2 = ((uv1.y - uv3.y)*uv.x + (uv3.x - uv1.x)*uv.y + uv1.x*uv3.y - uv3.x*uv1.y) / ((uv1.y - uv3.y)*uv2.x + (uv3.x - uv1.x)*uv2.y + uv1.x*uv3.y - uv3.x*uv1.y);
	t1 = 1 - t2 - t3;
	if(t1 < 0 || t2 < 0 || t3 < 0)
		discard;

	vec3 p = t1*p1 + t2*p2 + t3*p3;
	float dist;
	dist = length(cross(n, pos - p));
	if(dist > point_radius)
		discard;

	fragOut = vec3( (offset / proxy_tolerance + 1) / 2);
	fragOut1 = offset;
}
