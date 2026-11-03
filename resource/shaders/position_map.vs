#version 330 core
layout (location = 0) in vec3 aPos;

uniform vec3 p1;
uniform vec3 p2;
uniform vec3 p3;
uniform vec2 uv1; // u, v \in [-1, 1]
uniform vec2 uv2;
uniform vec2 uv3;
uniform vec3 n; // outward, normalized
uniform float proxy_tolerance;
uniform float pointsize;

out vec3 pos;
out float offset;

void main()
{
	float t1, t2, t3;
	// Möller Trumbore Algorithm
	vec3 e1 = p2 - p1;
	vec3 e2 = p3 - p1;
	vec3 s = aPos - p1;
	vec3 s1 = cross(n, e2);
	vec3 s2 = cross(s, e1);
	vec3 distance_t2_t3 = (1/dot(s1, e1)) * vec3(dot(s2, e2), dot(s1, s), dot(s2, n));
	float distance = distance_t2_t3.x;
	t2 = distance_t2_t3.y;
	t3 = distance_t2_t3.z;
	t1 = 1 - t2 - t3;
	pos = aPos;
	gl_Position = vec4(
		t1 * uv1 + t2 * uv2 + t3 * uv3,
		distance / proxy_tolerance,
		1.0
	);
	gl_PointSize = pointsize;
	offset = -distance;
}
