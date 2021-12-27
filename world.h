#ifndef WORLD_H
#define WORLD_H
#include<stdlib.h>
#include<SDL2/SDL.h>
#include<glad/glad.h>

#include"util.h"
#include"math3d.h"
#include"blocks.h"
#include"shaders.h"

#define CHUNK_SIZE 32 // The maximum width and depth of chunks, in number of blocks
#define CHUNK_MAX_HEIGHT 256 // The maximum height of chunks, in number of blocks
#define CHUNK_INITIAL_ALLOC_BLOCKS 4096 // The number of blocks to allocate vertex & index space for when a chunk is created. Increasing this reduces the number of allocations, but uses more memory
#define CHUNK_INDEX_TEXTURE_SIZE 2048 // The size of the texture used to store the indices which specify the texture to use for each cube

typedef enum { CUBE_FACE_FRONT = 0b00000001,  CUBE_FACE_BACK   = 0b00000010, 
               CUBE_FACE_LEFT  = 0b00000100,  CUBE_FACE_RIGHT  = 0b00001000, 
               CUBE_FACE_TOP   = 0b00010000,  CUBE_FACE_BOTTOM = 0b00100000, 
               CUBE_FACE_ALL   = 0b00111111 } CUBE_FACES;
typedef enum { CHUNK_EMPTY, CHUNK_PARTIALLY_FULL, CHUNK_FULL } CHUNK_FILL_STATE;

typedef struct CUBE
{
    BLOCK_TYPE type;
    unsigned long vertex_location, index_location, num_indices;
} CUBE;

typedef struct CUBE_TREE
{
    CHUNK_FILL_STATE full;
    vec3 min, size;
    struct CUBE_TREE* children[8];
} CUBE_TREE;

typedef struct CHUNK
{
    mat4 tranform;
    vec3 position;
    BLOCK_VERTEX* vertices;
    unsigned int vertex_array_object, vertex_buffer, index_buffer, *indices, index_texture, index_texture_offset_x, index_texture_offset_y;
    unsigned long num_vertices, num_indices, vertex_capacity, index_capacity, cube_child_buffer_loc;
    GLuint* index_texture_data;
    CUBE* cubes;
    CUBE_TREE cube_fill_state[8], *cube_child_buffer, *trees_to_update[256];
} CHUNK;

CHUNK* current_chunk = NULL;
CUBE empty_cube = { 0 };

vec3 cube_vertex_positions[] = {
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

// For each of the vertices above, which texcoord to use (by index, from the list above) when rendering the given face
unsigned int front_face_coords[]   = { 0, 1, 2, 3, 0, 0, 0, 0 };
unsigned int back_face_coords[]    = { 0, 0, 0, 0, 2, 3, 0, 1 };
unsigned int left_face_coords[]    = { 2, 3, 0, 0, 0, 1, 0, 0 };
unsigned int right_face_coords[]   = { 0, 0, 0, 1, 0, 0, 2, 3 }; 
unsigned int top_face_coords[]     = { 0, 0, 0, 2, 0, 1, 0, 3 };
unsigned int bottom_face_coords[]  = { 1, 0, 3, 0, 0, 0, 2, 0 };
unsigned int* face_texcoords[] = { front_face_coords, back_face_coords, left_face_coords, right_face_coords, top_face_coords, bottom_face_coords };
unsigned int block_textures;

vec3 tree_child_transformations[] = {
                                         0.0f, 0.0f, 0.0f,
                                         1.0f, 0.0f, 0.0f,
                                         0.0f, 0.0f, -1.0f,
                                         1.0f, 0.0f, -1.0f,
                                         0.0f, 1.0f, 0.0f,
                                         1.0f, 1.0f, 0.0f, 
                                         0.0f, 1.0f, -1.0f,
                                         1.0f, 1.0f, -1.0f
                                    };

vec3 full_chunk = { CHUNK_SIZE, CHUNK_SIZE, CHUNK_SIZE };

vec3 vec3_invert_if_negative_z(vec3 vector)
{
    if(vector.z < 0) vector.z *= -1;
    return vector;
}

// Returns a pointer to the node at the provided world-space position. 
// If stop_at_first_match is true, then it will stop at the first node which is completely full, or completely empty 
CUBE_TREE* cube_tree_find(CUBE_TREE* tree, vec3 position, bool stop_at_first_match)
{
    CUBE_TREE* cursor = tree;
    vec3 origin = cursor->min;
    while(!vec3_cmp(origin, position))
    {
        if(cursor->full == CHUNK_FULL || cursor->full == CHUNK_EMPTY)
            return cursor;
        origin = cursor->min;

        cursor = cursor->children[origin.y < position.y * 4 + origin.z > position.z * 2 + origin.x < position.x];
        if(cursor == NULL) return NULL;
    }
}

void cube_tree_fill(CUBE_TREE* tree, vec3 position)
{
    CUBE_TREE* cursor = tree;
    vec3 origin, size = full_chunk;
    // printf("%f %f %f\n", size.x, size.y, size.z);
    unsigned int num_to_update = 0, idx;
    while(!vec3_cmp(origin, position) || size.x > 1)
    {
        if(cursor->full == CHUNK_FULL) break;
        size = vec3_divide_scalar(size, 2);
        origin = vec3_add_vec3(vec3_invert_if_negative_z(cursor->min), size);
        idx = (origin.y <= position.y) * 4 + (-origin.z >= position.z) * 2 + (origin.x <= position.x);
        if(!cursor->children[idx])
        {
            // fprintf(stderr, "%u\n", idx);
            cursor->children[idx] = current_chunk->cube_child_buffer + current_chunk->cube_child_buffer_loc++;
            cursor->children[idx]->min = vec3_add_vec3(cursor->min, vec3_scale(size, tree_child_transformations[idx]));
            cursor->children[idx]->size = size;
            // printf("min: %f %f %f\n", cursor->children[idx]->min.x, cursor->children[idx]->min.y, cursor->children[idx]->min.z);
            // printf("size: %f %f %f\n", cursor->children[idx]->size.x, cursor->children[idx]->size.y, cursor->children[idx]->size.z);
            if(vec3_cmp(cursor->children[idx]->min, position) && cursor->children[idx]->size.x == 1) cursor->children[idx]->full = CHUNK_FULL;
        }
        
        current_chunk->trees_to_update[num_to_update++] = cursor;
        cursor = cursor->children[idx];
    }

    // printf("min: %f %f %f\n", cursor->min.x, cursor->min.y, cursor->min.z);
    // printf("size: %f %f %f\n", cursor->size.x, cursor->size.y, cursor->size.z);

    for(unsigned int i = 0; i < num_to_update; i++)
    {
        if(current_chunk->trees_to_update[i]->full == CHUNK_PARTIALLY_FULL)
        {
            current_chunk->trees_to_update[i]->full = CHUNK_FULL;
            for(unsigned char j = 0; j < 8; j++) 
                if(!current_chunk->trees_to_update[i]->children[j]) current_chunk->trees_to_update[i]->full = CHUNK_PARTIALLY_FULL;
            if(current_chunk->trees_to_update[i]->full == CHUNK_FULL)
            {
                for(unsigned char j = 0; j < 8; j++)
                    current_chunk->trees_to_update[i]->children[j] = NULL;
            }

        }
        else if(current_chunk->trees_to_update[i]->full == CHUNK_EMPTY) current_chunk->trees_to_update[i]->full = CHUNK_PARTIALLY_FULL;
    }
}

void cube_tree_empty(CUBE_TREE* tree, vec3 position)
{

}

void load_block_textures()
{
    glGenTextures(1, &block_textures);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, block_textures);

    // Use nearest filtering, but linear for mipmap filtering which I find minimizes artifacting
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, NUM_BLOCK_MIPMAP_LEVELS);

    // Load in the image data which was built into the executable
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, BLOCK_IMAGE_WIDTH, BLOCK_IMAGE_HEIGHT, NUM_BLOCK_TYPES, 0, GL_RGBA, GL_UNSIGNED_BYTE, _binary_build_block_images_start);

    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
}

BLOCK_VERTEX cube_vertex(unsigned char position_index, unsigned char face_index, vec3 place_at, vec3 size, BLOCK_TYPE block_type) 
{ 
    BLOCK_VERTEX to_return;
    to_return.position = vec3_add_vec3(vec3_scale(cube_vertex_positions[position_index], size), place_at);

    vec2 face_uv_scale;
    CUBE_FACES face = 1 << face_index;
    if(face == CUBE_FACE_FRONT || face == CUBE_FACE_BACK) face_uv_scale = v2(size.x, size.y);
    else if(face == CUBE_FACE_LEFT || face == CUBE_FACE_RIGHT) face_uv_scale = v2(size.z, size.y);
    else if (face == CUBE_FACE_TOP || face == CUBE_FACE_BOTTOM) face_uv_scale = v2(size.x, size.z);

    to_return.uv = vec2_scale_vec2(cube_texcoords[face_texcoords[face_index][position_index]], face_uv_scale);
    to_return.uv2 = v2(current_chunk->index_texture_offset_x, current_chunk->index_texture_offset_y);
    return to_return;
}

CUBE* get_cube(vec3 point)
{
    long long x = (long long)point.x, y = (long long)point.y, z = (long long)point.z;
    if(x < 0 || y < 0 || x >= CHUNK_SIZE || y >= CHUNK_MAX_HEIGHT || z > 0 || abs(z) >= CHUNK_SIZE)
        return &empty_cube;
    return current_chunk->cubes + (x + (abs(z) * CHUNK_SIZE) + (y * CHUNK_SIZE * CHUNK_SIZE));
}

void cube_faces(CUBE* cube_to_add, vec3 position, vec3 size, CUBE_FACES faces_to_add)
{
    if(current_chunk->num_vertices + 32 > current_chunk->vertex_capacity || current_chunk->num_indices + 32 > current_chunk->index_capacity)
    {
        // Allocate more space for vertices and indices. Since there are more indices than vertices per cube, the number of indices grows faster as well
        BLOCK_VERTEX* vertices; unsigned int* indices;
        if((vertices = realloc(current_chunk->vertices, current_chunk->vertex_capacity * 2 * sizeof(BLOCK_VERTEX))) == NULL)
            exit_with_error("Memory allocation error", "realloc() failed during chunk generation - likely run out of memory");
        if((indices = realloc(current_chunk->indices, current_chunk->index_capacity * 4 * sizeof(unsigned int))) == NULL)
            exit_with_error("Memory allocation error", "realloc() failed during chunk generation - likely ran out of memory");
        current_chunk->vertex_capacity *= 2;
        current_chunk->index_capacity *= 4;
        current_chunk->vertices = vertices; current_chunk->indices = indices;
    }

    cube_to_add->vertex_location = current_chunk->num_vertices;
    cube_to_add->index_location = current_chunk->num_indices;

    vec3 fill_to;
    CUBE_FACES face_to_add;
    BLOCK_TYPE face_cube_type;
    unsigned int num_vertices_added = 0, num_indices_added = 0, indices_added[8] = { 0 };
    unsigned int next_index, *index_loc, offset_x, offset_y, x_limit, greatest_y_offset;
    for(unsigned char i = 0; i < 6; i++)
    {
        face_to_add = 1 << i;
        if(faces_to_add & face_to_add)
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

            // Updates the index texture for the face by looping through each block on the face, and updating the texture coordinate with the block texture ID
            offset_x = current_chunk->index_texture_offset_x;
            offset_y = current_chunk->index_texture_offset_y;
            greatest_y_offset = 0;
            x_limit = size.x;
            
            // I might move this into its own function in the future, but I think it would be tricky because of the number of variables involved
            // TODO: Clean this up
            if(face_to_add == CUBE_FACE_FRONT)
            {
                if(size.y > greatest_y_offset) greatest_y_offset = size.y;
                for(float y = position.y; y < position.y + size.y; y++) 
                {
                    for(float x = position.x; x < position.x + size.x; x++)
                    {
                        unsigned long index = (offset_y * CHUNK_INDEX_TEXTURE_SIZE) + offset_x++;
                        face_cube_type = get_cube(at(x, y, position.z))->type ;
                        if(!block_face_textures[face_cube_type]) current_chunk->index_texture_data[index] = face_cube_type - 1;
                        else current_chunk->index_texture_data[index] = block_face_textures[face_cube_type][i];
                        if(offset_x == current_chunk->index_texture_offset_x + x_limit) { offset_x = current_chunk->index_texture_offset_x; offset_y++; }
                    }
                }
            }
            else if(face_to_add == CUBE_FACE_BACK)
            {
                if(size.y > greatest_y_offset) greatest_y_offset = size.y;
                for(float y = position.y; y < position.y + size.y; y++) 
                {
                    for(float x = position.x + size.x - 1; x > position.x - 1; x--)
                    {
                        unsigned long index = (offset_y * CHUNK_INDEX_TEXTURE_SIZE) + offset_x++;
                        face_cube_type = get_cube(at(x, y, position.z - size.z + 1))->type;
                        if(!block_face_textures[face_cube_type]) current_chunk->index_texture_data[index] = face_cube_type - 1;
                        else current_chunk->index_texture_data[index] = block_face_textures[face_cube_type][i];
                        if(offset_x == current_chunk->index_texture_offset_x + x_limit) { offset_x = current_chunk->index_texture_offset_x; offset_y++; }
                    }
                }
            }
            else if(face_to_add == CUBE_FACE_LEFT)
            {
                x_limit = size.z;
                if(size.y > greatest_y_offset) greatest_y_offset = size.y;
                for(float y = position.y; y < position.y + size.y; y++) 
                {
                    for(float z = position.z - size.z + 1; z < position.z + 1; z++)
                    {
                        unsigned long index = (offset_y * CHUNK_INDEX_TEXTURE_SIZE) + offset_x++;
                        face_cube_type = get_cube(at(position.x, y, z))->type;
                        if(!block_face_textures[face_cube_type]) current_chunk->index_texture_data[index] = face_cube_type - 1;
                        else current_chunk->index_texture_data[index] = block_face_textures[face_cube_type][i];
                        if(offset_x == current_chunk->index_texture_offset_x + x_limit) { offset_x = current_chunk->index_texture_offset_x; offset_y++; }
                    }
                }
            }
            else if(face_to_add == CUBE_FACE_RIGHT)
            {
                x_limit = size.z;
                if(size.y > greatest_y_offset) greatest_y_offset = size.y;
                for(float y = position.y; y < position.y + size.y; y++) 
                {
                    for(float z = position.z; z > position.z - size.z; z--)
                    {
                        unsigned long index = (offset_y * CHUNK_INDEX_TEXTURE_SIZE) + offset_x++;
                        face_cube_type = get_cube(at(position.x + size.x - 1, y, z))->type;
                        if(!block_face_textures[face_cube_type]) current_chunk->index_texture_data[index] = face_cube_type - 1;
                        else current_chunk->index_texture_data[index] = block_face_textures[face_cube_type][i];
                        if(offset_x == current_chunk->index_texture_offset_x + x_limit) { offset_x = current_chunk->index_texture_offset_x; offset_y++; }
                    }
                }
            }
            else if(face_to_add == CUBE_FACE_TOP)
            {
                if(size.z > greatest_y_offset) greatest_y_offset = size.z;
                for(float z = position.z; z > position.z - size.z; z--) 
                {
                    for(float x = position.x; x < position.x + size.x; x++)
                    {
                        unsigned long index = (offset_y * CHUNK_INDEX_TEXTURE_SIZE) + offset_x++;
                        face_cube_type = get_cube(at(x, position.y + size.y - 1, z))->type;
                        if(!block_face_textures[face_cube_type]) current_chunk->index_texture_data[index] = face_cube_type - 1;
                        else current_chunk->index_texture_data[index] = block_face_textures[face_cube_type][i];
                        if(offset_x == current_chunk->index_texture_offset_x + x_limit) { offset_x = current_chunk->index_texture_offset_x; offset_y++; }
                    }
                }
            }
            else if(face_to_add == CUBE_FACE_BOTTOM)
            {
                if(size.z > greatest_y_offset) greatest_y_offset = size.z;
                for(float z = position.z - size.z + 1; z < position.z + 1; z++) 
                {
                    for(float x = position.x; x < position.x + size.x; x++)
                    {
                        unsigned long index = (offset_y * CHUNK_INDEX_TEXTURE_SIZE) + offset_x++;
                        face_cube_type = get_cube(at(x, position.y, z))->type;
                        if(!block_face_textures[face_cube_type]) current_chunk->index_texture_data[index] = face_cube_type - 1;
                        else current_chunk->index_texture_data[index] = block_face_textures[face_cube_type][i];
                        if(offset_x == current_chunk->index_texture_offset_x + x_limit) { offset_x = current_chunk->index_texture_offset_x; offset_y++; }
                    }
                }
            }

            current_chunk->index_texture_offset_x += x_limit;
            if(current_chunk->index_texture_offset_x > CHUNK_INDEX_TEXTURE_SIZE)
            {
                current_chunk->index_texture_offset_x = 0;
                current_chunk->index_texture_offset_y = current_chunk->index_texture_offset_y + greatest_y_offset;
            }
        }
    }

    current_chunk->num_vertices += num_vertices_added;
    current_chunk->num_indices += num_indices_added;
    cube_to_add->num_indices = num_indices_added;
}

vec3 top_cube(float x, float z)
{
    float y = CHUNK_MAX_HEIGHT;
    CUBE* on_top = get_cube(at(x, y, z));
    while(on_top->type == EMPTY && y > 0)
        on_top = get_cube(at(x, y--, z));
    if(on_top->type != EMPTY)
        return at(x, y + 1, z);
    return at(-1, -1, -1);
}

// Generates the vertices and indices for a chunk based on its cube array.
void recalculate_chunk_model()
{
    vec3 block_position;
    int num_to_visit = 0;
    CUBE_TREE* to_visit[1024] = { NULL };
    unsigned int i = 1, range_min = 0, range_max = CHUNK_SIZE, level; 
    for(unsigned int i = 0; i < CHUNK_MAX_HEIGHT / CHUNK_SIZE; i++)
    {
        CUBE_TREE* cursor = current_chunk->cube_fill_state + i;
        if(cursor->full == CHUNK_FULL)
        {
            cube_faces(get_cube(cursor->min), cursor->min, v3(CHUNK_SIZE, CHUNK_SIZE, CHUNK_SIZE), CUBE_FACE_ALL & 0b11011111);
            // printf("%f %f %f\n", cursor->size.x, cursor->size.y, cursor->size.z);
        }
        else if(cursor->full == CHUNK_PARTIALLY_FULL)
        {
            num_to_visit = 0;
            while(cursor)
            {
                if(cursor->full == CHUNK_FULL)
                {
                    // fprintf(stderr, "min: %f %f %f\n", cursor->min.x, cursor->min.y, cursor->min.z);
                    // fprintf(stderr, "size: %f %f %f\n", cursor->size.x, cursor->size.y, cursor->size.z);
                    cube_faces(get_cube(cursor->min), cursor->min, cursor->size, CUBE_FACE_ALL);
                    // printf("%f %f %f\n", cursor->size.x, cursor->size.y, cursor->size.z);
                }
                else
                {
                    for(unsigned int j = 0; j < 8; j++)
                        if(cursor->children[j]) to_visit[num_to_visit++] = cursor->children[j];
                }

                // printf("g. min: %f %f %f\n", cursor->min.x, cursor->min.y, cursor->min.z);
                // printf("g. size: %f %f %f\n", cursor->size.x, cursor->size.y, cursor->size.z);
                // printf("%d\n", num_to_visit);
                if(!num_to_visit) break;
                cursor = to_visit[--num_to_visit];
            }
        }
    }

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

CUBE_TREE* parent_tree(CHUNK* chunk, vec3 position) { return &(chunk->cube_fill_state[(int)position.y / CHUNK_SIZE]); }

void place_block(BLOCK_TYPE type, vec3 position, bool recalculate_model)
{
    get_cube(position)->type = type;
    // if(parent_tree(current_chunk, position)->size.x == 0) printf("a\n");
    cube_tree_fill(parent_tree(current_chunk, position), position);
    if(recalculate_model) recalculate_chunk_model();
}

void remove_block(vec3 position, bool recalulate_model)
{
    get_cube(position)->type = EMPTY;
    cube_tree_empty(parent_tree(current_chunk, position), position);
    if(recalulate_model) recalculate_chunk_model();
}

// This is separated out because it's possible to create chunks in the existing chunk buffer
CHUNK* allocate_chunk_memory()
{
    CHUNK* to_return = calloc(1, sizeof(CHUNK));
    to_return->vertex_capacity = CHUNK_INITIAL_ALLOC_BLOCKS * 8;
    to_return->index_capacity = CHUNK_INITIAL_ALLOC_BLOCKS * 36;
    to_return->vertices = calloc(to_return->vertex_capacity, sizeof(BLOCK_VERTEX));
    to_return->indices = calloc(to_return->index_capacity, sizeof(unsigned int));
    to_return->cubes = calloc(CHUNK_SIZE * CHUNK_MAX_HEIGHT * CHUNK_SIZE, sizeof(CUBE));
    to_return->cube_child_buffer = calloc(CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE * (CHUNK_MAX_HEIGHT / CHUNK_SIZE), sizeof(CUBE_TREE));
    to_return->index_texture_data = calloc(CHUNK_INDEX_TEXTURE_SIZE * CHUNK_INDEX_TEXTURE_SIZE, sizeof(GLuint));
    return to_return;
}

CHUNK* make_chunk(vec3 position)
{
    CHUNK* to_return = allocate_chunk_memory();
    to_return->tranform = translate(position);
    current_chunk = to_return;

    for(unsigned int i = 0; i < CHUNK_MAX_HEIGHT / CHUNK_SIZE; i++)
    {
        current_chunk->cube_fill_state[i].min = v3(0, i * CHUNK_SIZE, 0);
        current_chunk->cube_fill_state[i].size = full_chunk;
    }

    vec3 cube_position;
    #ifdef DEBUG
    LARGE_INTEGER chunk_gen_start_time, chunk_gen_end_time;
    QueryPerformanceCounter(&chunk_gen_start_time);
    #endif
    for(unsigned int i = 0; i < CHUNK_SIZE; i++)
    {
        for(unsigned int j = 0; j < CHUNK_MAX_HEIGHT; j++)
        {
            for(int k = 0; k < CHUNK_SIZE; k++)
            {
                place_block(SOIL, at(i, j, -k), false);
            }
        }
    }

    // place_block(SOIL, at(0, 0, 0), false);
    
    // place_block(SOIL, at(0, 0, 0), false);
    // place_block(SOIL, at(1, 0, 0), false);
    // place_block(SOIL, at(0, 0, 0), false);
    // place_block(SOIL, at(1, 0, -1), false);
    // place_block(SOIL, at(0, 1, 0), false);
    // place_block(SOIL, at(1, 1, 0), false);
    // place_block(SOIL, at(0, 1, -1), false);
    // place_block(SOIL, at(1, 1, -1), false);

    // Make all the top blocks of the chunk grass blocks
    // for(int i = 0; i < CHUNK_SIZE; i++)
    // {
    //     for(int j = 0; j < CHUNK_SIZE; j++)
    //     {
    //         vec3 cube_position = top_cube(i, -j);
    //         if(!vec3_cmp(cube_position, at(-1, -1, -1))) get_cube(cube_position)->type = GRASS;
    //     }
    // }

    // current_chunk->cube_fill_state[7].full = CHUNK_PARTIALLY_FULL;
    // for(unsigned int i = 0; i < 8; i++) current_chunk->cube_fill_state[7].children[i] = NULL;
    // current_chunk->cube_fill_state[7].children[0] = calloc(1, sizeof(CUBE_TREE));
    // current_chunk->cube_fill_state[7].children[0]->full = CHUNK_FULL;
    // current_chunk->cube_fill_state[7].children[0]->min = v3(0, 223, 0);
    // current_chunk->cube_fill_state[7].children[0]->size = v3(16, 16, 16);

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
    glBufferData(GL_ARRAY_BUFFER, sizeof(BLOCK_VERTEX) * to_return->num_vertices, to_return->vertices, GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * to_return->num_indices, to_return->indices, GL_STATIC_DRAW);

    // Generate the texture index, then load the indices of all the vertices into it
    glGenTextures(1, &(current_chunk->index_texture));
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, current_chunk->index_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Note to self: check the texture formats (internal & supplied)
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, CHUNK_INDEX_TEXTURE_SIZE, CHUNK_INDEX_TEXTURE_SIZE, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, current_chunk->index_texture_data);

    configure_vertex_properties(VERTEX_POSITION | VERTEX_UV | VERTEX_UV2);
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
    free(to_free->cube_child_buffer);
    free(to_free->cubes);
    free(to_free->vertices);
    free(to_free->indices);
    free(to_free);
}

#endif