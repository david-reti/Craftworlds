#version 430 core
out vec4 fragColor;

in vec4 vertex_colour;

void main()
{
    fragColor = vertex_colour;
}