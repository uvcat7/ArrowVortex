#version 450

layout(set = 1, binding = 0) uniform sampler2D tex;

layout(location = 0) in vec2 fragUvs;

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = texture(tex, fragUvs);
}

void mainAlpha()
{
    outColor = vec4(texture(tex, fragUvs).r, 1, 1, 1);
}