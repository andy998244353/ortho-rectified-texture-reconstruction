#version 330 core

out float fragOut;

in float t1;
in float t2;
in float t3;
in float offset;

uniform float depth_tolerance;

void main()
{
	if(t1 < 0 || t2 < 0 || t3 < 0)
		discard;
	if(offset < depth_tolerance)
		discard;
	fragOut = offset / 2;	
}
