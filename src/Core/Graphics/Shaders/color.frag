#version 450

layout(location = 0) in vec4 fragColor;

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = fragColor;
}

void mainHSV()
{
	vec4 k = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
	vec3 p = abs(fract(fragColor.xxx + k.xyz) * 6.0 - k.www);
	outColor = vec4(fragColor.z * mix(k.xxx, clamp(p - k.xxx, 0.0, 1.0), fragColor.y), 1.0);
}