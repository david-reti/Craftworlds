#include<stdio.h>
#include<string.h>

#include"util.h"
#include"blocks.h"

#define ONLY_INCLUDE_DEFINITIONS
#include"shaders.h"

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
        unsigned char* image_data = calloc(NUM_BLOCK_TYPES * BLOCK_IMAGE_SIZE, 1);
        for(unsigned int i = 1; i < NUM_BLOCK_TYPES; i++)
        {
            sprintf(asset_file_to_open, "assets/textures/blocks/%s.png", block_names[i]);
            if((asset_file = fopen(asset_file_to_open, "rb")) == NULL) exit_with_error("Could not open image for preprocessing", asset_file_to_open);
            unsigned char* data = stbi_load_from_file(asset_file, &image_width, &image_height, &image_components, 4);
            if(!data) exit_with_error("Could not parse texture (PNG) from file", asset_file_to_open);
            fclose(asset_file);

            // memcpy(image_data + (BLOCK_IMAGE_SIZE * (i - 1)), data, BLOCK_IMAGE_SIZE);
            memcpy(image_data, data, BLOCK_IMAGE_SIZE);
            stbi_image_free(data);
        }

        strcpy(asset_file_to_open, "build/block.images");
        if((asset_file = fopen(asset_file_to_open, "wb")) == NULL) exit_with_error("Could not open data file for writing preprocessing result", asset_file_to_open);
        fwrite(image_data, 1, NUM_BLOCK_TYPES * BLOCK_IMAGE_SIZE, asset_file);
        fclose(asset_file);
        free(image_data);
    }

    if(!strcmp(argv[1], "--shaders"))
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
            {
                unsigned long new_capacity = shader_data_capacity;
                while(new_capacity < shader_data_length + file_length) new_capacity *= 2;
                shader_data = realloc(shader_data, new_capacity);
            }

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
}