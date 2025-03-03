#version 450

layout(binding = 0) uniform UBO { vec2 resolution; } ubo;

layout(location = 0) in vec2 inPos;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 fragColor;

void main()
{
    fragColor = inColor;
    gl_Position = vec4(inPos / (ubo.resolution * 0.5) - vec2(1.0, 1.0), 0.0, 1.0);
}