all: glad.o
	gcc main.c glad.o -ISDL2-2.0.16/x86_64-w64-mingw32/include -Iglad/include -LSDL2-2.0.16/x86_64-w64-mingw32/lib -lmingw32 -lopengl32 -lSDL2main -lSDL2 -O3 -DDEBUG -o Craftworlds  

glad.o:
	gcc glad/src/glad.c -Iglad/include -O3 -c

clean:
	rm Craftworlds