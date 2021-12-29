#include<stdio.h>
#include<string.h>

#include"util.h"
#include"blocks.h"

#define ONLY_INCLUDE_DEFINITIONS
#include"shaders.h"
#include"rendering.h"

unsigned char* stbi_load_from_file(FILE*, int*, int*, int*, int);
void stbi_image_free(void*);

int main(int argc, char** argv)
{
    FILE* asset_file;
    char asset_file_to_open[512] = { 0 };
    if(argc < 2) exit_with_error("Could not start preprocessor", "need at least another argument (type of processing to do): either --images or --shaders");

    if(!strcmp(argv[1], "--images"))
    {
        // Block Textures: load in all the block textures, then combine them into a single raw image, and write that out into the build directory
        int image_width, image_height, image_components;
        unsigned char* image_data = calloc((NUM_BLOCK_TEXTURES + 1) * BLOCK_IMAGE_SIZE, 1);
        unsigned char image_flip_buffer[BLOCK_IMAGE_WIDTH * 4] = { 0 };
        for(unsigned int i = 0; i < NUM_BLOCK_TEXTURES; i++)
        {
            sprintf(asset_file_to_open, "assets/textures/blocks/%s.png", block_texture_names[i]);
            if((asset_file = fopen(asset_file_to_open, "rb")) == NULL) exit_with_error("Could not open image for preprocessing", asset_file_to_open);
            unsigned char* data = stbi_load_from_file(asset_file, &image_width, &image_height, &image_components, 4);
            if(!data) exit_with_error("Could not parse texture (PNG) from file", asset_file_to_open);
            fclose(asset_file);

            memcpy(image_data + (BLOCK_IMAGE_SIZE * i), data, BLOCK_IMAGE_SIZE);

            // Flip the image on the Y axis - required because of the inverted coordinates in shader
            unsigned char* image_offset = image_data + (BLOCK_IMAGE_SIZE * i);
            for(unsigned int j = 0; j < image_height / 2; j++)
            {
                memcpy(image_flip_buffer, image_offset + (j * BLOCK_IMAGE_WIDTH * 4), BLOCK_IMAGE_WIDTH * 4);
                memcpy(image_offset + (j * BLOCK_IMAGE_WIDTH * 4), image_offset + ((BLOCK_IMAGE_HEIGHT - j - 1) * BLOCK_IMAGE_WIDTH * 4), BLOCK_IMAGE_WIDTH * 4);
                memcpy(image_offset + (BLOCK_IMAGE_HEIGHT - j - 1) * BLOCK_IMAGE_WIDTH * 4, image_flip_buffer, BLOCK_IMAGE_WIDTH * 4);
            }

            stbi_image_free(data);
        }

        strcpy(asset_file_to_open, "build/block.images");
        if((asset_file = fopen(asset_file_to_open, "wb")) == NULL) exit_with_error("Could not open data file for writing preprocessing result", asset_file_to_open);
        fwrite(image_data, 1, NUM_BLOCK_TEXTURES * BLOCK_IMAGE_SIZE, asset_file);
        fclose(asset_file);
        free(image_data);
    }
    else if(!strcmp(argv[1], "--shaders"))
    {
        // Shaders: load in all the shaders, then combine them into a single text file and write it to the build directory
        unsigned long shader_data_length = 0, shader_data_capacity = 2048, file_length, shader_offsets[NUM_SHADERS] = { 0 }, i;
        char* shader_data = calloc(shader_data_capacity, 1);
        for(i = 0; i < NUM_SHADERS; i++)
        {
            sprintf(asset_file_to_open, "assets/shaders/%s.glsl", shader_names[i]);
            if((asset_file = fopen(asset_file_to_open, "r")) == NULL) exit_with_error("Could not open shader for preprocessing", asset_file_to_open);
            fseek(asset_file, 0, SEEK_END);
            file_length = ftell(asset_file);
            fseek(asset_file, 0, SEEK_SET);
            
            if(shader_data_length + file_length > shader_data_capacity)
                shader_data = realloc(shader_data, shader_data_capacity *= 2);

            shader_offsets[i] = shader_data_length + sizeof(unsigned long) * NUM_SHADERS;
            fread(shader_data + shader_data_length, 1, file_length, asset_file);
            fclose(asset_file);
            shader_data_length += file_length;
        }

        // We add one extra length representing the end of the data, so we can do offsets[next] - offsets[current] to get shader length
        shader_offsets[i] = shader_data_length + sizeof(unsigned long) * NUM_SHADERS;
        strcpy(asset_file_to_open, "build/shaders.txt");
        if((asset_file = fopen(asset_file_to_open, "wb")) == NULL) exit_with_error("Could not open data file for writing preprocessing result", asset_file_to_open);
        fwrite(shader_offsets, sizeof(unsigned long), NUM_SHADERS, asset_file);
        fwrite(shader_data, 1, shader_data_length, asset_file);
        fclose(asset_file);
        free(shader_data);
    }
    else if(!strcmp(argv[1], "--models"))
    {
        unsigned long num_vertices = 0, num_indices = 0, vertex_capacity = 256, index_capacity = 256, file_length = 0, model_offsets[NUM_PREDEFINED_MODELS] = { 0 };
        char* model_file_contents, line[512] = { 0 };
        vec3* vertex_positions = calloc(vertex_capacity, sizeof(vec3));
        unsigned int* face_indices = calloc(index_capacity, sizeof(unsigned int));
        for(unsigned int i = 0; i < NUM_PREDEFINED_MODELS; i++)
        {
            sprintf(asset_file_to_open, "assets/models/%s.obj", model_names[i]);
            if((asset_file = fopen(asset_file_to_open, "r")) == NULL) exit_with_error("Could not open model for preprocessing", asset_file_to_open);
            fseek(asset_file, 0, SEEK_END);
            file_length = ftell(asset_file);
            fseek(asset_file, 0, SEEK_SET);

            model_offsets[i] = (NUM_PREDEFINED_MODELS * sizeof(unsigned long)) + (i * 2 * sizeof(unsigned long)) + (num_vertices * sizeof(vertex_positions)) + (num_indices * sizeof(unsigned int)); 

            model_file_contents = calloc(file_length + 1, 1);
            fread(model_file_contents, 1, file_length, asset_file);

            for(unsigned int i = 0; model_file_contents[i]; i++)
            {
                unsigned int j = i;
                memset(line, 0, 512);
                for(;model_file_contents[j] != '\n'; j++)
                    line[j - i] = model_file_contents[j];

                if(num_vertices + 1 > vertex_capacity)
                    vertex_positions = realloc(vertex_positions, sizeof(vec3) * (vertex_capacity *= 2));
                if(num_indices + 3 > index_capacity)
                    face_indices = realloc(face_indices, sizeof(unsigned int) * (index_capacity *= 2));

                if(line[0] == 'v') 
                { 
                    sscanf(line, "v %f %f %f", &(vertex_positions[num_vertices].x), &(vertex_positions[num_vertices].y), &(vertex_positions[num_vertices].z));
                    num_vertices++;
                }
                else if(line[0] == 'f')
                {
                    sscanf(line, "f %u %u %u", face_indices + num_indices, face_indices + num_indices + 1, face_indices + num_indices + 2);
                    for(unsigned long i = num_indices; i < num_indices + 3; i++) face_indices[i]--;
                    num_indices += 3;
                }

                i = j;
            }

            free(model_file_contents);
        }

        strcpy(asset_file_to_open, "build/predefined.models");
        if((asset_file = fopen(asset_file_to_open, "wb")) == NULL) exit_with_error("Could not open data file for writing preprocessing result", asset_file_to_open);
        fwrite(model_offsets, sizeof(unsigned long), NUM_PREDEFINED_MODELS, asset_file);
        fwrite(&num_vertices, sizeof(unsigned long), 1, asset_file);
        fwrite(&num_indices, sizeof(unsigned long), 1, asset_file);
        fwrite(vertex_positions, sizeof(vec3), num_vertices, asset_file);
        fwrite(face_indices, sizeof(unsigned int), num_indices, asset_file);
        fclose(asset_file);
        free(vertex_positions);
        free(face_indices);
    }
}