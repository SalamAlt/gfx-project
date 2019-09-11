#include "Matrices.h"
#include <math.h>
/*
Credits to wikipedia for the rotation matrices, scratchapixel.com for the others
*/
extern float fTheta;

//multiplies a vector and matrix using row major order style
vec3d Matrix_MultiplyVector(mat4x4 &m, vec3d &i)
{
    vec3d v;
    v.x = i.x * m.m[0][0] + i.y * m.m[1][0] + i.z * m.m[2][0] + i.w * m.m[3][0];
    v.y = i.x * m.m[0][1] + i.y * m.m[1][1] + i.z * m.m[2][1] + i.w * m.m[3][1];
    v.z = i.x * m.m[0][2] + i.y * m.m[1][2] + i.z * m.m[2][2] + i.w * m.m[3][2];
    v.w = i.x * m.m[0][3] + i.y * m.m[1][3] + i.z * m.m[2][3] + i.w * m.m[3][3];
    return v;
}

//identity matrix
mat4x4 Matrix_MakeIdentity()
{
    mat4x4 matrix;
    matrix.m[0][0] = 1.0f;
    matrix.m[1][1] = 1.0f;
    matrix.m[2][2] = 1.0f;
    matrix.m[3][3] = 1.0f;
    return matrix;
}

// row major order, referenced wikipedia for this
mat4x4 Matrix_MakeRotationX(float fAngleRad)
{
    mat4x4 matrix;
    matrix.m[0][0] = 1.0f;
    matrix.m[1][1] = cosf(fAngleRad);
    matrix.m[1][2] = sinf(fAngleRad);
    matrix.m[2][1] = -sinf(fAngleRad);
    matrix.m[2][2] = cosf(fAngleRad);
    matrix.m[3][3] = 1.0f;
    return matrix;
}

// row major order, referenced wikipedia for this
mat4x4 Matrix_MakeRotationY(float fAngleRad)
{
    mat4x4 matrix;
    matrix.m[0][0] = cosf(fAngleRad);
    matrix.m[0][2] = sinf(fAngleRad);
    matrix.m[2][0] = -sinf(fAngleRad);
    matrix.m[1][1] = 1.0f;
    matrix.m[2][2] = cosf(fAngleRad);
    matrix.m[3][3] = 1.0f;
    return matrix;
}

//row major order, referenced wikipedia for this
mat4x4 Matrix_MakeRotationZ(float fAngleRad)
{
    mat4x4 matrix;
    matrix.m[0][0] = cosf(fAngleRad);
    matrix.m[0][1] = sinf(fAngleRad);
    matrix.m[1][0] = -sinf(fAngleRad);
    matrix.m[1][1] = cosf(fAngleRad);
    matrix.m[2][2] = 1.0f;
    matrix.m[3][3] = 1.0f;
    return matrix;
}

//row major order, simple trans matrix
mat4x4 Matrix_MakeTranslation(float x, float y, float z)
{
    mat4x4 matrix;
    matrix.m[0][0] = 1.0f;
    matrix.m[1][1] = 1.0f;
    matrix.m[2][2] = 1.0f;
    matrix.m[3][3] = 1.0f;
    matrix.m[3][0] = x;
    matrix.m[3][1] = y;
    matrix.m[3][2] = z;
    return matrix;
}

//assuming a symmetric fulcrum,
//the equation is everywhere; i used a row major order form (tranpose of matrix in textbook)
mat4x4 Matrix_MakeProjection(float fFovDegrees, float fAspectRatio, float fNear, float fFar)
{
    float fFovRad = 1.0f / tanf(fFovDegrees * 0.5f / 180.0f * 3.14159f);
    mat4x4 matrix;
    matrix.m[0][0] = fAspectRatio * fFovRad;
			   matrix.m[1][1] = fFovRad;
						 matrix.m[2][2] = (fFar+fNear) / (fFar - fNear);	   matrix.m[2][3] = 1.0f;
						 matrix.m[3][2] = (-2*fFar * fNear) / (fFar - fNear);    matrix.m[3][3] = 0.0f;	//i added -2
    return matrix;
}

//row major order multiplication
mat4x4 Matrix_MultiplyMatrix(mat4x4& m1, mat4x4& m2)
{
    mat4x4 matrix;
    for (int c = 0; c < 4; c++)
        for (int r = 0; r < 4; r++)
            matrix.m[r][c] =
                m1.m[r][0] * m2.m[0][c] + m1.m[r][1] * m2.m[1][c] + m1.m[r][2] * m2.m[2][c] + m1.m[r][3] * m2.m[3][c];
    return matrix;
}

//creates a point-at vector from the camera
//from the slides and reference from scratchapixel.com to not get my signs wrong
mat4x4 Matrix_PointAt(vec3d& pos, vec3d& target, vec3d& up)
{
    //calculate new forward direction
    vec3d newForward = Vector_Sub(target, pos);
    newForward = Vector_Normalise(newForward);

    // calculate new Up direction
    vec3d a = Vector_Mul(newForward, Vector_DotProduct(up, newForward));
    vec3d newUp = Vector_Sub(up, a);
    newUp = Vector_Normalise(newUp);

    //new right direction using cross product
    vec3d newRight = Vector_CrossProduct(newUp, newForward);

    // Construct matrix. Again, it's row major order from which is different text
    mat4x4 matrix;
    matrix.m[0][0] = newRight.x;
    matrix.m[0][1] = newRight.y;
    matrix.m[0][2] = newRight.z;
    matrix.m[0][3] = 0.0f;
    matrix.m[1][0] = newUp.x;
    matrix.m[1][1] = newUp.y;
    matrix.m[1][2] = newUp.z;
    matrix.m[1][3] = 0.0f;
    matrix.m[2][0] = newForward.x;
    matrix.m[2][1] = newForward.y;
    matrix.m[2][2] = newForward.z;
    matrix.m[2][3] = 0.0f;
    matrix.m[3][0] = pos.x;
    matrix.m[3][1] = pos.y;
    matrix.m[3][2] = pos.z;
    matrix.m[3][3] = 1.0f;
    return matrix;
}

//simple inverse function with concatenated operations
mat4x4 Matrix_QuickInverse(mat4x4& m) // Only for Rotation/Translation Matrices
{
    mat4x4 matrix;
    matrix.m[0][0] = m.m[0][0];
    matrix.m[0][1] = m.m[1][0];
    matrix.m[0][2] = m.m[2][0];
    matrix.m[0][3] = 0.0f;
    matrix.m[1][0] = m.m[0][1];
    matrix.m[1][1] = m.m[1][1];
    matrix.m[1][2] = m.m[2][1];
    matrix.m[1][3] = 0.0f;
    matrix.m[2][0] = m.m[0][2];
    matrix.m[2][1] = m.m[1][2];
    matrix.m[2][2] = m.m[2][2];
    matrix.m[2][3] = 0.0f;
    matrix.m[3][0] = -(m.m[3][0] * matrix.m[0][0] + m.m[3][1] * matrix.m[1][0] + m.m[3][2] * matrix.m[2][0]);
    matrix.m[3][1] = -(m.m[3][0] * matrix.m[0][1] + m.m[3][1] * matrix.m[1][1] + m.m[3][2] * matrix.m[2][1]);
    matrix.m[3][2] = -(m.m[3][0] * matrix.m[0][2] + m.m[3][1] * matrix.m[1][2] + m.m[3][2] * matrix.m[2][2]);
    matrix.m[3][3] = 1.0f;
    return matrix;
}

//this creates a camera to world matrix that later gets inverted
//concatenates rotations and translations
void UpdateViews(mat4x4& matWorld)
{
    // Set up "World Tranmsform" though not updating theta
    // makes this redundant
    mat4x4 matRotZ, matRotX;
   //  fTheta += 0.02f; // Uncomment to make objects spin
    matRotZ = Matrix_MakeRotationZ(fTheta*0.5f);
    matRotX = Matrix_MakeRotationX(fTheta);

    mat4x4 matTrans;
    matTrans = Matrix_MakeTranslation(0.0f, 0.0f, 5.0f);	//5 is arbitrary

    matWorld = Matrix_MakeIdentity();                     // Form World Matrix
    matWorld = Matrix_MultiplyMatrix(matRotZ, matRotX);   // Transform by rotation
    matWorld = Matrix_MultiplyMatrix(matWorld, matTrans); // Transform by translation
}


//makes an orthogonal matrix for view space->NDCS
mat4x4 Matrix_MakeOrthogonal(float fNear, float fFar, float screenWidth, float screenHeight)
{
    mat4x4 matrix;
    matrix.m[0][0] = (float)(2.0f / screenWidth);
    matrix.m[1][1] = (float)(2.0f / screenHeight);
    matrix.m[2][2] = 2.0f / (fNear - fFar);
    matrix.m[3][2] = (-fFar - fNear) / (fFar - fNear);
    matrix.m[2][3] = 0.0f;
    matrix.m[3][3] = 1.f;
    return matrix;
}