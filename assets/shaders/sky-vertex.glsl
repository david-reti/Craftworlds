#version 430 core
layout(location = 0) in vec3 pos;
layout (location = 3) uniform mat4 model;
layout (location = 4) uniform mat4 view;
layout (location = 5) uniform mat4 projection;

out vec4 vertex_colour;

void main()
{
    vec4 horizon_colour = vec4(0.2, 0.4, 0.8, 1.0);
    vec4 zenith_colour = vec4(0.1, 0.3, 0.8, 1.0);
    gl_Position = projection * view * model  * vec4(pos, 1.0);
    vertex_colour = (max(pos.y, 0) * zenith_colour) + (max(2.0 - abs(pos.y), 0) * horizon_colour); 
}