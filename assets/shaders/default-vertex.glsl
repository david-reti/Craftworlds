#version 430 core
layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 uv;

layout (location = 2) uniform mat4 model;
layout (location = 3) uniform mat4 view;
layout (location = 4) uniform mat4 projection;

out vec2 texcoord;

void main()
{
    gl_Position = projection * view * model  * vec4(pos, 1.0);
    texcoord = uv;
}