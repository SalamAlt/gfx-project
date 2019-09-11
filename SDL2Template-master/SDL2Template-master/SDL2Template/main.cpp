#pragma warning(push, 0)
#pragma warning(disable : ALL_CODE_ANALYSIS_WARNINGS)

//library and header files below
#include "Matrices.h"
#include "PolygonObject.h"
#include "Sprite.h"
#include "VectorsMath.h"

#include <SDL.h>
#include <SDL_ttf.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include <string>
#include <strstream>

#pragma warning(pop)

// current settings are to display a cube with a texture on each face
//recommended combos: hasTex = true, useObjModel = false
// and				hasTex = false, useObjModel = true

// if the hasTex bool is false, then the ambient lighting will be shown
// if useObjModel is true, a few objects will be loaded into the scene
	//i recommend keeping hastex = false if using .obj models since 
		//i didnt make or find any suitable textures for the objects
bool hasTex = false;     // false -> models will not have textures
bool useObjModel = true; // true -> load obj file with specified name in main, else use a standard cube defined in main
bool drawWireOnly = false;	//draws only wire models of the objects



#define M_PI_F 3.14159265f
#define ScreenWidth 640		//settable
#define ScreenHeight 480	//settable

using namespace std;

//function prototypes below
inline void Draw(int x, int y, uint32_t (*pixels)[ScreenHeight][ScreenWidth], uint32_t col = 0xFF0000FF);
void DrawLine(int x1, int y1, int x2, int y2, uint32_t (*pixels)[ScreenHeight][ScreenWidth], uint32_t col = 0xFF0000FF);
void DrawTriangle(int x1, int y1, int x2, int y2, int x3, int y3, uint32_t (*pixels)[ScreenHeight][ScreenWidth],
                  uint32_t col = 0xFF0000FF);
void TexturedTriangle(int x1, int y1, float u1, float v1, float w1, int x2, int y2, float u2, float v2, float w2,
                      int x3, int y3, float u3, float v3, float w3, Sprite* tex,
                      uint32_t (*pixels)[ScreenHeight][ScreenWidth]);
void FillTriangle(int x1, int y1, int x2, int y2, int x3, int y3, uint32_t (*pixels)[ScreenHeight][ScreenWidth],
                  uint32_t col = 0xFF0000FF);
int Triangle_ClipAgainstPlane(vec3d plane_p, vec3d plane_n, triangle& in_tri, triangle& out_tri1, triangle& out_tri2);

std::vector<mesh> objects;	//scene objects container
std::vector<Sprite*> sprites;	//scene objects' textures container

mat4x4 matOrtho; // Matrix that converts from view space to ndc space
mat4x4 matProj;  // Matrix that converts from view space to clip space
mat4x4 matWorld; // model to world transform
mat4x4 matView;  // world to view transform
mat4x4 matCamera;
mat4x4 matCameraRot;

vec3d vUp = {0, 1, 0};	//define vup as +y
vec3d vCamera;  // Location of camera in world space
vec3d vLookDir; // Direction vector along the direction camera points
float fYaw;     // FPS Camera rotation in XZ plane
float fTheta;   // Spins World transform

float* pDepthBuffer;	//z-buffer
bool orthographic = false;

inline uint32_t game_running = 0;
void emscripten_cancel_main_loop() { game_running = 0; }

//calls the main game loop. from the tutorials..
void emscripten_set_main_loop_arg(void (*fcn)(void*), void* const arg, uint32_t const fps, int const)
{
    uint32_t const MPF = fps > 0 ? 1000 / fps : 1000 / 60; // milliseconds per frame

    // Only render a frame if at least the MPF has been passed in time
    game_running = 1;
    while (game_running)
    {
        uint32_t const startTime = SDL_GetTicks();
        fcn(arg);
        uint32_t const endTime = SDL_GetTicks();

        uint32_t const timeDiff = endTime - startTime;

        if (timeDiff < MPF)
        {
            SDL_Delay(MPF - timeDiff); // Sleep for the remainder of the time to maintain FPS
        }
    }
}

// Render properties
// Defaults: 640x480
struct GameResources
{
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    SDL_Surface* canvas;
    uint32_t (*pixels)[ScreenHeight][ScreenWidth];	//framebuffer

    TTF_Font* font;

    uint32_t windowID;

    ~GameResources() noexcept
    {
        TTF_CloseFont(font);
        SDL_FreeSurface(canvas);
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
    }
};

//user input method
int ProcessEvent(uint32_t windowID)
{
    char e = 0;

    static SDL_Event event;

	//go through each event
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_WINDOWEVENT:
        {
            if (event.window.windowID == windowID)
            {
                switch (event.window.event)
                {

					//closed window
                case SDL_WINDOWEVENT_CLOSE:
                    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Window closed\n");
                    event.type = SDL_QUIT;
                    SDL_PushEvent(&event);
                    break;

                default:
                    break;
                }
            }
        }
        break;

        case SDL_QUIT:
        {
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "SDL_QUIT!\n");
            return 0; // 0 -- signal terminate program
        }

        case SDL_KEYUP:
        {
            SDL_Keycode key = event.key.keysym.sym;
			//set camera mode between persp and ortho
			//only on keyup
            if (key == SDLK_q) // q
            {
                if (orthographic)
                    orthographic = false; // travel y axis upwards
                else
                    orthographic = true;
            }
            break;
		}

        case SDL_KEYDOWN:
        {
            if (event.key.repeat == 1)
            {
                ;
                // break;
            }

            // Because the GA is lazy: e is positive or negative depending
            // on whether it's a keyup or keydown
            e = SDL_KEYDOWN == event.type ? 1 : -1;

           SDL_Keycode key = event.key.keysym.sym;
           
            if (key == SDLK_t)     // y axis up
                vCamera.y += 0.1f; // travel y axis upwards

            if (key == SDLK_g)     // y axis down
                vCamera.y -= 0.1f; // travel y axis downwards

            // following two inputs below move the camera left or right along the x-axis, useful if not turning the camera, confusing
            // otherwise
            if (key == SDLK_LEFT)
                vCamera.x -= 0.1f; // Travel left Along X-Axis
            if (key == SDLK_RIGHT)
                vCamera.x += 0.1f; // Travel right Along X-Axis

            // forward
			//moves along looking direction
            if (key == SDLK_w)
            {
                vec3d vForward = Vector_Mul(vLookDir, 0.1f);
                vCamera = Vector_Add(vCamera, vForward);
            }

			// moves backwards from looking direction
            if (key == SDLK_s) // backward
            {
                vec3d vBackward = Vector_Mul(vLookDir, 0.1f);
                vCamera = Vector_Sub(vCamera, vBackward);
            }

			//two below are from one lone coder's console engine (link in documentation)
            if (key == SDLK_a) // turn camera to the left
                fYaw += 0.1f;

            if (key == SDLK_d) // turn camera to the right
                fYaw -= 0.1f;
        }
        }
    }
    return 1;
}

//writes last frame time to screen using ttf and a blit
void DrawFPS(GameResources* res, SDL_Surface* display, uint32_t const frameTime[2])
{
    // Render FPS
    static char frameTimeString[128];

    if (frameTime[0] != 0 || frameTime[1] != 0)
    {
        static SDL_Color const colour = {255, 255, 255, 255};

        SDL_snprintf(frameTimeString, sizeof(frameTimeString), "Previous frame time: %ums",
                     frameTime[1] - frameTime[0]);

        SDL_Surface* fpsGauge = TTF_RenderText_Blended(res->font, frameTimeString, colour);

        SDL_BlitSurface(fpsGauge, nullptr, display, nullptr);

        SDL_FreeSurface(fpsGauge);
    }
}

//main rendering work done here
void GameLoop(void* const arg)
{
	//get resources
    GameResources* const res = static_cast<GameResources* const>(arg);

    static uint32_t prevFrameTime[2] = {};

    prevFrameTime[0] = prevFrameTime[1];
    prevFrameTime[1] = SDL_GetTicks();

    // get user input/window close
    if (ProcessEvent(res->windowID) == 0)
    {
        // FreeGameResources(res); //browser specific: it handles cleanup
        emscripten_cancel_main_loop();
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Exiting!\n");
        SDL_Quit();
        std::exit(0); // Reason for not plain returning: emscripten won't call
                      // global destructors when main or the main loop exits.
    }

    UpdateViews(matWorld); // matrix updates views

    // face the +z axis
    vec3d vTarget = {0, 0, 1};

    // Create "Point At" Matrix for camera using row major order matrices
    //  (camera is facing +z direction)
    matCameraRot = Matrix_MakeRotationY(fYaw);
    vLookDir = Matrix_MultiplyVector(matCameraRot, vTarget);
    vTarget = Vector_Add(vCamera, vLookDir);
	//now create the view to world matrix
    matCamera = Matrix_PointAt(vCamera, vTarget, vUp);

    // ivnerse the above to get world to view
    matView = Matrix_QuickInverse(matCamera);

    // Store triangles for rastering later
    vector<triangle> vecTrianglesToRaster;

    // Clear Screen
    for (int i = 0; i < ScreenHeight; ++i)
        for (int j = 0; j < ScreenWidth; j++)
            (*res->pixels)[i][j] = 0x00FF60FF;

    // Clear Depth Buffer
    for (int i = 0; i < ScreenWidth * ScreenHeight; i++)
        pDepthBuffer[i] = 0.0f;

	//texture index
    int texIdx = -1;
    Sprite* currSprite;
    // start traversing object list
    for (auto& obj : objects)
    {
        // get texture for this object
        if (hasTex)
        {
            texIdx++;
            currSprite = sprites.at(texIdx);
        }
        // transform triangles
        for (auto& tri : obj.tris)
        {

            triangle triProjected, triTransformed, triViewed;

            // World Matrix Transform vertices and texture vertices
            triTransformed.p[0] = Matrix_MultiplyVector(matWorld, tri.p[0]);
            triTransformed.p[1] = Matrix_MultiplyVector(matWorld, tri.p[1]);
            triTransformed.p[2] = Matrix_MultiplyVector(matWorld, tri.p[2]);
            triTransformed.t[0] = tri.t[0];
            triTransformed.t[1] = tri.t[1];
            triTransformed.t[2] = tri.t[2];

            // calculate triangle normal below
            vec3d normal, line1, line2;
            // Get lines on either side of the triangle
            line1 = Vector_Sub(triTransformed.p[1], triTransformed.p[0]);
            line2 = Vector_Sub(triTransformed.p[2], triTransformed.p[0]);
            // Take cross product of lines to get normal to triangle surface
            normal = Vector_CrossProduct(line1, line2);
            // normalize
            normal = Vector_Normalise(normal);

            // Get ray from triangle to camera
            vec3d vCameraRay = Vector_Sub(triTransformed.p[0], vCamera);

            // check if ray is aligned with normal; if so, then triangle is visible
            if (Vector_DotProduct(normal, vCameraRay)
				 < 0.0f) // side note: if I chose z as negative then this would be < 0
            {
                // Illumination (directional light)
				//arbitrary direction chosen
                vec3d light_direction = {0.0f, 1.0f, -1.0f};
                light_direction = Vector_Normalise(light_direction);

                // get how aligned the light direction and triangle surface normal
				//with 0.1 being the minimum illumination
                float dp = max(0.1f, Vector_DotProduct(light_direction, normal));

				//set new triangle color
                if (!hasTex)
                    triTransformed.col.newColorFromDirec(dp);

                // Convert World Space --> View Space
                triViewed.p[0] = Matrix_MultiplyVector(matView, triTransformed.p[0]);
                triViewed.p[1] = Matrix_MultiplyVector(matView, triTransformed.p[1]);
                triViewed.p[2] = Matrix_MultiplyVector(matView, triTransformed.p[2]);

                triViewed.col = triTransformed.col;
                triViewed.t[0] = triTransformed.t[0];
                triViewed.t[1] = triTransformed.t[1];
                triViewed.t[2] = triTransformed.t[2];

                // 3D clip: start with near plane first
                // clip viewed triangle against near plane; this could form two additional
                //	additional triangles.
                int nClippedTriangles = 0;
                triangle clipped[2];
                nClippedTriangles = Triangle_ClipAgainstPlane({0.0f, 0.0f, 0.1f}, {0.0f, 0.0f, 1.0f}, triViewed,
                                                              clipped[0], clipped[1]);

                // project each triangle from the clip
                for (int n = 0; n < nClippedTriangles; n++)
                {

                 // project triangles into clip space (perspective or orthographic)
                 // vertices first

					//perspective
                    if (!orthographic)
                    {
                        triProjected.p[0] = Matrix_MultiplyVector(matProj, clipped[n].p[0]);
                        triProjected.p[1] = Matrix_MultiplyVector(matProj, clipped[n].p[1]);
                        triProjected.p[2] = Matrix_MultiplyVector(matProj, clipped[n].p[2]);

                    }
					//oetho
                    else
                    {
              
                        triProjected.p[0] = Matrix_MultiplyVector(matOrtho, clipped[n].p[0]);
                        triProjected.p[1] = Matrix_MultiplyVector(matOrtho, clipped[n].p[1]);
                        triProjected.p[2] = Matrix_MultiplyVector(matOrtho, clipped[n].p[2]);

                    }
                    // keep color
                    triProjected.col = clipped[n].col;

                    // and tex values
                    triProjected.t[0] = clipped[n].t[0];
                    triProjected.t[1] = clipped[n].t[1];
                    triProjected.t[2] = clipped[n].t[2];

                    // perspective divide
                    if (!orthographic)
                    {

                        triProjected.t[0].u = triProjected.t[0].u / triProjected.p[0].w;
                        triProjected.t[1].u = triProjected.t[1].u / triProjected.p[1].w;
                        triProjected.t[2].u = triProjected.t[2].u / triProjected.p[2].w;

                        triProjected.t[0].v = triProjected.t[0].v / triProjected.p[0].w;
                        triProjected.t[1].v = triProjected.t[1].v / triProjected.p[1].w;
                        triProjected.t[2].v = triProjected.t[2].v / triProjected.p[2].w;

                        triProjected.t[0].w = 1.0f / triProjected.p[0].w;
                        triProjected.t[1].w = 1.0f / triProjected.p[1].w;
                        triProjected.t[2].w = 1.0f / triProjected.p[2].w;

                        // perspective divide to normalized device coordinates
                        triProjected.p[0] = Vector_Div(triProjected.p[0], triProjected.p[0].w);
                        triProjected.p[1] = Vector_Div(triProjected.p[1], triProjected.p[1].w);
                        triProjected.p[2] = Vector_Div(triProjected.p[2], triProjected.p[2].w);

                    }
                    // flip y back to normal
                    triProjected.p[0].y *= -1.0f;
                    triProjected.p[1].y *= -1.0f;
                    triProjected.p[2].y *= -1.0f;


                    //transform to screen space
                    triProjected.p[0].x = (triProjected.p[0].x * 0.5f * (float)ScreenWidth) + 0.5f * (float)ScreenWidth;
                    triProjected.p[0].y =
                        (triProjected.p[0].y * 0.5f * (float)ScreenHeight) + 0.5f * (float)ScreenHeight;
                    triProjected.p[1].x = (triProjected.p[1].x * 0.5f * (float)ScreenWidth) + 0.5f * (float)ScreenWidth;

                    triProjected.p[1].y =
                        (triProjected.p[1].y * 0.5f * (float)ScreenHeight) + 0.5f * (float)ScreenHeight;
                    triProjected.p[2].x = (triProjected.p[2].x * 0.5f * (float)ScreenWidth) + 0.5f * (float)ScreenWidth;
                    triProjected.p[2].y =
                        (triProjected.p[2].y * 0.5f * (float)ScreenHeight) + 0.5f * (float)ScreenHeight;

                    // Store triangle for sorting
                    vecTrianglesToRaster.push_back(triProjected);
                }
            }
     
            // Loop through all projected triangles that passed clipping
            for (auto& triToRaster : vecTrianglesToRaster)
            {

                // Clip triangles against all four screen edges, this could yield
                // a bunch of triangles, so create a queue that we traverse to
                //  ensure we only test new triangles generated against clipping planes
                triangle clipped[2];
                list<triangle> listTriangles;

                // Add initial triangle
                listTriangles.push_back(triToRaster);

                int nNewTriangles = 1;

                for (int p = 0; p < 4; p++)
                {
                    int nTrisToAdd = 0;
                    while (nNewTriangles > 0)
                    {
                        // Take triangle from front of queue
                        triangle test = listTriangles.front();
                        listTriangles.pop_front();
                        nNewTriangles--;

                        /*
						Sutherland hodgman clip the triangle and this could create an additional triangle,
						which will be added to the list we're iterating. The new triangle is guaranteed to be inside the plane
						it came from.
						*/
                        switch (p)
                        {
                        case 0:
                            nTrisToAdd = Triangle_ClipAgainstPlane({0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, test,
                                                                   clipped[0], clipped[1]);
                            break;
                        case 1:
                            nTrisToAdd = Triangle_ClipAgainstPlane({0.0f, (float)ScreenHeight - 1, 0.0f},
                                                                   {0.0f, -1.0f, 0.0f}, test, clipped[0], clipped[1]);
                            break;
                        case 2:
                            nTrisToAdd = Triangle_ClipAgainstPlane({0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, test,
                                                                   clipped[0], clipped[1]);
                            break;
                        case 3:
                            nTrisToAdd = Triangle_ClipAgainstPlane({(float)ScreenWidth - 1, 0.0f, 0.0f},
                                                                   {-1.0f, 0.0f, 0.0f}, test, clipped[0], clipped[1]);
                            break;
                        }

                        // Clipping may yield a variable number of triangles, so
                        // add these new ones to the back of the queue for subsequent
                        // clipping against next planes
                        for (int w = 0; w < nTrisToAdd; w++)
                            listTriangles.push_back(clipped[w]);
                    }
                    nNewTriangles = listTriangles.size();
                }
                // Draw the transformed, viewed, clipped, projected, sorted, clipped triangles
                for (auto& t :listTriangles)
                {

					
                    // remove any tiny triangles
                    float left = (t.p[1].x - t.p[0].x) * (t.p[2].y - t.p[0].y);
                    float right = (t.p[2].x - t.p[0].x) * (t.p[1].y - t.p[0].y);
                    if (abs(0.5f * (left - right)) < 0.1f)
                        continue;

					//draw wire model or
                    if (drawWireOnly)
                    {
                        DrawTriangle(t.p[0].x, t.p[0].y, t.p[1].x, t.p[1].y, t.p[2].x, t.p[2].y, res->pixels,
                                     0xFFFFFFFF);
                    }
                    // draw a textured triangle
                    else if (hasTex)
                    {
                        TexturedTriangle(t.p[0].x, t.p[0].y, t.t[0].u, t.t[0].v, t.t[0].w, t.p[1].x, t.p[1].y, t.t[1].u,
                                         t.t[1].v, t.t[1].w, t.p[2].x, t.p[2].y, t.t[2].u, t.t[2].v, t.t[2].w,
                                         currSprite, res->pixels);
                    }
					//or fill it normally
                    else
                    {
                        FillTriangle(t.p[0].x, t.p[0].y, t.p[1].x, t.p[1].y, t.p[2].x, t.p[2].y, res->pixels, t.col.n);
                    }
                }
            }
        }
    } //  cout << "frame done" << endl;

    DrawFPS(res, res->canvas, prevFrameTime);
    // Rasterization section
    SDL_UpdateTexture(res->texture, nullptr, res->canvas->pixels, res->canvas->pitch);
    // Now draw this directly to the window
    SDL_RenderCopy(res->renderer, res->texture, nullptr, nullptr);
    // And ask the windowing system to redraw the window
    SDL_RenderPresent(res->renderer);
}


//first program call
//initializes resources then calls game loop
int main(int, char* [])
{
    // VERY IMPORTANT: Ensure SDL2 is initialized
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS) < 0)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "could not initialize sdl2: %s\n", SDL_GetError());
        return 1;
    }
    // VERY IMPORTANT: if using text in your program, ensure SDL2_ttf library is
    // initialized
    if (TTF_Init() < 0)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "could not initialize SDL2_ttf: %s\n", TTF_GetError());
    }

	//resources holder
    GameResources res;

    // This creates the actual window in which graphics are displayed
    res.window =
        SDL_CreateWindow("CG Project | Controls: Q=Ortho/Persp; W/S = forward/backwards; A/D to turn "
                         "camera; Arrow keys for x/y absolute movement",
                         SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, ScreenWidth, ScreenHeight, SDL_WINDOW_SHOWN);

    if (res.window == nullptr)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not create window!\n");
        return 1;
    }

    res.windowID = SDL_GetWindowID(res.window);

    // create renderer
    res.renderer = SDL_CreateRenderer(res.window, -1,
                                      0); // let SDL2 choose best accel

    if (res.renderer == nullptr)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "could not create renderer: %s\n", SDL_GetError());
        return 1;
    }
    SDL_SetRenderDrawColor(res.renderer, 255, 255, 255, 255);

	//x/y screen resolution holder
    int xres{};
    int yres{};
    if (SDL_GetRendererOutputSize(res.renderer, &xres, &yres) < 0)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "could not get renderer output size: %s\n", SDL_GetError());
        return 1;
    }
    // create the canvas
    res.canvas = SDL_CreateRGBSurfaceWithFormat(0, xres, yres, 32, SDL_PIXELFORMAT_RGBA8888);
    if (res.canvas == nullptr)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "could not create surface: %s\n", SDL_GetError());
        return 1;
    }
    // create the texture
    res.texture = SDL_CreateTexture(res.renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, res.canvas->w,
                                    res.canvas->h);
    if (res.texture == nullptr)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "could not create texture: %s\n", SDL_GetError());
        return 1;
    }

    // for text
    res.font = TTF_OpenFont("../assets/iosevka-regular.ttf", 16);
    if (res.font == nullptr)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "TTF_OpenFont: %s\n", TTF_GetError());
        return 1;
    }

    // create pixels to work with
    res.pixels = (uint32_t(*)[ScreenHeight][ScreenWidth])res.canvas->pixels;

    // initialize depth buffer
    pDepthBuffer = new float[xres * yres];

 // Load object files if specified
    mesh obj1;
    mesh obj2;
    mesh obj3;
    if (useObjModel)
    {
		//loads then sets position
        obj1.LoadFromObjectFile("sword.obj");
        obj2.LoadFromObjectFile("cat2.obj");
        obj3.LoadFromObjectFile("wolf2.obj");
        obj1.translatePos({5, 0, 0, 0});
        obj2.translatePos({5, 0, 0.f, 0});
        obj3.translatePos({-1.5f, 0, -0.5f, 0});

		//add to objects list
        objects.push_back(obj1);
        objects.push_back(obj2);
        objects.push_back(obj3);

        // load texture (not recommended without the right textures
		//but the feature works)
		//object will be black if the picture doesnt exist
        if (hasTex)
        {
            Sprite* tex1 = new Sprite("sword.png"); // this doesnt exist.
            Sprite* tex2 = new Sprite("cat.png");   // this doesnt exist
            Sprite* tex3 = new Sprite("wolf.png");  // this doesnt exist either
            sprites.push_back(tex1);
            sprites.push_back(tex2);
            sprites.push_back(tex3);
        }
    }
	//instnatiate my own objects (cubes)
    else
    {
        obj1.tris = {

            // SOUTH
            {
                0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f,
                1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f,
            },
            {
                0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f,
                1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
            },

            // EAST
            {
                1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
                1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f,
            },
            {
                1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f,
                1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
            },

            // NORTH
            {
                1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
                1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f,
            },
            {
                1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
                1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
            },

            // WEST
            {
                0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f,
                1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f,
            },
            {
                0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
                1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
            },

            // TOP
            {
                0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
                1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f,
            },
            {
                0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
                1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
            },

            // BOTTOM
            {
                1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,
                1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f,
            },
            {
                1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f,
                1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
            },

        };
        mesh new_obj2 = obj1;
        mesh new_obj3 = obj1;
        mesh new_obj4 = obj1;
        new_obj2.translatePos({5, 3, -2});
        new_obj3.translatePos({0, 0, 2.f});
        new_obj4.translatePos({0, 0, -2.f});
       objects.push_back(obj1);
        objects.push_back(new_obj2);
       objects.push_back(new_obj3);
        objects.push_back(new_obj4);
        // load texture
        if (hasTex)
        {
            Sprite* sprTex1 = new Sprite("a.png");
            sprites.push_back(sprTex1);
            Sprite* sprTex2 = new Sprite("b.png");
            sprites.push_back(sprTex2);
            Sprite* sprTex3 = new Sprite("c.png");
            sprites.push_back(sprTex3);
            Sprite* sprTex4 = new Sprite("d.jpg");
            sprites.push_back(sprTex4);
        }
    }

    // Projection Matrix: fov = 90, near=0.1,far=1000
    matProj = Matrix_MakeProjection(90.0f, (float)yres / (float)xres, 0.1f, 1000.0f);
	//orthographic matrix, 
    matOrtho = Matrix_MakeOrthogonal(0.1f, 1000.0f, 10, 10 * ScreenHeight / ScreenWidth);

    if (SDL_SetSurfaceBlendMode(res.canvas, SDL_BLENDMODE_BLEND) < 0)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_SetSurfaceBlendMode: %s\n", SDL_GetError());
        return 1;
    }
	//start main loop
    emscripten_set_main_loop_arg(GameLoop, &res, 0, 1);

	 std::cin.get();
    return 0;
}

// Draws a textured triangle
/*strategy: order the triangles vertices so that y1 < y2 < y3
then calculate differences in height between edges so that we can draw
scanlines by iterating over the x-value between the edges and using a parametric equation to get
the values at that spot (and map it to the texture)
*/
void TexturedTriangle(int x1, int y1, float u1, float v1, float w1, int x2, int y2, float u2, float v2, float w2,
                      int x3, int y3, float u3, float v3, float w3, Sprite* tex,
                      uint32_t (*pixels)[ScreenHeight][ScreenWidth])
{
	//order vertices
    if (y2 < y1)
    {
        swap(y1, y2);
        swap(x1, x2);
        swap(u1, u2);
        swap(v1, v2);
        swap(w1, w2);
    }

    if (y3 < y1)
    {
        swap(y1, y3);
        swap(x1, x3);
        swap(u1, u3);
        swap(v1, v3);
        swap(w1, w3);
    }

    if (y3 < y2)
    {
        swap(y2, y3);
        swap(x2, x3);
        swap(u2, u3);
        swap(v2, v3);
        swap(w2, w3);
    }

	//get stats
    int dy1 = y2 - y1;
    int dx1 = x2 - x1;
    float dv1 = v2 - v1;
    float du1 = u2 - u1;
    float dw1 = w2 - w1;

    int dy2 = y3 - y1;
    int dx2 = x3 - x1;
    float dv2 = v3 - v1;
    float du2 = u3 - u1;
    float dw2 = w3 - w1;

    float tex_u, tex_v, tex_w;

	//where a/b are for the screen vertices and u/v are for texture coordinates. w is for depth buffer
    float dax_step = 0, dbx_step = 0, du1_step = 0, dv1_step = 0, du2_step = 0, dv2_step = 0, dw1_step = 0,
          dw2_step = 0;

	//get x/y ratios
    if (dy1)
        dax_step = dx1 / (float)abs(dy1);
    if (dy2)
        dbx_step = dx2 / (float)abs(dy2);

    if (dy1)
        du1_step = du1 / (float)abs(dy1);
    if (dy1)
        dv1_step = dv1 / (float)abs(dy1);
    if (dy1)
        dw1_step = dw1 / (float)abs(dy1);

    if (dy2)
        du2_step = du2 / (float)abs(dy2);
    if (dy2)
        dv2_step = dv2 / (float)abs(dy2);
    if (dy2)
        dw2_step = dw2 / (float)abs(dy2);

	//draw scanline if y value of two vertices is not equal
    if (dy1)
    {

		//iterate from the height of one vertice to the next
		//first derive a parametric equation for each statistic
		//and then iterate from the left to right
        for (int i = y1; i <= y2; i++)
        {
            int ax = x1 + (float)(i - y1) * dax_step;
            int bx = x1 + (float)(i - y1) * dbx_step;

            float tex_su = u1 + (float)(i - y1) * du1_step;
            float tex_sv = v1 + (float)(i - y1) * dv1_step;
            float tex_sw = w1 + (float)(i - y1) * dw1_step;

            float tex_eu = u1 + (float)(i - y1) * du2_step;
            float tex_ev = v1 + (float)(i - y1) * dv2_step;
            float tex_ew = w1 + (float)(i - y1) * dw2_step;

            if (ax > bx)
            {
                swap(ax, bx);
                swap(tex_su, tex_eu);
                swap(tex_sv, tex_ev);
                swap(tex_sw, tex_ew);
            }

            tex_u = tex_su;
            tex_v = tex_sv;
            tex_w = tex_sw;

            float tstep = 1.0f / ((float)(bx - ax));
            float t = 0.0f;

			// calculate texture mapping at each step of the scanline
            for (int j = ax; j < bx; j++)
            {
                tex_u = (1.0f - t) * tex_su + t * tex_eu;
                tex_v = (1.0f - t) * tex_sv + t * tex_ev;
                tex_w = (1.0f - t) * tex_sw + t * tex_ew;
                // z-buffering check
                if (tex_w > pDepthBuffer[i * ScreenWidth + j])
                {
					//draw from texture to cube
                    Draw(j, i, pixels, tex->Sample(tex_u / tex_w, tex_v / tex_w));
                    pDepthBuffer[i * ScreenWidth + j] = tex_w;
                }
                t += tstep;
            }
        }
    }

//now repeat like above but for next pair of vertices

    dy1 = y3 - y2;
    dx1 = x3 - x2;
    dv1 = v3 - v2;
    du1 = u3 - u2;
    dw1 = w3 - w2;

if (dy1)
        dax_step = dx1 / (float)abs(dy1);
    if (dy2)
        dbx_step = dx2 / (float)abs(dy2);

    du1_step = 0, dv1_step = 0;
    if (dy1)
        du1_step = du1 / (float)abs(dy1);
    if (dy1)
        dv1_step = dv1 / (float)abs(dy1);
    if (dy1)
        dw1_step = dw1 / (float)abs(dy1);

    if (dy1)
    {
        for (int i = y2; i <= y3; i++)
        {
            int ax = x2 + (float)(i - y2) * dax_step;
            int bx = x1 + (float)(i - y1) * dbx_step;

            float tex_su = u2 + (float)(i - y2) * du1_step;
            float tex_sv = v2 + (float)(i - y2) * dv1_step;
            float tex_sw = w2 + (float)(i - y2) * dw1_step;

            float tex_eu = u1 + (float)(i - y1) * du2_step;
            float tex_ev = v1 + (float)(i - y1) * dv2_step;
            float tex_ew = w1 + (float)(i - y1) * dw2_step;

            if (ax > bx)
            {
                swap(ax, bx);
                swap(tex_su, tex_eu);
                swap(tex_sv, tex_ev);
                swap(tex_sw, tex_ew);
            }

            tex_u = tex_su;
            tex_v = tex_sv;
            tex_w = tex_sw;

            float tstep = 1.0f / ((float)(bx - ax));
            float t = 0.0f;

            for (int j = ax; j < bx; j++)
            {
                tex_u = (1.0f - t) * tex_su + t * tex_eu;
                tex_v = (1.0f - t) * tex_sv + t * tex_ev;
                tex_w = (1.0f - t) * tex_sw + t * tex_ew;

                if (tex_w > pDepthBuffer[i * ScreenWidth + j])
                {
                    Draw(j, i, pixels, tex->Sample(tex_u / tex_w, tex_v / tex_w));
                    pDepthBuffer[i * ScreenWidth + j] = tex_w;
                }
                t += tstep;
            }
        }
    }
}

//sutherland hodgman clipping for triangles against a plane
/*strategy: we check the distance from the vertices to the plane and
check if the vertex is inside or outside the plane.
We count the number inside the plane and outside the plane.
If the number inside is 3 -> whole triangle accepted,
if number is 0 -> triangle rejected; 
1 inside -> then the triangle is just shrunk down
2 inside -> we split the triangle into two from the resulting quadrilateral
*/
int Triangle_ClipAgainstPlane(vec3d plane_p, vec3d plane_n, triangle& in_tri, triangle& out_tri1, triangle& out_tri2)
{
    // Make sure plane normal is indeed normal
    plane_n = Vector_Normalise(plane_n);

    // Return signed shortest distance from point to plane, plane normal must be normalised
    auto dist = [&](vec3d& p) {
        vec3d n = Vector_Normalise(p);
        return (plane_n.x * p.x + plane_n.y * p.y + plane_n.z * p.z - Vector_DotProduct(plane_n, plane_p));
    };

    // Create two temporary storage arrays to classify points either side of plane
    // If distance sign is positive, point lies on "inside" of plane
    vec3d* inside_points[3];
    int nInsidePointCount = 0;
    vec3d* outside_points[3];
    int nOutsidePointCount = 0;
    vec2d* inside_tex[3];
    int nInsideTexCount = 0;
    vec2d* outside_tex[3];
    int nOutsideTexCount = 0;

    // Get signed distance of each point in triangle to plane
    float d0 = dist(in_tri.p[0]);
    float d1 = dist(in_tri.p[1]);
    float d2 = dist(in_tri.p[2]);

    if (d0 >= 0)
    {
        inside_points[nInsidePointCount++] = &in_tri.p[0];
        inside_tex[nInsideTexCount++] = &in_tri.t[0];
    }
    else
    {
        outside_points[nOutsidePointCount++] = &in_tri.p[0];
        outside_tex[nOutsideTexCount++] = &in_tri.t[0];
    }
    if (d1 >= 0)
    {
        inside_points[nInsidePointCount++] = &in_tri.p[1];
        inside_tex[nInsideTexCount++] = &in_tri.t[1];
    }
    else
    {
        outside_points[nOutsidePointCount++] = &in_tri.p[1];
        outside_tex[nOutsideTexCount++] = &in_tri.t[1];
    }
    if (d2 >= 0)
    {
        inside_points[nInsidePointCount++] = &in_tri.p[2];
        inside_tex[nInsideTexCount++] = &in_tri.t[2];
    }
    else
    {
        outside_points[nOutsidePointCount++] = &in_tri.p[2];
        outside_tex[nOutsideTexCount++] = &in_tri.t[2];
    }

    // Now classify triangle points, and break the input triangle into
    // smaller output triangles if required. There are four possible
    // outcomes...

    if (nInsidePointCount == 0)
    {
        // All points lie on the outside of plane, so clip whole triangle
        // It ceases to exist

        return 0; // No returned triangles are valid
    }

    if (nInsidePointCount == 3)
    {
        // All points lie on the inside of plane, so do nothing
        // and allow the triangle to simply pass through
        out_tri1 = in_tri;

        return 1; // Just the one returned original triangle is valid
    }

    if (nInsidePointCount == 1 && nOutsidePointCount == 2)
    {
        // Triangle should be clipped. As two points lie outside
        // the plane, the triangle simply becomes a smaller triangle

        // Copy appearance info to new triangle
        out_tri1.col = in_tri.col;
        // out_tri1.sym = in_tri.sym;

        // The inside point is valid, so keep that...
        out_tri1.p[0] = *inside_points[0];
        out_tri1.t[0] = *inside_tex[0];

        // but the two new points are at the locations where the
        // original sides of the triangle (lines) intersect with the plane
		//so we will derive those using parametric equations with parameter t
        float t;
        out_tri1.p[1] = Vector_IntersectPlane(plane_p, plane_n, *inside_points[0], *outside_points[0], t);
        out_tri1.t[1].u = t * (outside_tex[0]->u - inside_tex[0]->u) + inside_tex[0]->u;
        out_tri1.t[1].v = t * (outside_tex[0]->v - inside_tex[0]->v) + inside_tex[0]->v;
        out_tri1.t[1].w = t * (outside_tex[0]->w - inside_tex[0]->w) + inside_tex[0]->w;

        out_tri1.p[2] = Vector_IntersectPlane(plane_p, plane_n, *inside_points[0], *outside_points[1], t);
        out_tri1.t[2].u = t * (outside_tex[1]->u - inside_tex[0]->u) + inside_tex[0]->u;
        out_tri1.t[2].v = t * (outside_tex[1]->v - inside_tex[0]->v) + inside_tex[0]->v;
        out_tri1.t[2].w = t * (outside_tex[1]->w - inside_tex[0]->w) + inside_tex[0]->w;

        return 1; // Return the newly formed single triangle
    }

    if (nInsidePointCount == 2 && nOutsidePointCount == 1)
    {
        // Triangle should be clipped. As two points lie inside the plane,
        // the clipped triangle becomes a "quad". Fortunately, we can
        // represent a quad with two new triangles

        // Copy appearance info to new triangles
        out_tri1.col = 0xFF0000FF; //  out_tri1.col = in_tri.col;
                                   //   out_tri1.sym = in_tri.sym;

        out_tri2.col = 0xFF0000FF; //   out_tri2.col = in_tri.col;
        // out_tri2.sym = in_tri.sym;

        // The first triangle consists of the two inside points and a new
        // point determined by the location where one side of the triangle
        // intersects with the plane
        out_tri1.p[0] = *inside_points[0];
        out_tri1.p[1] = *inside_points[1];
        out_tri1.t[0] = *inside_tex[0];
        out_tri1.t[1] = *inside_tex[1];

		//again use parametric equations to get the points of intersection
        float t;
        out_tri1.p[2] = Vector_IntersectPlane(plane_p, plane_n, *inside_points[0], *outside_points[0], t);
        out_tri1.t[2].u = t * (outside_tex[0]->u - inside_tex[0]->u) + inside_tex[0]->u;
        out_tri1.t[2].v = t * (outside_tex[0]->v - inside_tex[0]->v) + inside_tex[0]->v;
        out_tri1.t[2].w = t * (outside_tex[0]->w - inside_tex[0]->w) + inside_tex[0]->w;

        // The second triangle is composed of one of he inside points, a
        // new point determined by the intersection of the other side of the
        // triangle and the plane, and the newly created point above
        out_tri2.p[0] = *inside_points[1];
        out_tri2.t[0] = *inside_tex[1];
        out_tri2.p[1] = out_tri1.p[2];
        out_tri2.t[1] = out_tri1.t[2];
        out_tri2.p[2] = Vector_IntersectPlane(plane_p, plane_n, *inside_points[1], *outside_points[0], t);
        out_tri2.t[2].u = t * (outside_tex[0]->u - inside_tex[1]->u) + inside_tex[1]->u;
        out_tri2.t[2].v = t * (outside_tex[0]->v - inside_tex[1]->v) + inside_tex[1]->v;
        out_tri2.t[2].w = t * (outside_tex[0]->w - inside_tex[1]->w) + inside_tex[1]->w;
        return 2; // Return two newly formed triangles which form a quad
    }
}


/*from https://www.avrfreaks.net/sites/default/files/triangles.c
Fills a triangle (bresenham drawing of lines) by splitting it into a flat top and a flat bottom (if it's not already one of those base cases)
and then drawing scanlines from side to side
Strategy:	For a flat bottom triangle, y1-y2 = y1 - y3, and so we draw scanlines top to bottom
			a flat top triangle is the opposite so we draw from bottom to top
			If the triangle is neither, we split it along the long edge
*/
void FillTriangle(int x1, int y1, int x2, int y2, int x3, int y3, uint32_t (*pixels)[ScreenHeight][ScreenWidth],
                  uint32_t col)
{
	//define a simple swap lambda
    auto SWAP = [](int& x, int& y) {
        int t = x;
        x = y;
        y = t;
    };

	//create stats
    int t1x = 0, t2x = 0, y = 0, minx = 0, maxx = 0, t1xp = 0, t2xp = 0;
    bool changed1 = false;
    bool changed2 = false;
    int signx1 = 0, signx2 = 0, dx1 = 0, dy1 = 0, dx2 = 0, dy2 = 0;
    int e1 = 0, e2 = 0;
    // Sort vertices
    if (y1 > y2)
    {
        SWAP(y1, y2);
        SWAP(x1, x2);
    }
    if (y1 > y3)
    {
        SWAP(y1, y3);
        SWAP(x1, x3);
    }
    if (y2 > y3)
    {
        SWAP(y2, y3);
        SWAP(x2, x3);
    }

    t1x = t2x = x1;
    y = y1; // Starting points
    dx1 = (x2 - x1);
    if (dx1 < 0)
    {
        dx1 = -dx1;
        signx1 = -1;
    }
    else
        signx1 = 1;
    dy1 = (int)(y2 - y1);

    dx2 = (int)(x3 - x1);
    if (dx2 < 0)
    {
        dx2 = -dx2;
        signx2 = -1;
    }
    else
        signx2 = 1;
    dy2 = (int)(y3 - y1);

    if (dy1 > dx1)
    { // swap values
        SWAP(dx1, dy1);
        changed1 = true;
    }
    if (dy2 > dx2)
    { // swap values
        SWAP(dy2, dx2);
        changed2 = true;
    }

    e2 = (int)(dx2 >> 1);
    // Flat top, just process the second half
    if (y1 == y2)
        goto next;
    e1 = (int)(dx1 >> 1);

    for (int i = 0; i < dx1;)
    {
        t1xp = 0;
        t2xp = 0;
        if (t1x < t2x)
        {
            minx = t1x;
            maxx = t2x;
        }
        else
        {
            minx = t2x;
            maxx = t1x;
        }
        // process first line until y value is about to change
        while (i < dx1)
        {
            i++;
            e1 += dy1;
            while (e1 >= dx1)
            {
                e1 -= dx1;
                if (changed1)
                    t1xp = signx1; // t1x += signx1;
                else
                    goto next1;
            }
            if (changed1)
                break;
            else
                t1x += signx1;
        }
        // Move line
    next1:
        // process second line until y value is about to change
        while (1)
        {
            e2 += dy2;
            while (e2 >= dx2)
            {
                e2 -= dx2;
                if (changed2)
                    t2xp = signx2; // t2x += signx2;
                else
                    goto next2;
            }
            if (changed2)
                break;
            else
                t2x += signx2;
        }
    next2:
//clip
        if (minx > t1x)
            minx = t1x;
        if (minx > t2x)
            minx = t2x;
        if (maxx < t1x)
            maxx = t1x;
        if (maxx < t2x)
            maxx = t2x;
        for (int i = minx; i <= maxx; i++)
            Draw(i, y, pixels, col);
        // Now increase y
        if (!changed1)
            t1x += signx1;
        t1x += t1xp;
        if (!changed2)
            t2x += signx2;
        t2x += t2xp;
        y += 1;
        if (y == y2)
            break;
    }
next:
    // Second half
    dx1 = (int)(x3 - x2);
    if (dx1 < 0)
    {
        dx1 = -dx1;
        signx1 = -1;
    }
    else
        signx1 = 1;
    dy1 = (int)(y3 - y2);
    t1x = x2;

    if (dy1 > dx1)
    { // swap values
        SWAP(dy1, dx1);
        changed1 = true;
    }
    else
        changed1 = false;

    e1 = (int)(dx1 >> 1);

    for (int i = 0; i <= dx1; i++)
    {
        t1xp = 0;
        t2xp = 0;
        if (t1x < t2x)
        {
            minx = t1x;
            maxx = t2x;
        }
        else
        {
            minx = t2x;
            maxx = t1x;
        }
        // process first line until y value is about to change
        while (i < dx1)
        {
            e1 += dy1;
            while (e1 >= dx1)
            {
                e1 -= dx1;
                if (changed1)
                {
                    t1xp = signx1;
                    break;
                } // t1x += signx1;
                else
                    goto next3;
            }
            if (changed1)
                break;
            else
                t1x += signx1;
            if (i < dx1)
                i++;
        }
    next3:
        // process second line until y value is about to change
        while (t2x != x3)
        {
            e2 += dy2;
            while (e2 >= dx2)
            {
                e2 -= dx2;
                if (changed2)
                    t2xp = signx2;
                else
                    goto next4;
            }
            if (changed2)
                break;
            else
                t2x += signx2;
        }
    next4:

		//clip
        if (minx > t1x)
            minx = t1x;
        if (minx > t2x)
            minx = t2x;
        if (maxx < t1x)
            maxx = t1x;
        if (maxx < t2x)
            maxx = t2x;
        for (int i = minx; i <= maxx; i++)
            Draw(i, y, pixels, col);
        if (!changed1)
            t1x += signx1;
        t1x += t1xp;
        if (!changed2)
            t2x += signx2;
        t2x += t2xp;
        y += 1;
        if (y > y3)
            return;
    }
}
// draws at a point; TODO prob unnecessary
inline void Draw(int x, int y, uint32_t (*pixels)[ScreenHeight][ScreenWidth], uint32_t color)
{
    (*pixels)[y][x] = color;
}

// bresenham line draw algo from textbook adapted
void DrawLine(int x1, int y1, int x2, int y2, uint32_t (*pixels)[ScreenHeight][ScreenWidth], uint32_t col)
{
    // original
    int x, y, dx, dy, dx1, dy1, px, py, xe, ye, i;
    dx = x2 - x1;
    dy = y2 - y1;
    dx1 = abs(dx);
    dy1 = abs(dy);

    px = 2 * dy1 - dx1;
    py = 2 * dx1 - dy1;

    if (dy1 <= dx1)
    {
        if (dx >= 0)
        {
            x = x1;
            y = y1;
            xe = x2;
        }
        else
        {
            x = x2;
            y = y2;
            xe = x1;
        }

        Draw(x, y, pixels, col);

        for (i = 0; x < xe; i++)
        {
            x = x + 1;
            if (px < 0)
                px = px + 2 * dy1;
            else
            {
                if ((dx < 0 && dy < 0) || (dx > 0 && dy > 0))
                    y = y + 1;
                else
                    y = y - 1;
                px = px + 2 * (dy1 - dx1);
            }
            Draw(x, y, pixels, col);
        }
    }
    else
    {
        if (dy >= 0)
        {
            x = x1;
            y = y1;
            ye = y2;
        }
        else
        {
            x = x2;
            y = y2;
            ye = y1;
        }

        Draw(x, y, pixels, col);

        for (i = 0; y < ye; i++)
        {
            y = y + 1;
            if (py <= 0)
                py = py + 2 * dx1;
            else
            {
                if ((dx < 0 && dy < 0) || (dx > 0 && dy > 0))
                    x = x + 1;
                else
                    x = x - 1;
                py = py + 2 * (dx1 - dy1);
            }
            Draw(x, y, pixels, col);
        }
    }
}

//simply bresenham draws a line for each edge
void DrawTriangle(int x1, int y1, int x2, int y2, int x3, int y3, uint32_t (*pixels)[ScreenHeight][ScreenWidth],
                  uint32_t col)
{
    DrawLine(x1, y1, x2, y2, pixels, col);
    DrawLine(x2, y2, x3, y3, pixels, col);
    DrawLine(x3, y3, x1, y1, pixels, col);
}
