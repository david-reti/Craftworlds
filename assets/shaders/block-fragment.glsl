#version 430 core
out vec4 fragColor;

in vec2 texcoords;
in vec2 index_texcoords;
layout(binding = 0) uniform sampler2DArray textures;
layout(binding = 1) uniform usampler2D chunk_index_texture;

void main()
{
    uint texture_index = texelFetch(chunk_index_texture, ivec2(index_texcoords.x + int(texcoords.x), index_texcoords.y + int(texcoords.y)), 0).x;
    // uint texture_index = texelFetch(chunk_index_texture, ivec2(int(texcoords.x), int(texcoords.y)), 0).x;
    fragColor = texture(textures, vec3(texcoords, texture_index));
}