#ifndef WORLD_H
#define WORLD_H
#include<stdlib.h>
#include<SDL2/SDL.h>
#include<glad/glad.h>

#include"util.h"
#include"math3d.h"
#include"shaders.h"

#ifdef DEBUG
#include"timer.h"
#endif

#define CHUNK_SIZE 32 // The maximum width and depth of chunks, in number of blocks
#define CHUNK_MAX_HEIGHT 256 // The maximum height of chunks, in number of blocks
#define CHUNK_INITIAL_ALLOC_BLOCKS 256 // The number of blocks to allocate vertex & index space for when a chunk is created. Increasing this reduces the number of allocations, but uses more memory

#define CHUNK_INDEX_TEXTURE_SIZE 2048 // The size of the texture used to store the indices which specify the texture to use for each cube

typedef enum { CUBE_FACE_FRONT = 0b00000001,  CUBE_FACE_BACK   = 0b00000010, 
               CUBE_FACE_LEFT  = 0b00000100,  CUBE_FACE_RIGHT  = 0b00001000, 
               CUBE_FACE_TOP   = 0b00010000,  CUBE_FACE_BOTTOM = 0b00100000, 
               CUBE_FACE_ALL   = 0b00111111 } CUBE_FACES;
typedef enum { EMPTY, SOIL } BLOCK_TYPE;
typedef enum { CHUNK_EMPTY, CHUNK_PARTIALLY_FULL, CHUNK_FULL } CHUNK_FILL_STATE;

typedef struct CUBE
{
    BLOCK_TYPE type;
    unsigned long vertex_location, index_location, num_indices;
} CUBE;

typedef struct CHUNK
{
    mat4 tranform;
    POINT position;
    VERTEX* vertices;
    unsigned int vertex_array_object, vertex_buffer, index_buffer, *indices, chunk_index_texture;
    unsigned long num_vertices, num_indices, vertex_capacity, index_capacity;
    unsigned char* index_texture_data;
    CUBE* cubes;
    CHUNK_FILL_STATE fill_state[CHUNK_MAX_HEIGHT];
} CHUNK;

CHUNK* current_chunk = NULL;
CUBE empty_cube = { 0 };

POINT cube_vertex_positions[] = {
//  Front
    0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    1.0f, 1.0f, 0.0f,
//  Back
    0.0f, 0.0f, -1.0f,
    0.0f, 1.0f, -1.0f,
    1.0f, 0.0f, -1.0f,
    1.0f, 1.0f, -1.0f
};

vec2 cube_texcoords[] = {
    0.0f, 0.0f,
    0.0f, 1.0,
    1.0f, 0.0f,
    1.0f, 1.0f
};

unsigned int cube_face_front[]  = { 0, 1, 2, 2, 1, 3 };
unsigned int cube_face_back[]   = { 4, 6, 5, 5, 6, 7 };
unsigned int cube_face_left[]   = { 4, 5, 0, 0, 5, 1 };
unsigned int cube_face_right[]  = { 2, 3, 6, 6, 3, 7 };
unsigned int cube_face_top[]    = { 1, 5, 3, 3, 5, 7 };
unsigned int cube_face_bottom[] = { 0, 2, 6, 6, 4, 0 };
unsigned int* faces[] = { cube_face_front, cube_face_back, cube_face_left, cube_face_right, cube_face_top, cube_face_bottom };

#define TILESET_SIZE 64 // Number of tiles in the texture - width and height
#define TILE_TEXTURE_SIZE 16 // Width and height of each texture, in pixels 

// For each of the vertices above, which texcoord to use (by index, from the list above) when rendering the given face
unsigned int front_face_coords[]   = { 0, 1, 2, 3, 0, 0, 0, 0 };
unsigned int back_face_coords[]    = { 0, 0, 0, 0, 0, 1, 2, 3 };
unsigned int left_face_coords[]    = { 0, 1, 0, 0, 2, 3, 0, 0 };
unsigned int right_face_coords[]   = { 0, 0, 0, 1, 0, 0, 2, 3 }; 
unsigned int top_face_coords[]     = { 0, 0, 0, 2, 0, 1, 0, 3 };
unsigned int bottom_face_coords[]  = { 0, 0, 2, 0, 1, 0, 3, 0 };
unsigned int* face_texcoords[] = { front_face_coords, back_face_coords, left_face_coords, right_face_coords, top_face_coords, bottom_face_coords };

vec2 block_texcoords[] = {{.x = 0.0f, .y = 0.0f }};

#define NUM_TILESETS 1
const char* tileset_paths[] = { "assets/tilesets/main.png" };
typedef enum { MAIN_TILESET } TILESET_TYPE;
unsigned int tilesets[NUM_TILESETS] = { 0 }, current_tileset = 0;

void apply_tileset(TILESET_TYPE to_apply)
{
    if(!tilesets[to_apply])
    {
        int image_width, image_height, image_components;
        FILE* image_file = fopen(tileset_paths[to_apply], "rb");
        if(!image_file) exit_with_error("Could not load main tileset from file", tileset_paths[to_apply]);
        unsigned char* image_data = stbi_load_from_file(image_file, &image_width, &image_height, &image_components, 0);
        if(!image_data) exit_with_error("Could not parse tileset (PNG) from file", tileset_paths[to_apply]);
        fclose(image_file);

        glGenTextures(1, tilesets + to_apply);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tilesets[to_apply]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

        // Args: texture to work with, mipmap level, format to store the texture (RGB), width of texture, height of texture, legacy (ignore), texture format, data type, data
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image_width, image_height, 0, GL_RGB, GL_UNSIGNED_BYTE, image_data);
        // glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(image_data);
    }

    glBindTexture(GL_TEXTURE_2D, tilesets[to_apply]);
    current_tileset = tilesets[to_apply];
}

VERTEX cube_vertex(unsigned char position_index, unsigned char face_index, POINT place_at, vec3 size, BLOCK_TYPE block_type) 
{ 
    VERTEX to_return;
    to_return.position = vec3_add_vec3(vec3_scale(cube_vertex_positions[position_index], size), place_at);

    vec2 face_uv_scale;
    CUBE_FACES face = 1 << face_index;
    if(face == CUBE_FACE_FRONT || face == CUBE_FACE_BACK) face_uv_scale = v2(size.x, size.y);
    else if(face == CUBE_FACE_LEFT || face == CUBE_FACE_RIGHT) face_uv_scale = v2(size.z, size.y);
    else if (face == CUBE_FACE_TOP || face == CUBE_FACE_BOTTOM) face_uv_scale = v2(size.x, size.z);

    to_return.uv = vec2_scale_vec2(cube_texcoords[face_texcoords[face_index][position_index]], face_uv_scale);
    return to_return;
}

void cube_faces(CUBE* cube_to_add, POINT position, vec3 size, CUBE_FACES faces_to_add)
{
    if(current_chunk->num_vertices + 8 > current_chunk->vertex_capacity)
    {
        // Allocate more space for vertices and indices. Since there are more indices than vertices per cube, the number of indices grows faster as well
        current_chunk->vertices = realloc(current_chunk->vertices, (current_chunk->vertex_capacity *= 2) * sizeof(VERTEX));
        current_chunk->indices = realloc(current_chunk->indices, (current_chunk->index_capacity *= 4.5) * sizeof(unsigned int));
    }

    cube_to_add->vertex_location = current_chunk->num_vertices;
    cube_to_add->index_location = current_chunk->num_indices;

    // Copy only the required vertices into the chunk's vertex list
    // char vertices_to_copy = 0;
    // if(faces_to_add & 1)        // Front
    //     vertices_to_copy |= 0b11110000;
    // if(faces_to_add & (1 << 1)) // Back
    //     vertices_to_copy |= 0b00001111;
    // if(faces_to_add & (1 << 2)) // Left
    //     vertices_to_copy |= 0b11001100;
    // if(faces_to_add & (1 << 3)) // Right
    //     vertices_to_copy |= 0b00110011;
    // if(faces_to_add & (1 << 4)) // Top
    //     vertices_to_copy |= 0b01010101;
    // if(faces_to_add & (1 << 5)) // Bottom
    //     vertices_to_copy |= 0b10101010;

    unsigned int num_vertices_added = 0, num_indices_added = 0, indices_added[8] = { 0 };
    // for(char i = 7; i >= 0; i--)
    // {
    //     if(vertices_to_copy & (1 << i))
    //     {
    //             current_chunk->vertices[current_chunk->num_vertices + num_vertices_added] = cube_vertex(index_loc[j], 0, position, size, cube_to_add->type);
    //             indices_added[7 - i] = current_chunk->num_vertices + num_vertices_added++;
    //     }
    // }

    unsigned int next_index, *index_loc;
    for(unsigned char i = 0; i < 6; i++)
    {
        if(faces_to_add & (1 << i))
        {
            // Copy the indices for the current face
            index_loc = current_chunk->indices + current_chunk->num_indices + num_indices_added;
            memcpy(index_loc, faces[i], sizeof(unsigned int) * 6);
            memset(indices_added, 0, 8 * sizeof(unsigned int));

            // Copy the corresponding vertices for all the indices
            for(unsigned char j = 0; j < 6; j++)
            {
                if(!indices_added[index_loc[j]])
                {
                    current_chunk->vertices[current_chunk->num_vertices + num_vertices_added++] = cube_vertex(index_loc[j], i, position, size, cube_to_add->type);
                    indices_added[index_loc[j]] = current_chunk->num_vertices + num_vertices_added - 1;
                }
                index_loc[j] = indices_added[index_loc[j]];
            }
            num_indices_added += 6;
        }
    }

    current_chunk->num_vertices += num_vertices_added;
    current_chunk->num_indices += num_indices_added;
    cube_to_add->num_indices = num_indices_added;
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

// Generates the vertices and indices for a chunk based on its cube array.
void recalculate_chunk_model()
{
    POINT block_position;
    unsigned int i = 1, range_min = 0, range_max = CHUNK_MAX_HEIGHT;
    while(range_max - range_min > 1 && i < CHUNK_MAX_HEIGHT)
    {
        if(current_chunk->fill_state[i - 1] == CHUNK_FULL)
        {
            block_position = at(0.0f, range_min, 0.0f);
            cube_faces(get_cube(block_position), block_position, v3(CHUNK_SIZE, range_max, CHUNK_SIZE), CUBE_FACE_ALL);
        }
        return;
    }
    return;

    // for(long long i = 0; i < CHUNK_SIZE; i++)
    // {
    //     for(long long j = 0; j < CHUNK_MAX_HEIGHT; j++)
    //     {
    //         for(long long k = 0; k < CHUNK_SIZE; k++)
    //         {  
    //             POINT cube_position = at(i, j, -k);
    //             CUBE* to_place = get_cube(cube_position);
    //             if(to_place->type != EMPTY)
    //             {                    
    //                 // If there is no adjacent cube in each direction, add a face.
    //                 CUBE_FACES faces_to_add = 0;
    //                 if(get_cube(at(i, j, -k + 1))->type == EMPTY)
    //                     faces_to_add |= CUBE_FACE_FRONT;
    //                 if(get_cube(at(i, j, -k - 1))->type == EMPTY)
    //                     faces_to_add |= CUBE_FACE_BACK;
    //                 if(get_cube(at(i - 1, j, -k))->type == EMPTY)
    //                     faces_to_add |= CUBE_FACE_LEFT;
    //                 if(get_cube(at(i + 1, j, -k))->type == EMPTY)
    //                     faces_to_add |= CUBE_FACE_RIGHT;
    //                 if(get_cube(at(i, j + 1, -k))->type == EMPTY)
    //                     faces_to_add |= CUBE_FACE_TOP;
    //                 if(j && get_cube(at(i, j - 1, -k))->type == EMPTY)
    //                     faces_to_add |= CUBE_FACE_BOTTOM;
                  
    //                 // faces_to_add = CUBE_FACE_RIGHT;
    //                 cube_faces(to_place, cube_position, faces_to_add);
    //             }
    //         }
    //     }
    // }
}

void place_block(BLOCK_TYPE type, POINT position)
{
    // Set the locations of the current cube's vertices and indices in the chunk's buffers
    get_cube(at(position.x, position.y, position.z))->type = type;
    recalculate_chunk_model();
}

void remove_block(POINT position)
{
    get_cube(at(position.x, position.y, position.z))->type = EMPTY;
    recalculate_chunk_model();
}

// This is separated out because it's possible to create chunks in the existing chunk buffer
CHUNK* allocate_chunk_memory()
{
    CHUNK* to_return = calloc(1, sizeof(CHUNK));
    to_return->vertex_capacity = CHUNK_INITIAL_ALLOC_BLOCKS * 8;
    to_return->index_capacity = CHUNK_INITIAL_ALLOC_BLOCKS * 36;
    to_return->vertices = calloc(to_return->vertex_capacity, sizeof(VERTEX));
    to_return->indices = calloc(to_return->index_capacity, sizeof(unsigned int));
    to_return->cubes = calloc(CHUNK_SIZE * CHUNK_MAX_HEIGHT * CHUNK_SIZE, sizeof(CUBE));
    to_return->index_texture_data = calloc(CHUNK_INDEX_TEXTURE_SIZE * CHUNK_INDEX_TEXTURE_SIZE, 1);
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
        for(unsigned int j = 0; j < CHUNK_MAX_HEIGHT; j++)
        {
            for(unsigned int k = 0; k < CHUNK_SIZE; k++)
            {
                get_cube(at((float)i, (float)j, -((float)k)))->type = SOIL;
            }
        }
    }
    current_chunk->fill_state[0] = CHUNK_FULL;
    recalculate_chunk_model();
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
    glBufferData(GL_ARRAY_BUFFER, sizeof(VERTEX) * to_return->num_vertices, to_return->vertices, GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * to_return->num_indices, to_return->indices, GL_STATIC_DRAW);

    // Generate the texture index, then load the indices of all the vertices into it
    glGenTextures(1, &(current_chunk->chunk_index_texture));
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, current_chunk->chunk_index_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG8UI, CHUNK_INDEX_TEXTURE_SIZE, CHUNK_INDEX_TEXTURE_SIZE, 0, GL_RG8UI, GL_UNSIGNED_INT, current_chunk->index_texture_data);

    configure_vertex_properties(VERTEX_POSITION | VERTEX_UV);
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
    free(to_free->index_texture_data);
    free(to_free->cubes);
    free(to_free->vertices);
    free(to_free->indices);
    free(to_free);
}

#endif