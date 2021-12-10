#ifndef WORLD_H
#define WORLD_H
#include<stdlib.h>
#include<glad/glad.h>

#include"math3d.h"
#include"shaders.h"

#define CHUNK_SIZE 32
#define MAX_CHUNK_HEIGHT 100

typedef enum { FRONT_FACING_OUT, BACK_FACING_OUT, LEFT_FACING_OUT, RIGHT_FACING_OUT, TOP_FACING_OUT, BOTTOM_FACING_OUT } CUBE_FACE;
typedef enum { DIRT } BLOCK_TYPE;

typedef struct CUBE
{
    BLOCK_TYPE type;
} CUBE;

typedef struct CHUNK
{
    mat4 tranform;
    float* vertices;
    unsigned int vertex_array_object, vertex_buffer, index_buffer, *indices;
    unsigned long num_vertices, num_indices;
    CUBE cubes[CHUNK_SIZE][CHUNK_SIZE][MAX_CHUNK_HEIGHT];
} CHUNK;

float cube_v[] = {
    // Front
    0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 
    1.0f, 0.0f, 0.0f,
    1.0f, 1.0f, 0.0f,
    // Back
    0.0f, 0.0f, -1.0f,
    0.0f, 1.0f, -1.0f, 
    1.0f, 0.0f, -1.0f,
    1.0f, 1.0f, -1.0f,
};

unsigned int cube_face_front_out[] =  { 0, 1, 2, 2, 1, 3 };
unsigned int cube_face_back_out[] =   { 4, 6, 5, 5, 6, 7 };
unsigned int cube_face_left_out[] =   { 4, 5, 0, 0, 5, 1 };
unsigned int cube_face_right_out[] =  { 2, 3, 6, 6, 3, 7 };
unsigned int cube_face_top_out[] =    { 1, 5, 3, 3, 5, 7 };
unsigned int cube_face_bottom_out[] = { 0, 2, 6, 6, 4, 0 };
unsigned int* faces[] = { cube_face_front_out, cube_face_back_out, cube_face_left_out, cube_face_right_out, cube_face_top_out, cube_face_bottom_out };

void cube_vertices(float* vertex_array, unsigned long* num_vertices, vec3 position)
{   
    memcpy(vertex_array + *num_vertices, cube_v, sizeof(vec3) * 8);
    *num_vertices += 8;
}

void cube_face(unsigned int* index_array, unsigned long* num_indices, CUBE_FACE face)
{
    memcpy(index_array + *num_indices, faces[face], sizeof(unsigned int) * 6);
    *num_indices += 6;
}

CHUNK* make_chunk()
{
    CHUNK* to_return = calloc(1, sizeof(CHUNK));
    to_return->vertices = calloc(8, sizeof(vec3));
    to_return->indices = calloc(64, sizeof(unsigned int));
    to_return->tranform = m4();

    cube_vertices(to_return->vertices, &(to_return->num_vertices), v3(0.0f, 0.0f, 0.0f));
    cube_face(to_return->indices, &(to_return->num_indices), FRONT_FACING_OUT);
    cube_face(to_return->indices, &(to_return->num_indices), BACK_FACING_OUT);
    cube_face(to_return->indices, &(to_return->num_indices), LEFT_FACING_OUT);
    cube_face(to_return->indices, &(to_return->num_indices), RIGHT_FACING_OUT);
    cube_face(to_return->indices, &(to_return->num_indices), TOP_FACING_OUT);
    cube_face(to_return->indices, &(to_return->num_indices), BOTTOM_FACING_OUT);

    glGenVertexArrays(1, &(to_return->vertex_array_object));
    glBindVertexArray(to_return->vertex_array_object);
    glGenBuffers(1, &(to_return->vertex_buffer));
    glGenBuffers(1, &(to_return->index_buffer));
    glBindBuffer(GL_ARRAY_BUFFER, to_return->vertex_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, to_return->index_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * to_return->num_vertices, to_return->vertices, GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * to_return->num_indices, to_return->indices, GL_STATIC_DRAW);
    configure_vertex_properties(VERTEX_POSITION);
    return to_return;
}

void render_chunk(CHUNK* to_render)
{
    glBindVertexArray(to_render->vertex_array_object);
    set_shader_value(MODEL_MATRIX, &(to_render->tranform));
    glDrawElements(GL_TRIANGLES, to_render->num_indices, GL_UNSIGNED_INT, 0);
}

void unload_chunk(CHUNK* to_free)
{
    free(to_free->vertices);
    free(to_free->indices);
    free(to_free);
}

#endif