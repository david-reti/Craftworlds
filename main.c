#include<stdio.h>
#include<stdbool.h>
#include<SDL2/SDL.h>
#include<glad/glad.h>
#include<SDL2/SDL_opengl.h>

#include"math3d.h"

// Shader Value Layout - Ensure this is the same in the shaders and that update_shader_methods reflects it
// 1 - Model Matrix (tranformation - local -> world space)
// 2 - View Matrix (world -> camera space)
// 3 - Projection Matrix (camera -> clip space)
typedef enum { DEFAULT_VERTEX, DEFAULT_FRAGMENT } SHADER_TYPE;
typedef enum { MODEL_MATRIX = 1, VIEW_MATRIX = 2, PROJECTION_MATRIX = 3 } SHADER_VALUE;
char* shader_paths[] = {"assets/shaders/default-vertex.glsl", "assets/shaders/default-fragment.glsl"};
unsigned int shaders[2] = { 0 };
unsigned int current_shader_program = 0;

// Update methods for each data type used in the shaders. Shader_update_methods is an array containing the method to use for each value.
void update_shader_matrix(unsigned int shader_program, SHADER_VALUE value, void* to_set) { glUniformMatrix4fv(value, 1, GL_FALSE, (const GLfloat*)((mat4*)to_set)->values); };
void (*shader_update_methods[])(unsigned int, SHADER_VALUE, void*) = { NULL, update_shader_matrix, update_shader_matrix, update_shader_matrix };

bool failed(int result) { return result < 0; }

void exit_with_error(const char* message, const char* detail)
{
    fprintf(stderr, "%s: %s\n", message, detail);
    exit(1);
}

typedef struct SETTINGS
{
    float fov;
    unsigned int window_width, window_height;
} SETTINGS;

unsigned int load_shader(unsigned long type, const char* path) // Internal
{
    size_t source_length = 0;
    FILE* shader_source_file = fopen(path, "r");
    if(!shader_source_file)
        exit_with_error("Could not open shader source file for compiling", path);

    // Load the source from the shader file
    fseek(shader_source_file, 0, SEEK_END);
    source_length = ftell(shader_source_file);
    fseek(shader_source_file, 0, SEEK_SET);

    char* source = calloc(source_length + 1, 1);
    fread(source, 1, source_length, shader_source_file);
    fclose(shader_source_file);

    // Create & compile the shader
    unsigned int to_return = glCreateShader(type);
    glShaderSource(to_return, 1, (const char**)&source, NULL);
    glCompileShader(to_return);
    free(source);

    // Check for errors, and print them if there are any
    int success;
    glGetShaderiv(to_return, GL_COMPILE_STATUS, &success);
    if(!success)
    {
        char error_info[1024];
        glGetShaderInfoLog(to_return, 1024, NULL, error_info);
        exit_with_error("Could not compile shader", error_info);
    }

    return to_return;
}

void use_shader_program(unsigned int shader_program)
{
    glUseProgram(shader_program);
    current_shader_program = shader_program;
}

unsigned int link_program(unsigned int vertex_shader, unsigned int fragment_shader) // Internal
{
    // Create program & link shaders
    unsigned int to_return = glCreateProgram();
    glAttachShader(to_return, vertex_shader);
    glAttachShader(to_return, fragment_shader);
    glLinkProgram(to_return);

    // Check the program status
    int success;
    glGetProgramiv(to_return, GL_LINK_STATUS, &success);
    if(!success)
    {
        char error_info[1024];
        glGetProgramInfoLog(to_return, 1024, NULL, error_info);
        exit_with_error("Could not link shader program", error_info);
    }

    use_shader_program(to_return);
}

unsigned int shader_program(SHADER_TYPE vertex, SHADER_TYPE fragment)
{
    if(!shaders[vertex])
        shaders[vertex] = load_shader(GL_VERTEX_SHADER, shader_paths[vertex]);
    if(!shaders[fragment])
        shaders[fragment] = load_shader(GL_FRAGMENT_SHADER, shader_paths[fragment]);
    return link_program(shaders[vertex], shaders[fragment]);
}

void shader_value(SHADER_VALUE value, void* to_set) { shader_update_methods[value](current_shader_program, value, to_set); }

void resize_renderer(SETTINGS* settings, mat4* camera_projection)
{
    glViewport(0, 0, settings->window_width, settings->window_height);
    *camera_projection = perspective_projection((float)settings->window_width / settings->window_height, settings->fov, 0.1, 1000);
    if(current_shader_program) shader_value(PROJECTION_MATRIX, camera_projection);
}

int main(int argc, char** argv)
{
    /// Initialize SDL
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_GLContext* context;
    SETTINGS settings = {.window_width = 1280, .window_height = 720, .fov = 45.0 };
    bool key_pressed[256] = { 0 };

    if(failed(SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO)))
        exit_with_error("Could not initialize SDL", SDL_GetError());

    if(!(window = SDL_CreateWindow("Craftworlds", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, settings.window_width, settings.window_height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL)))
        exit_with_error("Could not open game window", SDL_GetError());

    /// Initialize OpenGL
    // Use OpenGL 4.3 core
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    if(!(context = SDL_GL_CreateContext(window)))
        exit_with_error("Could not initialize opengl 4.3", SDL_GetError());

    if(!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
        exit_with_error("Could not load required opengl extentions", "using glad - gl 4.3");

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    /// Setting up the rendering - temporary
    float cube_vertices[] = {
        // Front
        -0.5f, -0.5f, -0.5f,
        -0.5f, 0.5f, -0.5f, 
        0.5f, -0.5f, -0.5f,
        0.5f, 0.5f, -0.5f,
        // Back
        -0.5f, -0.5f, 0.5f,
        -0.5f, 0.5f, 0.5f, 
        0.5f, -0.5f, 0.5f,
        0.5f, 0.5f, 0.5f,
    };

    unsigned int cube_indices[] = { 0, 1, 2, 2, 1, 3 };

    // Load the vertex & fragment shaders
    unsigned int default_program = shader_program(DEFAULT_VERTEX, DEFAULT_FRAGMENT);
    glUseProgram(default_program);

    /// Set up vertex attributes - this state will be stored in vertex array object
    unsigned int default_vao;
    glGenVertexArrays(1, &default_vao);
    glBindVertexArray(default_vao);

    // Vertex buffer - has type GL_ARRAY_BUFFER
    unsigned int vb;
    glGenBuffers(1, &vb);
    glBindBuffer(GL_ARRAY_BUFFER, vb);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);

    // Index buffer - works in a similar way to the vertex buffer
    unsigned int eb;
    glGenBuffers(1, &eb);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eb);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_indices), cube_indices, GL_STATIC_DRAW);

    // Attribute to configure, num elems, type, whether to normalize, stride (between attrib in next vertex), offset(where it begins)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GL_FLOAT), 0);
    glEnableVertexAttribArray(0);

    // Create and configure the camera
    mat4 camera_projection;
    resize_renderer(&settings, &camera_projection);
    mat4 tranform = m4();
    shader_value(MODEL_MATRIX, &tranform);

    vec3 camera_position = v3(0.0f, 0.0f, 3.0f);
    vec3 camera_forward = v3(0.0f, 0.0f, -1.0f);
    mat4 camera_view = m4();
    float camera_speed = 0.0001f;

    /// Event Processing Loop
    bool running = true;
    while(running)
    {
        /// Detect Events
        SDL_Event event;
        while(SDL_PollEvent(&event))
        {
            if(event.type == SDL_KEYDOWN)
                key_pressed[event.key.keysym.sym] = true;
            else if(event.type == SDL_KEYUP)
                key_pressed[event.key.keysym.sym] = false;
            else if(event.type == SDL_QUIT)
                running = false;
            else if(event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED)
            {
                settings.window_width = event.window.data1;
                settings.window_height = event.window.data2;
                resize_renderer(&settings, &camera_projection);
            }
        }

        /// Handle Events
        if(key_pressed[SDLK_ESCAPE])
            running = false;

        // Basic Camera Movement
        if(key_pressed[SDLK_w])
            camera_position = vec3_subtract_vec3(camera_position, v3(0.0f, 0.0f, camera_speed));
        if(key_pressed[SDLK_s])
            camera_position = vec3_add_vec3(camera_position, v3(0.0f, 0.0f, camera_speed));
        if(key_pressed[SDLK_a])
            camera_position = vec3_subtract_vec3(camera_position, vec3_multiply_scalar(vec3_normalize(vec3_cross(camera_forward, v3(0.0f, 1.0f, 0.0f))), camera_speed));
        if(key_pressed[SDLK_d])
            camera_position = vec3_add_vec3(camera_position, vec3_multiply_scalar(vec3_normalize(vec3_cross(camera_forward, v3(0.0f, 1.0f, 0.0f))), camera_speed));

        camera_view = lookat(camera_position, vec3_add_vec3(camera_position, camera_forward));
        shader_value(VIEW_MATRIX, &camera_view);

        /// Render
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glBindVertexArray(default_vao);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        SDL_GL_SwapWindow(window);
    }

    /// Cleanup
    glDeleteShader(shaders[0]);
    glDeleteShader(shaders[1]);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}