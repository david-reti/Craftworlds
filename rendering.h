#ifndef RENDERING_H
#define RENDERING_H
#include"shaders.h"
#include<glad/glad.h>

typedef struct BLOCK_VERTEX
{
    vec3 position;
    vec2 uv, uv2;
} BLOCK_VERTEX;

typedef struct SKY_VERTEX
{
    vec3 position;
    vec3 colour;
} SKY_VERTEX;

typedef struct MODEL
{
    void* vertices;
    VERTEX_PROPERTY vertex_properties;
    unsigned int vertex_array_object, vertex_buffer, index_buffer, *indices;
    unsigned long num_vertices, num_indices, vertex_capacity, index_capacity;
    bool deallocate;
} MODEL;

#define NUM_PREDEFINED_MODELS 1
typedef enum { SKY_MODEL } MODEL_TYPE;
const char* model_names[] = { "sphere" };
VERTEX_PROPERTY model_vertex_properties[] = { VERTEX_POSITION | VERTEX_COLOUR };

extern char _binary_build_predefined_models_start[];
extern char _binary_build_predefined_models_end[];
extern char _binary_build_predefined_models_length[];

#ifndef ONLY_INCLUDE_DEFINITIONS
// Allocates memory for a new model with the specified vertex properties
// Optionally specify the number of vertices and indices to initially allocate memory for (default 256).
// Optionally pass a pointer to an existing vertex and index array, in this case the model will wrap these pointers instead of allocating memory. Make sure you still pass the number of vertices and indices, the model needs it to render properly
MODEL* make_model(VERTEX_PROPERTY vertex_properties, size_t num_vertices, size_t num_indices, void* vertices, unsigned int* indices)
{
    MODEL* to_return = calloc(1, sizeof(MODEL));
    to_return->vertex_properties = vertex_properties;
    if(vertices)
    {
        to_return->num_vertices = num_vertices;
        to_return->num_indices = num_indices;
        to_return->vertices = vertices;
        to_return->indices = indices;
        to_return->vertex_capacity = num_vertices;
        to_return->index_capacity = num_indices;
        return to_return;
    }
    
    to_return->vertex_capacity = num_vertices > 256 ? num_vertices : 256;
    to_return->index_capacity = num_indices > 256 ? num_indices : 256;
    to_return->vertices = calloc(to_return->vertex_capacity, vertex_size(vertex_properties));
    to_return->indices = calloc(to_return->index_capacity, sizeof(unsigned int));
    to_return->deallocate = true;
    return to_return;
}

// Takes the vertex and index data stored inside the model, and generates opengl objects from them
void finalise_model(MODEL* to_finalise)
{
    glGenVertexArrays(1, &(to_finalise->vertex_array_object));
    glBindVertexArray(to_finalise->vertex_array_object);
    glGenBuffers(1, &(to_finalise->vertex_buffer));
    glGenBuffers(1, &(to_finalise->index_buffer));
    glBindBuffer(GL_ARRAY_BUFFER, to_finalise->vertex_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, to_finalise->index_buffer);
    glBufferData(GL_ARRAY_BUFFER, vertex_size(to_finalise->vertex_properties) * to_finalise->num_vertices, to_finalise->vertices, GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * to_finalise->num_indices, to_finalise->indices, GL_STATIC_DRAW);
    configure_vertex_properties(to_finalise->vertex_properties);
}

// Creates a new model loaded with data from one of the predefined models built into the application (such as the sky)
MODEL* load_predefined_model(MODEL_TYPE to_load)
{
    unsigned long model_offset = *(((unsigned long*)_binary_build_predefined_models_start) + to_load);
    unsigned long num_vertices = *((unsigned long*)(_binary_build_predefined_models_start + model_offset));
    unsigned long num_indices = *((unsigned long*)(_binary_build_predefined_models_start + model_offset + sizeof(unsigned long))); 
    MODEL* to_return = make_model(VERTEX_POSITION, num_vertices, num_indices, 
                                  _binary_build_predefined_models_start + model_offset + (sizeof(unsigned long) * 2),
                                  (unsigned int*)(_binary_build_predefined_models_start + model_offset + (num_vertices * sizeof(vec3)) + (sizeof(unsigned long) * 2)));
    finalise_model(to_return);
    return to_return;
}

void render_model(MODEL* to_render)
{
    glBindVertexArray(to_render->vertex_array_object);
    glDrawElements(GL_TRIANGLES, to_render->num_indices, GL_UNSIGNED_INT, 0);
}

void unload_model(MODEL* to_unload)
{
    glDeleteBuffers(1, &(to_unload->vertex_buffer));
    glDeleteBuffers(1, &(to_unload->index_buffer));
    if(to_unload->deallocate)
    {
        free(to_unload->vertices);
        free(to_unload->indices);
    }
}
#endif
#endif