# Craftworlds

This is the repository for my block-building game, which I programmed over my winter break in 2022, and is very similar to Minecraft. The target platform is Windows with MinGW. Optimizing the terrain generation process was interesting - I decided to use octrees because this way the generation algorithm can run in logarithmic time instead of O(n^3) and adjacent cubes can be combined automatically. The challenge is that players like to build flat surfaces and the algorithm doesn't work well with those. Another innovation is compiling the textures into the program, and using texture arrays with mipmapping with results in a smooth look even in the distance. 

![screenshot](/full-craftworlds.jpg)