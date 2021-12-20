#ifndef MATH_3D
#define MATH_3D
#include<math.h>
#define PI 3.14159265359

float deg2rad(float degrees) { return degrees * (PI / 180); }
float rad2deg(float radians) { return radians * (180 / PI); }

/// Vectors
// Vector - a direction and strength (magnitude)
typedef struct vec2
{
    float x, y;
} vec2;

typedef struct vec3
{
    float x, y, z;
} vec3;

typedef vec3 POINT;

typedef struct vec4
{
    float x, y, z, w;
} vec4;

vec2 v2(float x, float y)
{
    vec2 to_return = { .x = x, .y = y };
    return to_return;
}

vec3 v3(float x, float y, float z)
{
    vec3 to_return = { .x = x, .y = y, .z = z };
    return to_return;
}

#define at v3

vec4 v4(float x, float y, float z, float w)
{
    vec4 to_return = { .x = x, .y = y, .z = z, .w = w };
    return to_return;
}

vec2 vec2_add_scalar(vec2 vector, float toadd)
{
    vector.x += toadd;
    vector.y += toadd;
    return vector;
}

vec2 vec2_add_vec2(vec2 vector, vec2 toadd)
{
    vector.x += toadd.x;
    vector.y += toadd.y;
    return vector;
}

vec2 vec2_scale_vec2(vec2 vector, vec2 toscale)
{
    vector.x *= toscale.x;
    vector.y *= toscale.y;
    return vector;
}

vec2 vec2_multiply_scalar(vec2 vector, float tomul)
{
    vector.x *= tomul;
    vector.y *= tomul;
    return vector;
}

vec2 vec2_divide_scalar(vec2 vector, float todivide)
{
    vector.x /= todivide;
    vector.y /= todivide;
    return vector;
}

vec3 vec3_add_scalar(vec3 vector, float toadd)
{
    vector.x += toadd;
    vector.y += toadd;
    vector.z += toadd;
    return vector;
}

vec3 vec3_subtract_scalar(vec3 vector, float tosub)
{
    vector.x -= tosub;
    vector.y -= tosub;
    vector.z -= tosub;
    return vector;
}

vec3 vec3_multiply_scalar(vec3 vector, float tomul)
{
    vector.x *= tomul;
    vector.y *= tomul;
    vector.z *= tomul;
    return vector;
}

vec3 vec3_divide_scalar(vec3 vector, float todiv)
{
    vector.x /= todiv;
    vector.y /= todiv;
    vector.z /= todiv;
    return vector;
}

vec3 vec3_scale(vec3 vector, vec3 tomul)
{
    vector.x *= tomul.x;
    vector.y *= tomul.y;
    vector.z *= tomul.z;
    return vector;
}

vec3 vec3_negate(vec3 vector) { return vec3_multiply_scalar(vector, -1.0f); }

vec3 vec3_add_vec3(vec3 vector1, vec3 vector2)
{
    vector1.x += vector2.x;
    vector1.y += vector2.y;
    vector1.z += vector2.z;
    return vector1;
}

vec3 vec3_subtract_vec3(vec3 vector1, vec3 vector2) { return vec3_add_vec3(vector1, vec3_negate(vector2)); }

// Length - pythagoras theoren
float vec3_length(vec3 vector)
{
    return sqrt((vector.x * vector.x) + (vector.y * vector.y) + (vector.z * vector.z));
}

// Normalize - keep the direction, but make the length 1
vec3 vec3_normalize(vec3 vector)
{
    return vec3_divide_scalar(vector, vec3_length(vector));
}

// Dot product - useful for getting the angle between the two vectors, the degree of which can be taken by applying cos-1 to it
float vec3_dot(vec3 vector1, vec3 vector2)
{
    float to_return = 0;
    to_return += vector1.x * vector2.x;
    to_return += vector1.y * vector2.y;
    to_return += vector1.z * vector2.z;
    return to_return;
}

// Cross product - results in a vector which is orthogonal to both of the input vectors (if they are not parallel, or already orthogonal)
vec3 vec3_cross(vec3 vector1, vec3 vector2)
{
    vec3 to_return = { 0 };
    to_return.x = (vector1.y * vector2.z) - (vector1.z * vector2.y);
    to_return.y = (vector1.z * vector2.x) - (vector1.x * vector2.z);
    to_return.z = (vector1.x * vector2.y) - (vector1.y * vector2.x);
    return to_return;
} 

vec4 vec4_add_scalar(vec4 vector, float toadd)
{
    vector.x += toadd;
    vector.y += toadd;
    vector.z += toadd;
    vector.w += toadd;
    return vector;
}

vec4 vec4_subtract_scalar(vec4 vector, float tosub)
{
    vector.x -= tosub;
    vector.y -= tosub;
    vector.z -= tosub;
    vector.w -= tosub;
    return vector;
}

vec4 vec4_multiply_scalar(vec4 vector, float tomul)
{
    vector.x *= tomul;
    vector.y *= tomul;
    vector.z *= tomul;
    vector.w *= tomul;
    return vector;
}

vec4 vec4_divide_scalar(vec4 vector, float todiv)
{
    vector.x /= todiv;
    vector.y /= todiv;
    vector.z /= todiv;
    vector.w /= todiv;
    return vector;
}

vec4 vec4_negate(vec4 vector) { return vec4_multiply_scalar(vector, -1.0f); }

vec4 vec4_add_vec4(vec4 vector1, vec4 vector2)
{
    vector1.x += vector2.x;
    vector1.y += vector2.y;
    vector1.z += vector2.z;
    vector1.w += vector2.w;
    return vector1;
}

vec4 vec4_subtract_vec4(vec4 vector1, vec4 vector2) { return vec4_add_vec4(vector1, vec4_negate(vector2)); }

// Length - pythagoras theoren
float vec4_length(vec4 vector)
{
    return sqrtf((vector.x * vector.x) + (vector.y * vector.y) + (vector.z * vector.z) + (vector.w * vector.w));
}

// Normalize - keep the direction, but make the length 1
vec4 vec4_normalize(vec4 vector)
{
    return vec4_divide_scalar(vector, vec4_length(vector));
}

// Dot product - useful for getting the angle between the two vectors, the degree of which can be taken by applying cos-1 to it
float vec4_dot(vec4 vector1, vec4 vector2)
{
    float to_return = 0;
    to_return += vector1.x * vector2.x;
    to_return += vector1.y * vector2.y;
    to_return += vector1.z * vector2.z;
    to_return += vector1.w * vector2.w;
    return to_return;
}

// Matrices
// Matrix - A 2d array of mathematical expressions, which is indexed by the column first (opposite of what we would expect)
typedef struct mat3 
{
    float values[3][3];
} mat3;

typedef struct mat4 
{
    float values[4][4];
} mat4;

mat3 mat3_add_mat3(mat3 matrix1, mat3 matrix2)
{
    for(unsigned short i = 0; i < 3; i++)
        for(unsigned short j = 0; j < 3; j++)
            matrix1.values[i][j] += matrix2.values[i][j];
    return matrix1;
}

mat3 mat3_subtract_mat3(mat3 matrix1, mat3 matrix2)
{
    for(unsigned short i = 0; i < 3; i++)
        for(unsigned short j = 0; j < 3; j++)
            matrix1.values[i][j] -= matrix2.values[i][j];
    return matrix1;
}

mat3 mat3_multiply_scalar(mat3 matrix1, float tomul)
{
    for(unsigned short i = 0; i < 3; i++)
        for(unsigned short j = 0; j < 3; j++)
            matrix1.values[i][j] *= tomul;
    return matrix1;
}

// Matrix multiplication - not commutative (a x b != b x a)
mat3 mat3_multiply_mat3(mat3 matrix1, mat3 matrix2)
{
    mat3 to_return;
    for(unsigned short i = 0; i < 3; i++)
    {
        to_return.values[i][0] = matrix1.values[i][0] * matrix2.values[0][0] + matrix1.values[i][1] * matrix2.values[1][0] + matrix1.values[i][2] * matrix2.values[2][0];
        to_return.values[i][1] = matrix1.values[i][0] * matrix2.values[0][1] + matrix1.values[i][1] * matrix2.values[1][1] + matrix1.values[i][2] * matrix2.values[2][1];
        to_return.values[i][2] = matrix1.values[i][0] * matrix2.values[0][2] + matrix1.values[i][1] * matrix2.values[1][2] + matrix1.values[i][2] * matrix2.values[2][2];
    }
    return to_return;
}

vec3 mat3_multiply_vec3(mat3 matrix, vec3 vector)
{
    vec3 to_return;
    to_return.x = matrix.values[0][0] * vector.x + matrix.values[0][1] * vector.y + matrix.values[0][2] * vector.z;
    to_return.y = matrix.values[1][0] * vector.x + matrix.values[1][1] * vector.y + matrix.values[1][2] * vector.z;
    to_return.z = matrix.values[2][0] * vector.x + matrix.values[2][1] * vector.y + matrix.values[2][2] * vector.z;
    return to_return;
}

mat3 mat3_identity()
{
    mat3 to_return;
    memset(to_return.values, 0, 9 * sizeof(float));
    to_return.values[0][0] = 1.0f;
    to_return.values[1][1] = 1.0f;
    to_return.values[2][2] = 1.0f;
    return to_return;
}

#define m3 mat3_identity

mat4 mat4_add_mat4(mat4 matrix1, mat4 matrix2)
{
    for(unsigned short i = 0; i < 4; i++)
        for(unsigned short j = 0; j < 4; j++)
            matrix1.values[i][j] += matrix2.values[i][j];
    return matrix1;
}

mat4 mat4_subtract_mat3(mat4 matrix1, mat4 matrix2)
{
    for(unsigned short i = 0; i < 4; i++)
        for(unsigned short j = 0; j < 4; j++)
            matrix1.values[i][j] -= matrix2.values[i][j];
    return matrix1;
}

mat4 mat4_multiply_scalar(mat4 matrix1, float tomul)
{
    for(unsigned short i = 0; i < 4; i++)
        for(unsigned short j = 0; j < 4; j++)
            matrix1.values[i][j] *= tomul;
    return matrix1;
}

// Matrix multiplication - not commutative (a x b != b x a)
mat4 mat4_multiply_mat4(mat4 matrix1, mat4 matrix2)
{
    mat4 to_return;
    for(unsigned short i = 0; i < 4; i++)
    {
        to_return.values[i][0] = matrix1.values[i][0] * matrix2.values[0][0] + matrix1.values[i][1] * matrix2.values[1][0] + matrix1.values[i][2] * matrix2.values[2][0] + matrix1.values[i][3] * matrix2.values[3][0];
        to_return.values[i][1] = matrix1.values[i][0] * matrix2.values[0][1] + matrix1.values[i][1] * matrix2.values[1][1] + matrix1.values[i][2] * matrix2.values[2][1] + matrix1.values[i][3] * matrix2.values[3][1];
        to_return.values[i][2] = matrix1.values[i][0] * matrix2.values[0][2] + matrix1.values[i][1] * matrix2.values[1][2] + matrix1.values[i][2] * matrix2.values[2][2] + matrix1.values[i][3] * matrix2.values[3][2];
        to_return.values[i][3] = matrix1.values[i][0] * matrix2.values[0][3] + matrix1.values[i][1] * matrix2.values[1][3] + matrix1.values[i][2] * matrix2.values[2][3] + matrix1.values[i][3] * matrix2.values[3][3];
    }
    return to_return;
}

vec4 mat4_multiply_vec4(mat4 matrix, vec4 vector)
{
    vec4 to_return;
    to_return.x = matrix.values[0][0] * vector.x + matrix.values[0][1] * vector.y + matrix.values[0][2] * vector.z + matrix.values[0][3] * vector.w;
    to_return.y = matrix.values[1][0] * vector.x + matrix.values[1][1] * vector.y + matrix.values[1][2] * vector.z + matrix.values[1][3] * vector.w;
    to_return.z = matrix.values[2][0] * vector.x + matrix.values[2][1] * vector.y + matrix.values[2][2] * vector.z + matrix.values[2][3] * vector.w;
    to_return.w = matrix.values[3][0] * vector.x + matrix.values[3][1] * vector.y + matrix.values[3][2] * vector.z + matrix.values[3][3] * vector.w;
    return to_return;
}

mat4 mat4_identity()
{
    mat4 to_return;
    memset(to_return.values, 0, 16 * sizeof(float));
    to_return.values[0][0] = 1.0f;
    to_return.values[1][1] = 1.0f;
    to_return.values[2][2] = 1.0f;
    to_return.values[3][3] = 1.0f;
    return to_return;
}

#define m4 mat4_identity

// Generate a scaling matrix (which essentially involves multiplying each vector element)
mat4 mat4_scale(vec3 factor)
{
    mat4 to_return = m4();
    to_return.values[0][0] = factor.x;
    to_return.values[1][1] = factor.y;
    to_return.values[2][2] = factor.z;
    return to_return;
}

// Generate a translation matrix (which essentially involves adding a vector on top of the first)
mat4 mat4_translate(vec3 amount)
{
    mat4 to_return = m4();
    to_return.values[3][0] = amount.x;
    to_return.values[3][1] = amount.y;
    to_return.values[3][2] = amount.z;
    return to_return;
}

mat4 mat4_rotate_x(float radians)
{
    mat4 to_return = m4();
    to_return.values[1][1] = cos(radians);
    to_return.values[1][2] = -sin(radians);
    to_return.values[2][1] = sin(radians);
    to_return.values[2][2] = cos(radians);
    return to_return;
}

mat4 mat4_rotate_y(float radians)
{
    mat4 to_return = m4();
    to_return.values[0][0] = cos(radians);
    to_return.values[0][2] = sin(radians);
    to_return.values[2][0] = -sin(radians);
    to_return.values[2][2] = cos(radians);
    return to_return;
}

mat4 mat4_rotate_z(float radians)
{
    mat4 to_return = m4();
    to_return.values[0][0] = cos(radians);
    to_return.values[0][1] = -sin(radians);
    to_return.values[1][0] = sin(radians);
    to_return.values[1][1] = cos(radians);
    return to_return;
}

// This has not been thoroughly checked, and I mostly added it for completeness
mat4 mat4_rotate_axis(vec3 axis, float radians)
{
    mat4 to_return = m4();
    to_return.values[0][0] = cos(radians) + ((axis.x * axis.x) * (1 - cos(radians)));
    to_return.values[0][1] = (axis.x * axis.y * (1 - cos(radians))) - (axis.z * sin(radians));
    to_return.values[0][2] = (axis.x * axis.z * (1 - cos(radians))) + (axis.y * sin(radians));
    to_return.values[1][0] = (axis.y * axis.x * (1 - cos(radians))) + (axis.z * sin(radians));
    to_return.values[1][1] = cos(radians) + (axis.y * axis.y * (1 - cos(radians)));
    to_return.values[1][2] = (axis.y * axis.z * (1 - cos(radians))) - (axis.x * sin(radians));
    to_return.values[2][0] = (axis.z * axis.x * (1 - cos(radians))) - (axis.y * sin(radians));
    to_return.values[2][1] = (axis.z * axis.y * (1 - cos(radians))) + (axis.x * sin(radians));
    to_return.values[2][2] = cos(radians) + (axis.z * axis.z * (1 - cos(radians)));
    return to_return;
}

// Note about transformations: since the last matrix is applied to the vector first, read them from right to left
// The recommended order is to first scale, then rotate and lastly translate


mat4 mat4_orthographic_projection(float aspect_ratio, float near, float far)
{
    mat4 to_return = m4();
    float right = aspect_ratio, left = -aspect_ratio, bottom = -1, top = 1;
    to_return.values[0][0] = 1.0 / aspect_ratio;
    to_return.values[2][2] = -2.0 / (far - near);
    to_return.values[3][2] = -((far + near) / (far - near));
    return to_return;
}

mat4 mat4_perspective_projection(float aspect_ratio, float fov, float near, float far)
{
    mat4 to_return = m4();
    to_return.values[0][0] = 1.0 / (aspect_ratio * tan(fov / 2));
    to_return.values[1][1] = 1.0 / tan(fov / 2);
    to_return.values[2][2] = -((far + near) / (far - near));
    to_return.values[3][2] = -((2 * near * far) / (far - near));
    to_return.values[2][3] = -1;
    return to_return;
}

mat4 mat4_lookat(vec3 position, vec3 direction, vec3 up, vec3 right)
{
    mat4 to_return = m4();
    to_return.values[0][0] = right.x;
    to_return.values[1][0] = right.y;
    to_return.values[2][0] = right.z;
    to_return.values[0][1] = up.x;
    to_return.values[1][1] = up.y;
    to_return.values[2][1] = up.z;
    to_return.values[0][2] = direction.x;
    to_return.values[1][2] = direction.y;
    to_return.values[2][2] = direction.z;
    return mat4_multiply_mat4(mat4_translate(vec3_negate(position)), to_return);
}

#define translate mat4_translate
#define rotate_x mat4_rotate_x
#define rotate_y mat4_rotate_y
#define rotate_z mat4_rotate_z
#define rotate_axis mat4_rotate_axis
#define orthographic_projection mat4_orthographic_projection
#define perspective_projection mat4_perspective_projection
#endif