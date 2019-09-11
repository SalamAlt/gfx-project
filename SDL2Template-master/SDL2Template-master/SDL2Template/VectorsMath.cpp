
#include "VectorsMath.h"
#include <math.h>
#include <algorithm>

/*
functions in charge of all vector operations
*/

using namespace std;


vec3d Vector_Add(vec3d& v1, vec3d& v2) { return {v1.x + v2.x, v1.y + v2.y, v1.z + v2.z}; }

vec3d Vector_Sub(vec3d& v1, vec3d& v2) { return {v1.x - v2.x, v1.y - v2.y, v1.z - v2.z}; }

vec3d Vector_Mul(vec3d& v1, float k) { return {v1.x * k, v1.y * k, v1.z * k}; }

vec3d Vector_Div(vec3d& v1, float k) { return {v1.x / k, v1.y / k, v1.z / k}; }

float Vector_DotProduct(vec3d& v1, vec3d& v2) { return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z; }

float Vector_Length(vec3d& v) { return sqrtf(Vector_DotProduct(v, v)); }

vec3d Vector_Normalise(vec3d& v)
{
    float l = Vector_Length(v);
    return {v.x / l, v.y / l, v.z / l};
}

vec3d Vector_CrossProduct(vec3d& v1, vec3d& v2)
{
    vec3d v;
    v.x = v1.y * v2.z - v1.z * v2.y;
    v.y = v1.z * v2.x - v1.x * v2.z;
    v.z = v1.x * v2.y - v1.y * v2.x;
    return v;
}

//used for sutherland hodgman algorithm
//returns a vector at the intersection of a line and a plane
vec3d Vector_IntersectPlane(vec3d& plane_p, vec3d& plane_n, vec3d& lineStart, vec3d& lineEnd, float& t)
{
    plane_n = Vector_Normalise(plane_n);
    float plane_d = -Vector_DotProduct(plane_n, plane_p);
    float ad = Vector_DotProduct(lineStart, plane_n);
    float bd = Vector_DotProduct(lineEnd, plane_n);
    t = (-plane_d - ad) / (bd - ad);
    vec3d lineStartToEnd = Vector_Sub(lineEnd, lineStart);
    vec3d lineToIntersect = Vector_Mul(lineStartToEnd, t);
    return Vector_Add(lineStart, lineToIntersect);
}
