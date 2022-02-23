#ifndef WORLD_H
#define WORLD_H
#include<stdlib.h>
#include<SDL2/SDL.h>
#include<glad/glad.h>

#include"util.h"
#include"noise.h"
#include"math3d.h"
#include"blocks.h"
#include"rendering.h"

#define CHUNK_SIZE 32 // The maximum width and depth of chunks, in number of blocks
#define CHUNK_MAX_HEIGHT 256 // The maximum height of chunks, in number of blocks
#define CHUNK_INITIAL_ALLOC_BLOCKS 4096 // The number of blocks to allocate vertex & index space for when a chunk is created. Increasing this reduces the number of allocations, but uses more memory
#define CHUNK_INDEX_TEXTURE_SIZE 2048 // The size of the texture used to store the indices which specify the texture to use for each cube

#define BASE_LEVEL CHUNK_SIZE * 3 // This is the height at which water will be generated, and which any terrain will be added on, meaning that all chunks under this will be completely filled in
#define WATER_LEVEL 13 // How far up from the base level water should reach

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
    MODEL* model, *transparency_model; // A separate temporary model is used for transparent object, which will be added on to the end of the terrain model so that transparency works properly
    unsigned int index_texture, index_texture_offset_x, index_texture_offset_y, index_texture_highest_y_offset;
    unsigned long cube_child_buffer_size;
    GLuint* index_texture_data;
    CUBE* cubes;
    CUBE_TREE cube_fill_state[8], transparency_fill_state[8], *cube_child_buffer, *trees_to_update[256];
} CHUNK;

CHUNK **chunks;
CUBE empty_cube = { 0 };
const vec3 full_chunk = { CHUNK_SIZE, CHUNK_SIZE, CHUNK_SIZE };
unsigned int chunk_buffer_size;

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
        if(cursor->full == CHUNK_FULL || cursor->full == CHUNK_EMPTY) return cursor;
        origin = cursor->min;

        cursor = cursor->children[origin.y < position.y * 4 + origin.z > position.z * 2 + origin.x < position.x];
        if(cursor == NULL) return NULL;
    }
}

void cube_tree_fill(CHUNK* chunk, CUBE_TREE* tree, vec3 position)
{
    CUBE_TREE* cursor = tree;
    vec3 origin, size = full_chunk;
    // position.y -= tree->min.y;
    // printf("%f %f %f\n", size.x, size.y, size.z);
    unsigned int num_to_update = 0, idx;
    while(!vec3_cmp(origin, position) || size.x > 1)
    {
        if(cursor->full == CHUNK_FULL) break;
        size = vec3_divide_scalar(size, 2);
        // vec3 min_point = v3(cursor->min.x, cursor->min.y - tree->min.y, cursor->min.z);
        origin = vec3_add_vec3(vec3_invert_if_negative_z(cursor->min), size);
        idx = (origin.y <= position.y) * 4 + (-origin.z >= position.z) * 2 + (origin.x <= position.x);
        if(!cursor->children[idx])
        {
            // fprintf(stderr, "%u\n", idx);
            cursor->children[idx] = chunk->cube_child_buffer + chunk->cube_child_buffer_size++;
            cursor->children[idx]->min = vec3_add_vec3(cursor->min, vec3_scale(size, tree_child_transformations[idx]));
            cursor->children[idx]->size = size;
            // printf("min: %f %f %f\n", cursor->children[idx]->min.x, cursor->children[idx]->min.y, cursor->children[idx]->min.z);
            // printf("size: %f %f %f\n", cursor->children[idx]->size.x, cursor->children[idx]->size.y, cursor->children[idx]->size.z);
            if(vec3_cmp(cursor->children[idx]->min, position) && cursor->children[idx]->size.x == 1) cursor->children[idx]->full = CHUNK_FULL;
        }
        
        chunk->trees_to_update[num_to_update++] = cursor;
        cursor = cursor->children[idx];
    }

    // printf("min: %f %f %f\n", cursor->min.x, cursor->min.y, cursor->min.z);
    // printf("size: %f %f %f\n", cursor->size.x, cursor->size.y, cursor->size.z);

    for(int i = num_to_update - 1; i >= 0; i--)
    {
        if(chunk->trees_to_update[i]->full == CHUNK_PARTIALLY_FULL)
        {
            chunk->trees_to_update[i]->full = CHUNK_FULL;
            for(unsigned char j = 0; j < 8; j++)
            {
                if(!chunk->trees_to_update[i]->children[j]) chunk->trees_to_update[i]->full = CHUNK_PARTIALLY_FULL;
                else if(chunk->trees_to_update[i]->children[j]->full != CHUNK_FULL) chunk->trees_to_update[i]->full = CHUNK_PARTIALLY_FULL;
            }
        }
        else if(chunk->trees_to_update[i]->full == CHUNK_EMPTY) chunk->trees_to_update[i]->full = CHUNK_PARTIALLY_FULL;
    }
}

void cube_tree_empty(CHUNK* chunk, CUBE_TREE* tree, vec3 position)
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
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, BLOCK_IMAGE_WIDTH, BLOCK_IMAGE_HEIGHT, NUM_BLOCK_TEXTURES, 0, GL_RGBA, GL_UNSIGNED_BYTE, _binary_build_block_images_start);

    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
}

BLOCK_VERTEX cube_vertex(CHUNK* chunk, unsigned char position_index, unsigned char face_index, vec3 place_at, vec3 size) 
{ 
    BLOCK_VERTEX to_return;
    to_return.position = vec3_add_vec3(vec3_scale(cube_vertex_positions[position_index], size), place_at);

    vec2 face_uv_scale;
    CUBE_FACES face = 1 << face_index;
    if(face == CUBE_FACE_FRONT || face == CUBE_FACE_BACK) face_uv_scale = v2(size.x, size.y);
    else if(face == CUBE_FACE_LEFT || face == CUBE_FACE_RIGHT) face_uv_scale = v2(size.z, size.y);
    else if (face == CUBE_FACE_TOP || face == CUBE_FACE_BOTTOM) face_uv_scale = v2(size.x, size.z);

    to_return.uv = vec2_scale_vec2(cube_texcoords[face_texcoords[face_index][position_index]], face_uv_scale);
    to_return.uv2 = v2(chunk->index_texture_offset_x, chunk->index_texture_offset_y);
    return to_return;
}

CUBE* get_cube(CHUNK* parent_chunk, vec3 point)
{
    long long x = (long long)point.x, y = (long long)point.y, z = (long long)point.z;
    if(x < 0 || y < 0 || x >= CHUNK_SIZE || y >= CHUNK_MAX_HEIGHT || z > 0 || abs(z) >= CHUNK_SIZE)
        return &empty_cube;
    return parent_chunk->cubes + (x + (abs(z) * CHUNK_SIZE) + (y * CHUNK_SIZE * CHUNK_SIZE));
}

void expand_chunk_model(MODEL* to_expand, int capacity_cutoff)
{
    if(to_expand->num_vertices + capacity_cutoff > to_expand->vertex_capacity || to_expand->num_indices + capacity_cutoff > to_expand->index_capacity)
    {
        // Allocate more space for vertices and indices. Since there are more indices than vertices per cube, the number of indices grows faster as well
        BLOCK_VERTEX* vertices; unsigned int* indices;
        if((vertices = realloc(to_expand->vertices, to_expand->vertex_capacity * 2 * sizeof(BLOCK_VERTEX))) == NULL)
            exit_with_error("Memory allocation error", "realloc() failed during chunk generation - likely run out of memory");
        if((indices = realloc(to_expand->indices, to_expand->index_capacity * 4 * sizeof(unsigned int))) == NULL)
            exit_with_error("Memory allocation error", "realloc() failed during chunk generation - likely ran out of memory");
        to_expand->vertex_capacity *= 2;
        to_expand->index_capacity *= 4;
        to_expand->vertices = vertices; to_expand->indices = indices;
    }
}

void cube_faces(CHUNK* parent_chunk, MODEL* to_fill, vec3 position, vec3 size, CUBE_FACES faces_to_add)
{
    expand_chunk_model(to_fill, 32);
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
            index_loc = to_fill->indices + to_fill->num_indices + num_indices_added;
            memcpy(index_loc, faces[i], sizeof(unsigned int) * 6);
            memset(indices_added, 0, 8 * sizeof(unsigned int));

            // Copy the corresponding vertices for all the indices
            for(unsigned char j = 0; j < 6; j++)
            {
                if(!indices_added[index_loc[j]])
                {
                    ((BLOCK_VERTEX*)to_fill->vertices)[to_fill->num_vertices + num_vertices_added++] = cube_vertex(parent_chunk, index_loc[j], i, position, size);
                    indices_added[index_loc[j]] = to_fill->num_vertices + num_vertices_added - 1;
                }
                index_loc[j] = indices_added[index_loc[j]];
            }
            num_indices_added += 6;

            // Updates the index texture for the face by looping through each block on the face, and updating the texture coordinate with the block texture ID
            offset_x = parent_chunk->index_texture_offset_x;
            offset_y = parent_chunk->index_texture_offset_y;
            x_limit = size.x;
            
            // I might move this into its own function in the future, but I think it would be tricky because of the number of variables involved
            // TODO: Clean this up
            if(face_to_add == CUBE_FACE_FRONT)
            {
                if(size.y > parent_chunk->index_texture_highest_y_offset) parent_chunk->index_texture_highest_y_offset = size.y;
                for(float y = position.y; y < position.y + size.y; y++) 
                {
                    for(float x = position.x; x < position.x + size.x; x++)
                    {
                        unsigned long index = (offset_y * CHUNK_INDEX_TEXTURE_SIZE) + offset_x++;
                        face_cube_type = get_cube(parent_chunk, at(x, y, position.z))->type;
                        if(!block_face_textures[face_cube_type]) parent_chunk->index_texture_data[index] = face_cube_type - 1;
                        else parent_chunk->index_texture_data[index] = block_face_textures[face_cube_type][i];
                        if(offset_x == parent_chunk->index_texture_offset_x + x_limit) { offset_x = parent_chunk->index_texture_offset_x; offset_y++; }
                    }
                }
            }
            else if(face_to_add == CUBE_FACE_BACK)
            {
                if(size.y > parent_chunk->index_texture_highest_y_offset) parent_chunk->index_texture_highest_y_offset = size.y;
                for(float y = position.y; y < position.y + size.y; y++) 
                {
                    for(float x = position.x + size.x - 1; x > position.x - 1; x--)
                    {
                        unsigned long index = (offset_y * CHUNK_INDEX_TEXTURE_SIZE) + offset_x++;
                        face_cube_type = get_cube(parent_chunk, at(x, y, position.z - size.z + 1))->type;
                        if(!block_face_textures[face_cube_type]) parent_chunk->index_texture_data[index] = face_cube_type - 1;
                        else parent_chunk->index_texture_data[index] = block_face_textures[face_cube_type][i];
                        if(offset_x == parent_chunk->index_texture_offset_x + x_limit) { offset_x = parent_chunk->index_texture_offset_x; offset_y++; }
                    }
                }
            }
            else if(face_to_add == CUBE_FACE_LEFT)
            {
                x_limit = size.z;
                if(size.y > parent_chunk->index_texture_highest_y_offset) parent_chunk->index_texture_highest_y_offset = size.y;
                for(float y = position.y; y < position.y + size.y; y++) 
                {
                    for(float z = position.z - size.z + 1; z < position.z + 1; z++)
                    {
                        unsigned long index = (offset_y * CHUNK_INDEX_TEXTURE_SIZE) + offset_x++;
                        face_cube_type = get_cube(parent_chunk, at(position.x, y, z))->type;
                        if(!block_face_textures[face_cube_type]) parent_chunk->index_texture_data[index] = face_cube_type - 1;
                        else parent_chunk->index_texture_data[index] = block_face_textures[face_cube_type][i];
                        if(offset_x == parent_chunk->index_texture_offset_x + x_limit) { offset_x = parent_chunk->index_texture_offset_x; offset_y++; }
                    }
                }
            }
            else if(face_to_add == CUBE_FACE_RIGHT)
            {
                x_limit = size.z;
                if(size.y > parent_chunk->index_texture_highest_y_offset) parent_chunk->index_texture_highest_y_offset = size.y;
                for(float y = position.y; y < position.y + size.y; y++) 
                {
                    for(float z = position.z; z > position.z - size.z; z--)
                    {
                        unsigned long index = (offset_y * CHUNK_INDEX_TEXTURE_SIZE) + offset_x++;
                        face_cube_type = get_cube(parent_chunk, at(position.x + size.x - 1, y, z))->type;
                        if(!block_face_textures[face_cube_type]) parent_chunk->index_texture_data[index] = face_cube_type - 1;
                        else parent_chunk->index_texture_data[index] = block_face_textures[face_cube_type][i];
                        if(offset_x == parent_chunk->index_texture_offset_x + x_limit) { offset_x = parent_chunk->index_texture_offset_x; offset_y++; }
                    }
                }
            }
            else if(face_to_add == CUBE_FACE_TOP)
            {
                if(size.z > parent_chunk->index_texture_highest_y_offset) parent_chunk->index_texture_highest_y_offset = size.z;
                for(float z = position.z; z > position.z - size.z; z--) 
                {
                    for(float x = position.x; x < position.x + size.x; x++)
                    {
                        unsigned long index = (offset_y * CHUNK_INDEX_TEXTURE_SIZE) + offset_x++;
                        face_cube_type = get_cube(parent_chunk, at(x, position.y + size.y - 1, z))->type;
                        if(!block_face_textures[face_cube_type]) parent_chunk->index_texture_data[index] = face_cube_type - 1;
                        else parent_chunk->index_texture_data[index] = block_face_textures[face_cube_type][i];
                        if(offset_x == parent_chunk->index_texture_offset_x + x_limit) { offset_x = parent_chunk->index_texture_offset_x; offset_y++; }
                    }
                }
            }
            else if(face_to_add == CUBE_FACE_BOTTOM)
            {
                if(size.z > parent_chunk->index_texture_highest_y_offset) parent_chunk->index_texture_highest_y_offset = size.z;
                for(float z = position.z - size.z + 1; z < position.z + 1; z++) 
                {
                    for(float x = position.x; x < position.x + size.x; x++)
                    {
                        unsigned long index = (offset_y * CHUNK_INDEX_TEXTURE_SIZE) + offset_x++;
                        face_cube_type = get_cube(parent_chunk, at(x, position.y, z))->type;
                        if(!block_face_textures[face_cube_type]) parent_chunk->index_texture_data[index] = face_cube_type - 1;
                        else parent_chunk->index_texture_data[index] = block_face_textures[face_cube_type][i];
                        if(offset_x == parent_chunk->index_texture_offset_x + x_limit) { offset_x = parent_chunk->index_texture_offset_x; offset_y++; }
                    }
                }
            }

            parent_chunk->index_texture_offset_x += x_limit + 1;
            if(parent_chunk->index_texture_offset_x >= CHUNK_INDEX_TEXTURE_SIZE)
            {
                parent_chunk->index_texture_offset_x = 0;
                parent_chunk->index_texture_offset_y = parent_chunk->index_texture_offset_y + parent_chunk->index_texture_highest_y_offset + 1;
                parent_chunk->index_texture_highest_y_offset = 0;
            }
        }
    }

    to_fill->num_vertices += num_vertices_added;
    to_fill->num_indices += num_indices_added;
}

vec3 top_cube(CHUNK* chunk, float x, float z)
{
    float y = CHUNK_MAX_HEIGHT;
    CUBE* on_top = get_cube(chunk, at(x, y, z));
    while(on_top->type == EMPTY && y > 0)
        on_top = get_cube(chunk, at(x, y--, z));
    if(on_top->type != EMPTY)
        return at(x, y + 1, z);
    return at(-1, -1, -1);
}

vec3 raycast_block(CHUNK* chunk, vec3 origin, vec3 ray)
{
    return v3(-1, -1, -1);
}

// Generates the vertices and indices for a chunk based on its cube array.
void recalculate_chunk_model(CHUNK* to_recalculate, bool transparent)
{
    vec3 block_position;
    int num_to_visit = 0;
    CUBE_TREE* to_visit[1024] = { NULL };
    unsigned int i = 1, range_min = 0, range_max = CHUNK_SIZE, level;
    MODEL* dst_model = (transparent ? to_recalculate->transparency_model : to_recalculate->model);
    CUBE_TREE* origin = (transparent ? to_recalculate->transparency_fill_state : to_recalculate->cube_fill_state);
    for(unsigned int i = 0; i < CHUNK_MAX_HEIGHT / CHUNK_SIZE; i++)
    {
        CUBE_TREE* cursor = origin + i;
        if(cursor->full == CHUNK_FULL)
        {
            cube_faces(to_recalculate, dst_model, cursor->min, v3(CHUNK_SIZE, CHUNK_SIZE, CHUNK_SIZE), CUBE_FACE_ALL & 0b11011111);
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
                    cube_faces(to_recalculate, dst_model, cursor->min, cursor->size, CUBE_FACE_ALL);
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
}

CUBE_TREE* parent_tree(CHUNK* chunk, vec3 position, bool for_transparency) 
{
    if(for_transparency)
        return &(chunk->transparency_fill_state[(int)position.y / CHUNK_SIZE]);
    return &(chunk->cube_fill_state[(int)position.y / CHUNK_SIZE]); 
}

void place_block(CHUNK* chunk, BLOCK_TYPE type, vec3 position, bool recalculate_model)
{
    get_cube(chunk, position)->type = type;
    if(type == WATER || type == LEAVES)
        cube_tree_fill(chunk, parent_tree(chunk, position, true), position);
    else
        cube_tree_fill(chunk, parent_tree(chunk, position, false), position);
    // if(recalculate_model) recalculate_chunk_model(chunk, dst_tree);
}

void remove_block(CHUNK* chunk, vec3 position, bool recalulate_model)
{
    // get_cube(chunk, position)->type = EMPTY;
    // cube_tree_empty(chunk, parent_tree(chunk, position, false), position);
    // if(recalulate_model) recalculate_chunk_model(chunk);
}

// This is separated out because it's possible to create chunks in the existing chunk buffer
CHUNK* allocate_chunk_memory()
{
    CHUNK* to_return = calloc(1, sizeof(CHUNK));
    to_return->model = make_model(VERTEX_POSITION | VERTEX_UV | VERTEX_UV2, CHUNK_INITIAL_ALLOC_BLOCKS * 8, CHUNK_INITIAL_ALLOC_BLOCKS * 16, NULL, NULL);
    to_return->transparency_model = make_model(VERTEX_POSITION | VERTEX_UV | VERTEX_UV2, CHUNK_INITIAL_ALLOC_BLOCKS * 8, CHUNK_INITIAL_ALLOC_BLOCKS * 16, NULL, NULL);
    to_return->cubes = calloc(CHUNK_SIZE * CHUNK_MAX_HEIGHT * CHUNK_SIZE, sizeof(CUBE));
    to_return->cube_child_buffer = calloc(CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE * (CHUNK_MAX_HEIGHT / CHUNK_SIZE) * 2, sizeof(CUBE_TREE));
    to_return->index_texture_data = calloc(CHUNK_INDEX_TEXTURE_SIZE * CHUNK_INDEX_TEXTURE_SIZE, sizeof(GLuint));
    return to_return;
}

void initialize_chunk_buffer(unsigned int buffer_size)
{
    chunks = calloc(buffer_size, sizeof(CHUNK*));
    for(unsigned int i = 0; i < buffer_size; i++) chunks[i] = allocate_chunk_memory();
}

// Optionally, the chunk can be generated into an already existing allocated chunk object - place_into. If this is null, memory will be allocated anew
CHUNK* make_chunk(vec3 position, CHUNK* place_into)
{
    CHUNK* to_return = place_into;
    if(!to_return) to_return = allocate_chunk_memory();

    to_return->position = position;
    to_return->tranform = translate(to_return->position);

    for(unsigned int i = 0; i < CHUNK_MAX_HEIGHT / CHUNK_SIZE; i++)
    {
        to_return->cube_fill_state[i].min = v3(0, i * CHUNK_SIZE, 0);
        to_return->cube_fill_state[i].size = full_chunk;
        to_return->transparency_fill_state[i].min = v3(0, i * CHUNK_SIZE, 0);
        to_return->transparency_fill_state[i].size = full_chunk;
    }

    bool water_block;
    unsigned char noise_val[4] = { 0 }, terrain_height;

    vec3 cube_position;
    #ifdef DEBUG
    LARGE_INTEGER chunk_gen_start_time, chunk_gen_end_time;
    QueryPerformanceCounter(&chunk_gen_start_time);
    #endif
    // Generate terrain and water blocks based on terrain height
    for(unsigned int i = 0; i < CHUNK_SIZE; i++)
    {
        for(int j = 0; j < CHUNK_SIZE; j++)
        {
            water_block = false;
            unsigned int k = 0;
            for(;k < CHUNK_MAX_HEIGHT; k++)
            {
                simplex(noise_val, position.x + i / 4, (-position.z) + (j / 4));
                terrain_height = BASE_LEVEL + ((float)noise_val[0] / 255) * 40;
                if(k < BASE_LEVEL + ((terrain_height - BASE_LEVEL) / 4)) place_block(to_return, STONE, at(i, k, -j), false); 
                else if(k <= terrain_height) { place_block(to_return, SOIL, at(i, k, -j), false); }
                else if(k > terrain_height && k <= BASE_LEVEL + WATER_LEVEL) { water_block = true; place_block(to_return, WATER, at(i, k, -j), false); }
                else if(k > terrain_height && !water_block) { place_block(to_return, GRASS, at(i, k, -j), false); break; }
            }
        }
    }

    // Generate trees
    for(unsigned int i = 0; i < 10; i++)
    {
        int x = (rand() / (float)RAND_MAX) * CHUNK_SIZE;
        int z = (rand() / (float)RAND_MAX) * -CHUNK_SIZE;
        float probability = 1.0;
        vec3 top_cube_position = top_cube(to_return, x, z), leaf_position;
        CUBE* cube = get_cube(to_return, top_cube_position), *leaf_cube;
        if(cube->type == GRASS || cube->type == SOIL)
        {
            cube->type = SOIL;
            for(unsigned int i = 0; i < ((rand() / (float)RAND_MAX * 4) + 3) - 1; i++)
            {
                top_cube_position = vec3_add_vec3(top_cube_position, v3(0.0, 1.0, 0.0));
                place_block(to_return, WOOD, top_cube_position, false);
            }

            top_cube_position = vec3_add_vec3(top_cube_position, v3(0.0, 1.0, 0.0));
            place_block(to_return, WOOD_TOP, top_cube_position, false);

            for(unsigned int i = 0; i < 42 * 3; i++)
            { 
                float x_pos = (float)(i % 7) - 3, y_pos = (float)(i / 42) - 1, z_pos = -(float)((i  % 42 ) / 7) + 3;
                float distance = sqrtf(x_pos * x_pos + y_pos * y_pos + z_pos * z_pos); 
                if((float)rand() / RAND_MAX < probability * fmaxf(0.0, (3.0 - distance)))
                {
                    leaf_position = vec3_add_vec3(top_cube_position, v3(x_pos, y_pos, z_pos));
                    if(leaf_position.x < 0 || leaf_position.y < 0 || leaf_position.z > 0 || leaf_position.x >= CHUNK_SIZE || leaf_position.y >= CHUNK_MAX_HEIGHT || leaf_position.z <= -CHUNK_SIZE) continue;
                    leaf_cube = get_cube(to_return, leaf_position);
                    if(leaf_cube->type == EMPTY)
                        place_block(to_return, LEAVES, leaf_position, false);
                }
            }
        } 
    }

    recalculate_chunk_model(to_return, false);
    recalculate_chunk_model(to_return, true);
    #ifdef DEBUG
    QueryPerformanceCounter(&chunk_gen_end_time);
    double genTime = ((double)(chunk_gen_end_time.QuadPart - chunk_gen_start_time.QuadPart) / frequency.QuadPart);
    printf("Generated chunk at (%.2f, %.2f): took %lf seconds, %lu vertices, %lu indices\n", position.x, position.z, genTime, to_return->model->num_vertices, to_return->model->num_indices);
    #endif
    return to_return;
}

void finalise_chunk(CHUNK* to_finalise)
{
    // Generate the texture index, then load the indices of all the vertices into it
    glGenTextures(1, &(to_finalise->index_texture));
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, to_finalise->index_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, CHUNK_INDEX_TEXTURE_SIZE, CHUNK_INDEX_TEXTURE_SIZE, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, to_finalise->index_texture_data);
    
    finalise_model(to_finalise->model);
    finalise_model(to_finalise->transparency_model);
}

void render_chunk(CHUNK* to_render)
{
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, to_render->index_texture);
    set_shader_value(MODEL_MATRIX, &(to_render->tranform));
    glEnable(GL_CULL_FACE);
    render_model(to_render->model);
    glDisable(GL_CULL_FACE);
    render_model(to_render->transparency_model);
}

void unload_chunk(CHUNK* to_free)
{
    unload_model(to_free->model);
    unload_model(to_free->transparency_model);
    free(to_free->index_texture_data);
    free(to_free->cube_child_buffer);
    free(to_free->cubes);
    free(to_free);
}

void unload_chunk_buffer()
{
    for(unsigned int i = 0; i < chunk_buffer_size; i++) unload_chunk(chunks[i]);
    free(chunks);
}

#endif