#include<stdbool.h>
#include<SDL2/SDL.h>
#include<glad/glad.h>
#include<SDL2/SDL_opengl.h>

#include"math3d.h"
#include"shaders.h"
#include"camera.h"

typedef struct SETTINGS
{
    float fov;
    unsigned int window_width, window_height;
    bool invert_y_axis;
} SETTINGS;

void resize_renderer(SETTINGS* settings, CAMERA* camera)
{
    glViewport(0, 0, settings->window_width, settings->window_height);
    camera->projection = perspective_projection((float)settings->window_width / settings->window_height, settings->fov, 0.1, 1000);
    if(current_shader_program) set_shader_value(PROJECTION_MATRIX, &(camera->projection));
}

int main(int argc, char** argv)
{
    /// Initialize SDL
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_GLContext* context;
    SETTINGS settings = { .window_width = 1280, .window_height = 720, .fov = 45.0 , .invert_y_axis = true };
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
    CAMERA player_camera = make_camera(PERSPECTIVE_PROJECTION, settings.window_width, settings.window_height, settings.fov);
    resize_renderer(&settings, &player_camera);
    mat4 tranform = m4();
    set_shader_value(MODEL_MATRIX, &tranform);

    float camera_speed = 3.0f;
    float mouse_sensitivity = 1.0f; 
    float camera_pitch_limit_bottom = 89.0f, camera_pitch_limit_top = -89.0f;

    /// Event Processing Loop
    bool running = true;
    double elapsed_time = 0;
    unsigned long last_frame_time = 0;
    while(running)
    {
        /// Get Elapsed Time
        unsigned long current_time = SDL_GetTicks();
        elapsed_time = (current_time - last_frame_time) / 1000.0;
        last_frame_time = current_time;

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
            else if(event.type == SDL_MOUSEMOTION)
            {
                yaw_camera(&player_camera, event.motion.xrel * mouse_sensitivity);
                pitch_camera_with_limit(&player_camera, event.motion.yrel * mouse_sensitivity * (settings.invert_y_axis ? -1 : 1), camera_pitch_limit_top, camera_pitch_limit_bottom);
            }
            else if(event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED)
            {
                settings.window_width = event.window.data1;
                settings.window_height = event.window.data2;
                resize_renderer(&settings, &player_camera);
            }
        }

        /// Handle Events
        if(key_pressed[SDLK_ESCAPE])
            running = false;

        // Basic Camera Movement - all coordinates are in local camera space
        if(key_pressed[SDLK_w])
            move_camera(&player_camera, v3(0.0f, 0.0f, -camera_speed * elapsed_time));
        if(key_pressed[SDLK_s])
            move_camera(&player_camera, v3(0.0f, 0.0f, camera_speed * elapsed_time));
        if(key_pressed[SDLK_a])
            move_camera(&player_camera, v3(-camera_speed * elapsed_time, 0.0f, 0.0f));
        if(key_pressed[SDLK_d])
            move_camera(&player_camera, v3(camera_speed * elapsed_time, 0.0f, 0.0f));
        
        set_shader_value(VIEW_MATRIX, &(player_camera.view));

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