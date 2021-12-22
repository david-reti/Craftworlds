#version 430 core
layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec2 uv2;

layout (location = 3) uniform mat4 model;
layout (location = 4) uniform mat4 view;
layout (location = 5) uniform mat4 projection;

out vec2 texcoords;
out vec2 index_texcoords;

void main()
{
    gl_Position = projection * view * model  * vec4(pos, 1.0);
    texcoords = uv;
    index_texcoords = uv2;
}