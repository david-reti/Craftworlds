#ifndef CAMERA_H
#define CAMERA_H
#include"math3d.h"

typedef struct CAMERA
{
    vec3 position, direction;
    mat4 view, projection;
    float pitch, yaw;
} CAMERA;

typedef enum { PERSPECTIVE_PROJECTION, ORTHOGRAPHIC_PROJECTION } PROJECTION_TYPE;

// Like lookat, but more streamlined for most cases becuase it assumes the up vector
mat4 camera_lookat(vec3 position, vec3 target)
{
    vec3 up = v3(0.0f, 1.0f, 0.0f);
    vec3 camera_direction = vec3_normalize(vec3_subtract_vec3(position, target));
    vec3 camera_right = vec3_normalize(vec3_cross(up, camera_direction));
    vec3 camera_up = vec3_cross(camera_direction, camera_right);
    return mat4_lookat(position, camera_direction, camera_up, camera_right);
}

mat4 camera_look_in_direction(vec3 position, vec3 direction)
{
    vec3 up = v3(0.0f, 1.0f, 0.0f);
    vec3 camera_right = vec3_normalize(vec3_cross(up, direction));
    vec3 camera_up = vec3_normalize(vec3_cross(direction, camera_right));
    return mat4_lookat(position, direction, camera_up, camera_right);
}

#define lookat camera_lookat
#define look_toward camera_look_in_direction

void recalculate_camera_view(CAMERA* camera)
{
    camera->direction.x = cos(deg2rad(camera->yaw)) * cos(deg2rad(camera->pitch));
    camera->direction.y = sin(deg2rad(camera->pitch));
    camera->direction.z = sin(deg2rad(camera->yaw)) * cos(deg2rad(camera->pitch));
    camera->view = look_toward(camera->position, vec3_negate(vec3_normalize(camera->direction)));
}

void move_camera(CAMERA* camera, vec3 amount) 
{ 
    camera->position = vec3_add_vec3(camera->position, vec3_multiply_scalar(camera->direction, -amount.z));
    camera->position = vec3_add_vec3(camera->position, vec3_multiply_scalar(v3(0.0f, 1.0f, 0.0f), amount.y));
    camera->position = vec3_add_vec3(camera->position, vec3_multiply_scalar(vec3_normalize(vec3_cross(camera->direction, v3(0.0f, 1.0f, 0.0f))), amount.x));
    recalculate_camera_view(camera);
}

void pitch_camera(CAMERA* camera, float amount) 
{ 
    camera->pitch += amount;
    recalculate_camera_view(camera);
}

void pitch_camera_with_limit(CAMERA* camera, float amount, float upper_limit, float lower_limit)
{
    if(camera->pitch + amount > lower_limit) { amount = lower_limit - camera->pitch; }
    else if(camera->pitch + amount < upper_limit) { amount = upper_limit - camera->pitch; }
    pitch_camera(camera, amount);
}

void yaw_camera(CAMERA* camera, float amount) 
{ 
    camera->yaw += amount;
    recalculate_camera_view(camera);
};

CAMERA make_camera(PROJECTION_TYPE proj_type, float render_width, float render_height, float fov)
{
    CAMERA to_return = { 0 };
    to_return.position = v3(0.0f, 0.0f, 0.0f);
    to_return.direction = v3(0.0f, 0.0f, -1.0f);
    if(proj_type == PERSPECTIVE_PROJECTION)
        to_return.projection = perspective_projection(render_width / render_height, fov, 0.1, 1000);
    else
        to_return.projection = orthographic_projection(render_width / render_height, 0.1, 1000);
    yaw_camera(&to_return, -90);
    return to_return;
}


#endif