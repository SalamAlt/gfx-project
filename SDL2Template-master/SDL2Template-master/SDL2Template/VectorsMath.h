#pragma once

/*In charge of most vector related operations
defines 3d and 2d vectors all with a w component (homogeneous)
*/

// Created a 2D structure to hold texture coordinates
struct vec2d
{
    float u = 0;
    float v = 0;
    float w = 1;
};

struct vec3d
{
    float x = 0;
    float y = 0;
    float z = 0;
    float w = 1; // Need a 4th term for homogeneous operations
};

#ifndef VECTORSMATH_H_INCLUDED
#define VECTORSMATH_H_INCLUDED

vec3d Vector_Add(vec3d& v1, vec3d& v2);
vec3d Vector_Sub(vec3d& v1, vec3d& v2);
vec3d Vector_Mul(vec3d& v1, float k);
vec3d Vector_Div(vec3d& v1, float k);
float Vector_DotProduct(vec3d& v1, vec3d& v2);

float Vector_Length(vec3d& v);

vec3d Vector_Normalise(vec3d& v);

vec3d Vector_CrossProduct(vec3d& v1, vec3d& v2);

vec3d Vector_IntersectPlane(vec3d& plane_p, vec3d& plane_n, vec3d& lineStart, vec3d& lineEnd, float& t);

#endif