#pragma once
//in charge of all matrix operations

#ifndef MATRICES_H_INCLUDED
#define MATRICES_H_INCLUDED

struct mat4x4
{
    float m[4][4] = {0};
};
#include "VectorsMath.h"
#include <algorithm>

vec3d Matrix_MultiplyVector(mat4x4& m, vec3d& i);

mat4x4 Matrix_MakeIdentity();
mat4x4 Matrix_MakeRotationX(float fAngleRad);
mat4x4 Matrix_MakeRotationY(float fAngleRad);
mat4x4 Matrix_MakeRotationZ(float fAngleRad);
mat4x4 Matrix_MakeTranslation(float x, float y, float z);

mat4x4 Matrix_MakeOrthogonal(float fNear, float fFar, float screenWidth, float screenHeight);
mat4x4 Matrix_MakeProjection(float fFovDegrees, float fAspectRatio, float fNear, float fFar);
mat4x4 Matrix_MultiplyMatrix(mat4x4& m1, mat4x4& m2);

mat4x4 Matrix_PointAt(vec3d& pos, vec3d& target, vec3d& up);

mat4x4 Matrix_QuickInverse(mat4x4& m); // Only for Rotation/Translation Matrices

void UpdateViews(mat4x4& matWorld);

#endif