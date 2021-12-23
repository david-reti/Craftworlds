#include<stdbool.h>
#include<SDL2/SDL.h>
#include<glad/glad.h>
#include<SDL2/SDL_opengl.h>

#include"util.h"
#include"world.h"
#include"camera.h"
#include"shaders.h"

typedef struct SETTINGS
{
    float fov, look_sensitivity, max_render_distance;
    unsigned int window_width, window_height;
    bool invert_y_axis, show_fps, render_wireframe;
} SETTINGS;

void resize_renderer(SETTINGS* settings, CAMERA* camera)
{
    glViewport(0, 0, settings->window_width, settings->window_height);
    camera->projection = perspective_projection((float)settings->window_width / settings->window_height, settings->fov, 0.1, settings->max_render_distance);
    if(current_shader_program) set_shader_value(PROJECTION_MATRIX, &(camera->projection));
}

void switch_render_settings(SETTINGS* settings)
{
    if(settings->render_wireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
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
                          .show_fps = true,
                          .render_wireframe = false,
                          .max_render_distance = 500
                        };
    bool key_pressed[256] = { 0 };

    #ifdef DEBUG
    open_console_window();
    #endif

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

    initialise_timer();
    switch_render_settings(&settings);

    /// Setting up the rendering - temporary
    // Load the vertex & fragment shaders
    unsigned int blocks_shader_program = shader_program(BLOCK_VERTEX_SHADER, BLOCK_FRAGMENT_SHADER);
    unsigned int debug_shader_program = shader_program(BLOCK_VERTEX_SHADER, DEBUG_FRAGMENT_SHADER);
    apply_shader_program(blocks_shader_program);

    // Make the first terrain chunk
    load_block_textures();
    CHUNK* chunk = make_chunk(at(0.0f, 0.0f, 0.0f));

    // Create and configure the camera
    CAMERA player_camera = make_camera(PERSPECTIVE_PROJECTION, settings.window_width, settings.window_height, settings.fov);
    vec3 initial_player_position = vec3_add_vec3(top_cube(10, -10), v3(0.0f, 3.0f, 0.0f));
    resize_renderer(&settings, &player_camera);
    move_camera(&player_camera, initial_player_position);

    float camera_speed = 10.0f;
    float camera_pitch_limit_bottom = 89.0f, camera_pitch_limit_top = -89.0f;

    /// Event Processing Loop
    bool running = true;
    double elapsed_time = 0;
    unsigned long setting_switch_cooldown = 0;
    while(running)
    {
        /// Get Elapsed Time
        elapsed_time = get_elapsed_time();

        if(settings.show_fps)
        {
            printf("\rfps: %.2lf", 1.0 / elapsed_time);
            fflush(stdout);
        }

        /// Detect Events
        SDL_Event event;
        while(SDL_PollEvent(&event))
        {
            if(event.type == SDL_KEYDOWN)
                key_pressed[clamp(event.key.keysym.sym, 0, 255)] = true;
            else if(event.type == SDL_KEYUP)
                key_pressed[clamp(event.key.keysym.sym, 0, 255)] = false;
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
        if(key_pressed[SDLK_q] && SDL_GetTicks() - setting_switch_cooldown > 400)
        {
            settings.render_wireframe = !settings.render_wireframe;
            if(settings.render_wireframe) apply_shader_program(debug_shader_program);
            else apply_shader_program(blocks_shader_program);
            switch_render_settings(&settings);
            resize_renderer(&settings, &player_camera);
            setting_switch_cooldown = SDL_GetTicks();
        }

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