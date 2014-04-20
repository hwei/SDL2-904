#version 150

uniform vec4 ScreenTransfrom;

in vec2 position;
in vec2 texcoord;

out vec2 Texcoord;

void main()
{
    gl_Position = vec4(
        position.x * ScreenTransfrom.x + ScreenTransfrom.z,
        position.y * ScreenTransfrom.y + ScreenTransfrom.w,
        0.5, 1.0);
    Texcoord = texcoord / 128.0;
}
