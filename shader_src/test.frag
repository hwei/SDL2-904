#version 150

uniform sampler2D TexSampler;

in vec2 Texcoord;
in vec4 ColorMult;
in vec3 ColorAdd;

out vec4 outColor;

void main()
{
    outColor = texture(TexSampler, Texcoord) * ColorMult + vec4(ColorAdd, 0.0);
}
