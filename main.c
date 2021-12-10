#include<stdbool.h>
#include<SDL2/SDL.h>
#include<glad/glad.h>
#include<SDL2/SDL_opengl.h>

#include"math3d.h"
#include"shaders.h"
#include"camera.h"
#include"world.h"

typedef struct SETTINGS
{
    float fov, look_sensitivity;
    unsigned int window_width, window_height;
    bool invert_y_axis, show_fps;
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
    SETTINGS settings = { 
                          .window_width = 1280, 
                          .window_height = 720, 
                          .fov = 45.0, 
                          .invert_y_axis = true,
                          .look_sensitivity = 0.5f,
                          .show_fps = true
                        };
    bool key_pressed[256] = { 0 };

    if(failed(SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_TIMER)))
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

    if(SDL_SetRelativeMouseMode(true) == -1)
        exit_with_error("Could not set the proper mouse mode for the game", SDL_GetError());

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CW);

    /// Setting up the rendering - temporary
    // Load the vertex & fragment shaders
    unsigned int default_program = shader_program(DEFAULT_VERTEX, DEFAULT_FRAGMENT);

    // Make the first terrain chunk
    CHUNK* chunk = make_chunk();

    // Create and configure the camera
    CAMERA player_camera = make_camera(PERSPECTIVE_PROJECTION, settings.window_width, settings.window_height, settings.fov);
    resize_renderer(&settings, &player_camera);
    move_camera(&player_camera, v3(1.0f, 1.0f, 2.0f));

    float camera_speed = 3.0f;
    float camera_pitch_limit_bottom = 89.0f, camera_pitch_limit_top = -89.0f;

    /// Event Processing Loop
    bool running = true;
    double elapsed_time = 0, fps = 0;
    unsigned long long current_time, last_frame_time = 0;
    while(running)
    {
        /// Get Elapsed Time
        current_time = SDL_GetTicks();
        elapsed_time = (current_time - last_frame_time) / 1000.0;
        last_frame_time = current_time;

        if(settings.show_fps)
        {
            printf("\rfps: %lf", 1.0 / elapsed_time);
            fflush(stdout);
        }

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
                yaw_camera(&player_camera, event.motion.xrel * settings.look_sensitivity);
                pitch_camera_with_limit(&player_camera, event.motion.yrel * settings.look_sensitivity * (settings.invert_y_axis ? -1 : 1), camera_pitch_limit_top, camera_pitch_limit_bottom);
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
        render_chunk(chunk);
        SDL_GL_SwapWindow(window);
    }

    /// Cleanup
    unload_chunk(chunk);
    unload_shaders();
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}