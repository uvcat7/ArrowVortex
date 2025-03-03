#version 450

layout(binding = 0) uniform UBO { vec2 resolution; } ubo;

layout(location = 0) in vec2 inPos;
layout(location = 1) in vec2 inUvs;

layout(location = 0) out vec2 fragUvs;

void main()
{
    fragUvs = inUvs;
    gl_Position = vec4(inPos / (ubo.resolution * 0.5) - vec2(1.0, 1.0), 0.0, 1.0);
}