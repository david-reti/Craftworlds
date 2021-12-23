#ifndef BLOCKS_H
#define BLOCKS_H

#define BLOCK_IMAGE_WIDTH 16 // The width of each block image, in pixels
#define BLOCK_IMAGE_HEIGHT 16 // The height of each block image, in pixels
#define BLOCK_IMAGE_SIZE BLOCK_IMAGE_WIDTH * BLOCK_IMAGE_HEIGHT * 4 // The size of each block's image, in bytes (using 4, for rgba)
#define NUM_BLOCK_MIPMAP_LEVELS 3 // Number of mipmaps to generate for each block (depends on tile texture size)

#define NUM_BLOCK_TYPES       3 // The length of the list of the types of blocks available in the game
#define NUM_BLOCK_TEXTURES    3
typedef enum                { EMPTY,   SOIL,   GRASS,   GRAVEL,   SAND,   STONE } BLOCK_TYPE;
const char* block_names[] = {"EMPTY", "Soil", "Grass", "Gravel", "Sand", "Stone" };
const char* block_texture_names[] = { "Soil", "Grass", "SoilWithGrass", "Sand", "Stone" };

// Each number represents the index of the texture to use for that face i.e. the first number(index 0) - front face - texture 2 (SoilWithGrass)
const int grass_textures[] = { 2, 2, 2, 2, 1, 0 };
const int* block_face_textures[] = { NULL, NULL, grass_textures }; // A list of pointers to arrays of face textures for each side - NULL means it has the same texture on all sides

extern char _binary_build_block_images_start[];
extern char _binary_build_block_images_end[];
extern char _binary_build_block_images_size[];
#endif