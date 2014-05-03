#version 150

in vec2 position;
in vec2 texcoord;
in vec4 color;

out vec2 Texcoord;
out vec4 ColorMult;
out vec3 ColorAdd;

void main()
{
    gl_Position = vec4(position, 0.5, 1.0);
    Texcoord = texcoord / 128.0;
    vec3 m = color.rgb / 16.0;
    vec3 n = floor(m);
    ColorMult = vec4(n / 15.0, color.a / 255.0);
    ColorAdd = (m - n) * 16.0 / 15.0;
}
