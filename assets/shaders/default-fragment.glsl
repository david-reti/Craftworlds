#version 430 core
out vec4 fragColor;

in vec2 texcoord;
layout(binding = 0) uniform sampler2D tileset;
layout(binding = 1) uniform usampler2D chunk_index_texture;

void main()
{
    float tile_size = 64;
    vec2 scale = vec2(texcoord.x - int(texcoord.x), texcoord.y - int(texcoord.y));
    uvec2 texture_index = uvec2(texelFetch(chunk_index_texture, ivec2(int(texcoord.x), int(texcoord.y)), 0));
    // fragColor = texture(tileset, vec2(scalex, scaley));
    fragColor = texture(tileset, (scale + texture_index) / tile_size);
}