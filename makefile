preprocess=preprocess.exe
include_dirs=-ISDL2-2.0.16/x86_64-w64-mingw32/include -Iglad/include
library_dirs=-LSDL2-2.0.16/x86_64-w64-mingw32/lib
libraries_to_link=-lopengl32
sdl_static_windows_libraries=-lmingw32 -lSDL2main -lSDL2 -mwindows -Wl,--dynamicbase -Wl,--nxcompat -Wl,--high-entropy-va -lm -ldinput8 -ldxguid -ldxerr8 -luser32 -lgdi32 -lwinmm -limm32 -lole32 -loleaut32 -lshell32 -lsetupapi -lversion -luuid
optimisation_level=-Ofast
object_files=build/glad.o build/stb_image.o build/block.images.o build/shaders.o build/noise.o build/models.o
icon_resource=build/icon.res

all: $(object_files) $(icon_resource)
	gcc main.c -g $(object_files) $(icon_resource) $(include_dirs) $(library_dirs) -static $(libraries_to_link) $(sdl_static_windows_libraries) $(optimisation_level) -DDEBUG -o build/Craftworlds

build/glad.o:
	mkdir -p build
	gcc glad/src/glad.c -Iglad/include $(optimisation_level) -c -o build/glad.o

build/stb_image.o:
	mkdir -p build
	gcc stb_image.c $(optimisation_level) -c -o build/stb_image.o

build/noise.o:
	mkdir -p build
	gcc opensimplex.c -Ofast -c -o build/noise.o

build/block.images.o: $(preprocess) $(wildcard assets/textures/blocks/*.png)
	$(preprocess) --images
	ld -r -b binary -o build/block.images.o build/block.images
	rm -f build/block.images

build/shaders.o: $(preprocess) $(wildcard assets/shaders/*.glsl)
	$(preprocess) --shaders
	ld -r -b binary -o build/shaders.o build/shaders.txt
	rm -f build/shaders.txt

build/models.o: $(preprocess) $(wildcard assets/models/*.obj)
	$(preprocess) --models
	ld -r -b binary -o build/models.o build/predefined.models
	rm -f build/predefined.models

$(preprocess): blocks.h build/stb_image.o preprocess.c
	gcc preprocess.c build/stb_image.o $(include_dirs) $(optimisation_level) -o preprocess

$(icon_resource): assets/textures/misc/program_icon.ico assets/misc/resources.rc
	windres assets/misc/resources.rc -O coff -o $(icon_resource)

run: all
	build/Craftworlds.exe 2>craftworlds_errors.log

clean:
	rm -rf build