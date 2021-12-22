#version 430 core
out vec4 fragColor;

in vec2 texcoords;
in vec2 index_texcoords;
layout(binding = 0) uniform sampler2DArray textures;
layout(binding = 1) uniform usampler2D chunk_index_texture;

void main()
{
    fragColor = vec4(1.0, 1.0, 1.0, 1.0);
}