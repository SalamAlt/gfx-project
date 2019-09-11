COMP-3520 Computer Graphics Winter 2019 Project
Group Members: Salam Al-Tamimi (solo)
Student ID: 104 066 489

Note: I included screenshots just in case.

Summary
I created a scene that shows either .obj models or simply cubes whose positions I wrote in code. There is an option to use textures, the camera is movable, and there is directional light.
Note: the FPS is really low (1/second) when using the .obj models provided.

Goals Met
Orthographic/perspective camera and:
 Different Objects
　　　o 3D Objects: Polyhedral ? Medium to High
-each object is formed of a mesh of triangles and can be loaded in from waveform (.obj) files 

Rendering 
　　　o Basic drawing of objects ? Low to Medium to High 
　　　	-My program can draw wireframe models, fill each triangle face with a color,
　　　	or map a texture to the object.
　　　o Surface rendering 
　　　
Clipping 
o 3D Clipping ? Medium to High 
-My program clips in the near plane in the view space and then when in screen space it clips each triangle against the screen edges using a variation of sutherland hodgman for triangles.

Hidden Surface Removal and Visibility 
o Z-buffer ? Low to Medium 
-A depth buffer is stored and used only for when textures are drawn since I didn’t get around to adding it for simple filling or wireframe draws
o Backface ? Low to Medium 
-A simple plane inside/outside test is done to dismiss triangles facing away from the camera.

Lighting 
o Light source(s) ? Medium to High 
	-A directional light is used and works well with onjects with a high triangle count.
	This is not applied when texture mapping is used.
o Simple ambient ? Low 
-I’m not sure if this counts but the directional light makes sure objects have a minimum intensity.
o Moving POV/camera(s) ? Medium to High 
-I have a camera that can move forward, backwards, move in the +/- x or y direction, and turn left or right.


Execution
Compile using visual studio 2017 on windows OS (since I used gdiPlus for image loading), and if there is an error during compilation, please click rebuild SDLTemplate.
At the top of the main.cpp file, there are boolean switches used to draw only wireframe models, or with textures. If both are false, then the objects will be colored with a default color. There is also an option to load obj files (that I found for free on the internet (all sources will be listed in another section) for noncommercial reuse) and if false, cubes will be shown instead.
Note: I couldn’t find any textures for my models so I do not recommend setting that with .obj files or else objects are drawn black. But it is functional and works as shown when used with “useObjModel = false;”.

Controls
Q to switch between orthographic and perspective.
W/S moves the camera forwards/backwards
A/D turn the camera left/right.
T/G to move up/down in y direction.
Arrow keys left/right to move left/right in x direction.

Program Structure
Drawing is done by writing to a framebuffer and presenting it to the screen. Objects are stored as meshes outlined in the PolygonObject.h header. Sprites are stored/loaded using the Sprite.h header using textures in the current working directory, and they’re loaded using the GDIplus library. Matrix operations are performed using functions in Matrices.h, and vector operations are done using ones from VectorsMath.h. The main.cpp file calls upon all of these.

Program Flow
The program initializes all resources in main and then calls the main gameloop (I used the tutorial SDL2Template and modified it). Then, the framebuffer and depth buffer are reset. Space transformation matrices are then created. Each triangle of each polygon is transformed to view space, clipped in the near plane, backface culled, illuminated, transformed to screen space and clipped on the screen edges using a variation of Sutherland Hodgman for triangles. New triangles are (possibly) added during the clipping process. Then all valid triangles are drawn and the screen is refreshed. 
The algorithms are described in detail in the code.

Issues
I had general issues with C++ trying to layout my headers and classes and linking libraries like GDIplus. 3D clipping was very difficult since I was trying to use Cohen Sutherland but things werent working out so I switched to Sutherland Hodgman. I spent a lot of time trying to figure out texture shading and moving the camera any direction with the mouse, but I couldn't manage it on time.

Sources
For the 3D clipping, texture loading and mapping: One Lone Coder’s console game engine that I adapted to use pixels and SDL2: https://github.com/OneLoneCoder/videos/blob/master/OneLoneCoder_olcEngine3D_Part4.cpp
The triangle fill method is from https://www.avrfreaks.net/sites/default/files/triangles.c adapted to C++.
scratchapixel.com for a lot of theory and math references.
And the SDL2Template was used as a starting point.
