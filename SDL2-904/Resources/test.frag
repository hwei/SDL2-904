#version 150

uniform vec3 Color;
uniform sampler2D TexSampler;

in vec2 Texcoord;

out vec4 outColor;

void main()
{
    outColor = texture(TexSampler, Texcoord) * vec4(Color, 1.0);
}
