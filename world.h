#ifndef WORLD_H
#define WORLD_H
#include<stdlib.h>
#include<glad/glad.h>

#include"math3d.h"
#include"shaders.h"

#ifdef DEBUG
#include"timer.h"
#endif

#define CHUNK_SIZE 32
#define CHUNK_MAX_HEIGHT 100
#define CHUNK_INITIAL_ALLOC_BLOCKS 256 // The number of blocks to allocate vertex & index space for when a chunk is created. Increasing this reduces the number of allocations, but uses more memory

typedef enum { FRONT_FACING_OUT, BACK_FACING_OUT, LEFT_FACING_OUT, RIGHT_FACING_OUT, TOP_FACING_OUT, BOTTOM_FACING_OUT } CUBE_FACE;
typedef enum { EMPTY, SOIL } BLOCK_TYPE;

typedef struct CUBE
{
    BLOCK_TYPE type;
    unsigned long vertex_location, index_location, num_indices;
} CUBE;

typedef struct CHUNK
{
    mat4 tranform;
    POINT* vertices, position;
    unsigned int vertex_array_object, vertex_buffer, index_buffer, *indices;
    unsigned long num_vertices, num_indices, vertex_capacity, index_capacity;
    CUBE* cubes;
} CHUNK;

CHUNK* current_chunk = NULL;
CUBE empty_cube = { 0 };

POINT cube_v[] = {
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

void cube_vertices(POINT position)
{   
    memcpy(current_chunk->vertices + current_chunk->num_vertices, cube_v, sizeof(POINT) * 8);
    for(unsigned long i = current_chunk->num_vertices; i < current_chunk->num_vertices + 8; i++)
        current_chunk->vertices[i] = vec3_add_vec3(current_chunk->vertices[i], position);
    current_chunk->num_vertices += 8;
}

void cube_face(CUBE_FACE face)
{
    memcpy(current_chunk->indices + current_chunk->num_indices, faces[face], sizeof(unsigned int) * 6);
    for(unsigned long i = current_chunk->num_indices; i < current_chunk->num_indices + 6; i++)
        current_chunk->indices[i] += current_chunk->num_vertices - 8;
    current_chunk->num_indices += 6;
}

CUBE* get_cube(POINT point)
{
    long long x = (long long)point.x, y = (long long)point.y, z = (long long)point.z;
    if(x < 0 || y < 0 || x >= CHUNK_SIZE || y >= CHUNK_MAX_HEIGHT || z > 0 || abs(z) >= CHUNK_SIZE)
        return &empty_cube;
    return current_chunk->cubes + (x + (abs(z) * CHUNK_SIZE) + (y * CHUNK_SIZE * CHUNK_SIZE));
}

POINT top_cube(float x, float z)
{
    float y = CHUNK_MAX_HEIGHT;
    CUBE* on_top = get_cube(at(x, y, z));
    while(on_top->type == EMPTY && y > 0)
        on_top = get_cube(at(x, y--, z));
    if(on_top->type != EMPTY)
        return at(x, y, z);
    return at(0, 0, 0);
}

// TODO: Rewrite this entire function - it's so inefficient
void remove_cube_face(CUBE* remove_from, CUBE_FACE to_remove)
{
    // Search for the face to remove at the cube's location in the chunk's index buffer.
    // Only the first two indices need to be compared (every 6 indices) as they form a unique key 
    for(unsigned long i = remove_from->index_location; i < remove_from->index_location + remove_from->num_indices; i += 6)
    {
        if(current_chunk->indices[i] - remove_from->vertex_location == faces[to_remove][0] && current_chunk->indices[i + 1] - remove_from->vertex_location == faces[to_remove][1])
        {
            memcpy(&(current_chunk->indices[i]), &(current_chunk->indices[i + 6]), (current_chunk->num_indices - i - 6) * sizeof(unsigned int));
            current_chunk->num_indices -= 6;
            remove_from->num_indices -= 6;
            for(CUBE* i = current_chunk->cubes; i != current_chunk->cubes + (CHUNK_SIZE * CHUNK_MAX_HEIGHT * CHUNK_SIZE - 1); i++)
            {
                if(i->index_location > remove_from->index_location)
                    i->index_location -= 6;
            }
        }
    }
}

// This is meant good for interactively placing blocks, because it will check any existing blocks for conflicts. However, it is very slow
void place_block(BLOCK_TYPE type, POINT position)
{
    if(current_chunk->num_vertices + 8 > current_chunk->vertex_capacity)
    {
        // Allocate more space for vertices and indices. Since there are more indices than vertices per cube, the number of indices grows faster as well
        current_chunk->vertices = realloc(current_chunk->vertices, (current_chunk->vertex_capacity *= 2) * sizeof(POINT));
        current_chunk->indices = realloc(current_chunk->indices, (current_chunk->index_capacity *= 4.5) * sizeof(unsigned int));
    }

    // Set the locations of the current cube's vertices and indices in the chunk's buffers
    CUBE* to_place = get_cube(at(position.x, position.y, position.z));
    to_place->vertex_location = current_chunk->num_vertices;
    to_place->index_location = current_chunk->num_indices;

    cube_vertices(position);

    // If there is no adjacent cube in each direction, add a face. If there is already a cube there, remove the shared face
    if(get_cube(at(position.x, position.y, position.z + 1))->type == EMPTY)
        cube_face(FRONT_FACING_OUT);
    else
        remove_cube_face(get_cube(at(position.x, position.y, position.z + 1)), BACK_FACING_OUT);

    if(get_cube(at(position.x, position.y, position.z - 1))->type == EMPTY)
        cube_face(BACK_FACING_OUT);
    else
        remove_cube_face(get_cube(at(position.x, position.y, position.z - 1)), FRONT_FACING_OUT);

    if(get_cube(at(position.x - 1, position.y, position.z))->type == EMPTY)
        cube_face(LEFT_FACING_OUT);
    else
        remove_cube_face(get_cube(at(position.x - 1, position.y, position.z)), RIGHT_FACING_OUT);

    if(get_cube(at(position.x + 1, position.y, position.z))->type == EMPTY)
        cube_face(RIGHT_FACING_OUT);
    else
        remove_cube_face(get_cube(at(position.x + 1, position.y, position.z)), LEFT_FACING_OUT);

    if(get_cube(at(position.x, position.y + 1, position.z))->type == EMPTY)
        cube_face(TOP_FACING_OUT);
    else
        remove_cube_face(get_cube(at(position.x, position.y + 1, position.z)), BOTTOM_FACING_OUT);

    if(get_cube(at(position.x, position.y - 1, position.z))->type == EMPTY)
        cube_face(BOTTOM_FACING_OUT);
    else
        remove_cube_face(get_cube(at(position.x, position.y - 1, position.z)), TOP_FACING_OUT);

    to_place->num_indices = current_chunk->num_indices - to_place->index_location;
    to_place->type = type;
}

void remove_block(POINT position)
{
    
}

// Generates the vertices and indices for a chunk based on its cube array. Useful for generation, but one-way only - can't remove existing faces
void calculate_chunk_model()
{
    for(long long i = 0; i < CHUNK_SIZE; i++)
    {
        for(long long j = 0; j < CHUNK_MAX_HEIGHT; j++)
        {
            for(long long k = 0; k < CHUNK_SIZE; k++)
            {  
                CUBE* to_place = get_cube(at(i, j, -k));
                if(to_place->type != EMPTY)
                {
                    if(current_chunk->num_vertices + 8 > current_chunk->vertex_capacity)
                    {
                        // Allocate more space for vertices and indices. Since there are more indices than vertices per cube, the number of indices grows faster as well
                        current_chunk->vertices = realloc(current_chunk->vertices, (current_chunk->vertex_capacity *= 2) * sizeof(POINT));
                        current_chunk->indices = realloc(current_chunk->indices, (current_chunk->index_capacity *= 4.5) * sizeof(unsigned int));
                    }

                    to_place->vertex_location = current_chunk->num_vertices;
                    to_place->index_location = current_chunk->num_indices;
                    cube_vertices(at(i, j, -k));

                    // If there is no adjacent cube in each direction, add a face.
                    if(get_cube(at(i, j, -k + 1))->type == EMPTY)
                        cube_face(FRONT_FACING_OUT);
                    if(get_cube(at(i, j, -k - 1))->type == EMPTY)
                        cube_face(BACK_FACING_OUT);
                    if(get_cube(at(i - 1, j, -k))->type == EMPTY)
                        cube_face(LEFT_FACING_OUT);
                    if(get_cube(at(i + 1, j, -k))->type == EMPTY)
                        cube_face(RIGHT_FACING_OUT);
                    if(get_cube(at(i, j + 1, -k))->type == EMPTY)
                        cube_face(TOP_FACING_OUT);
                    if(get_cube(at(i, j - 1, -k))->type == EMPTY)
                        cube_face(BOTTOM_FACING_OUT);
                  
                    to_place->num_indices = current_chunk->num_indices - to_place->index_location;
                }
            }
        }
    }
}

// This is separated out because it's possible to create chunks in the existing chunk buffer
CHUNK* allocate_chunk_memory()
{
    CHUNK* to_return = calloc(1, sizeof(CHUNK));
    to_return->vertex_capacity = CHUNK_INITIAL_ALLOC_BLOCKS * 8;
    to_return->index_capacity = CHUNK_INITIAL_ALLOC_BLOCKS * 36;
    to_return->vertices = calloc(to_return->vertex_capacity, sizeof(POINT));
    to_return->indices = calloc(to_return->index_capacity, sizeof(unsigned int));
    to_return->cubes = calloc(CHUNK_SIZE * CHUNK_MAX_HEIGHT * CHUNK_SIZE, sizeof(CUBE));
    return to_return;
}

CHUNK* make_chunk(POINT position)
{
    CHUNK* to_return = allocate_chunk_memory();
    to_return->tranform = translate(position);
    current_chunk = to_return;

    #ifdef DEBUG
    LARGE_INTEGER chunk_gen_start_time, chunk_gen_end_time;
    QueryPerformanceCounter(&chunk_gen_start_time);
    #endif
    for(unsigned int i = 0; i < CHUNK_SIZE; i++)
    {
        for(unsigned int j = 0; j < 30; j++)
        {
            for(unsigned int k = 0; k < CHUNK_SIZE; k++)
            {
                get_cube(at((float)i, (float)j, -((float)k)))->type = SOIL;
            }
        }
    }
    calculate_chunk_model();
    #ifdef DEBUG
    QueryPerformanceCounter(&chunk_gen_end_time);
    double genTime = ((double)(chunk_gen_end_time.QuadPart - chunk_gen_start_time.QuadPart) / frequency.QuadPart);
    printf("Generated chunk at (%.2f, %.2f): took %lf seconds, %lu vertices, %lu indices\n", position.x, position.z, genTime, current_chunk->num_vertices, current_chunk->num_indices);
    #endif

    // Generate the VAO and buffers (vertex, index) for the chunk, then upload all the cubes to the buffers
    glGenVertexArrays(1, &(to_return->vertex_array_object));
    glBindVertexArray(to_return->vertex_array_object);
    glGenBuffers(1, &(to_return->vertex_buffer));
    glGenBuffers(1, &(to_return->index_buffer));
    glBindBuffer(GL_ARRAY_BUFFER, to_return->vertex_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, to_return->index_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(POINT) * to_return->num_vertices, to_return->vertices, GL_STATIC_DRAW);
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
    free(to_free->cubes);
    free(to_free->vertices);
    free(to_free->indices);
    free(to_free);
}

#endif