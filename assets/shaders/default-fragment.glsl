#version 430 core
out vec4 fragColor;

in vec2 texcoord;
uniform sampler2D tileset;

void main()
{
    fragColor = texture(tileset, texcoord);
}