#ifndef SHADERS_H
#define SHADERS_H
#include"util.h"

unsigned char* stbi_load_from_file(FILE*, int*, int*, int*, int);
void stbi_image_free(void*);

// Shader Value Layout - Ensure this is the same in the shaders and that update_shader_methods reflects it
// 1 - Model Matrix (tranformation - local -> world space)
// 2 - View Matrix (world -> camera space)
// 3 - Projection Matrix (camera -> clip space)
typedef enum { DEFAULT_VERTEX, DEFAULT_FRAGMENT } SHADER_TYPE;
typedef enum { MODEL_MATRIX = 2, VIEW_MATRIX = 3, PROJECTION_MATRIX = 4 } SHADER_VALUE;
char* shader_paths[] = {"assets/shaders/default-vertex.glsl", "assets/shaders/default-fragment.glsl"};
unsigned int shaders[2] = { 0 };
unsigned int current_shader_program = 0;

typedef struct VERTEX
{
    POINT position;
    vec2 uv;
} VERTEX;

unsigned int vertex_attrib_sizes[] = { 3, 2 };

// Update methods for each data type used in the shaders. Shader_update_methods is an array containing the method to use for each value.
void update_shader_matrix(unsigned int shader_program, SHADER_VALUE value, void* to_set) { glUniformMatrix4fv(value, 1, GL_FALSE, (const GLfloat*)((mat4*)to_set)->values); };
void (*shader_update_methods[])(unsigned int, SHADER_VALUE, void*) = { NULL, NULL, update_shader_matrix, update_shader_matrix, update_shader_matrix };

typedef enum { VERTEX_POSITION = 0b10000000, VERTEX_UV = 0b01000000 } VERTEX_PROPERTY;

unsigned int load_shader(unsigned long type, const char* path) // Internal
{
    size_t source_length = 0;
    FILE* shader_source_file = fopen(path, "r");
    if(!shader_source_file)
        exit_with_error("Could not open shader source file for compiling", path);

    // Load the source from the shader file
    fseek(shader_source_file, 0, SEEK_END);
    source_length = ftell(shader_source_file);
    fseek(shader_source_file, 0, SEEK_SET);

    char* source = calloc(source_length + 1, 1);
    fread(source, 1, source_length, shader_source_file);
    fclose(shader_source_file);

    // Create & compile the shader
    unsigned int to_return = glCreateShader(type);
    glShaderSource(to_return, 1, (const char**)&source, NULL);
    glCompileShader(to_return);
    free(source);

    // Check for errors, and print them if there are any
    int success;
    glGetShaderiv(to_return, GL_COMPILE_STATUS, &success);
    if(!success)
    {
        char error_info[1024];
        glGetShaderInfoLog(to_return, 1024, NULL, error_info);
        exit_with_error("Could not compile shader", error_info);
    }

    return to_return;
}

void apply_shader_program(unsigned int shader_program)
{
    glUseProgram(shader_program);
    current_shader_program = shader_program;
}

unsigned int link_program(unsigned int vertex_shader, unsigned int fragment_shader) // Internal
{
    // Create program & link shaders
    unsigned int to_return = glCreateProgram();
    glAttachShader(to_return, vertex_shader);
    glAttachShader(to_return, fragment_shader);
    glLinkProgram(to_return);

    // Check the program status
    int success;
    glGetProgramiv(to_return, GL_LINK_STATUS, &success);
    if(!success)
    {
        char error_info[1024];
        glGetProgramInfoLog(to_return, 1024, NULL, error_info);
        exit_with_error("Could not link shader program", error_info);
    }

    apply_shader_program(to_return);
    return to_return;
}

unsigned int shader_program(SHADER_TYPE vertex, SHADER_TYPE fragment)
{
    if(!shaders[vertex])
        shaders[vertex] = load_shader(GL_VERTEX_SHADER, shader_paths[vertex]);
    if(!shaders[fragment])
        shaders[fragment] = load_shader(GL_FRAGMENT_SHADER, shader_paths[fragment]);
    return link_program(shaders[vertex], shaders[fragment]);
}

void set_shader_value(SHADER_VALUE value, void* to_set) { shader_update_methods[value](current_shader_program, value, to_set); }

void unload_shaders()
{
    glDeleteShader(shaders[0]);
    glDeleteShader(shaders[1]);
}

void configure_vertex_properties(VERTEX_PROPERTY properties_to_use)
{
    unsigned long long stride = 0, offsets[8] = { 0 };
    for(unsigned int i = 0; i < 2; i++)
    {
        if(properties_to_use & (1 << (7 - i)))
        {
            stride += vertex_attrib_sizes[i] * sizeof(GLfloat);
            if(i > 0) offsets[i] = vertex_attrib_sizes[i - 1] * sizeof(GLfloat);
        }
    }

    for(unsigned int i = 0; i < 2; i++)
    {
        if(properties_to_use & (1 << (7 - i)))
        {
            // Args: Attribute to configure, num elems, type, whether to normalize, stride (between attrib in next vertex), offset(where it begins)
            glVertexAttribPointer(i, vertex_attrib_sizes[i], GL_FLOAT, GL_FALSE, stride, (void*)(offsets[i]));
            glEnableVertexAttribArray(i);
        }
    }
}

#endif