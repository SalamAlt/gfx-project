#pragma once

/*
This header is the representation of the polygons/objects and their structure
Contains: Pixels, Triangles, and meshes made up of triangles.
*/

#include <algorithm>
#include <fstream>
#include <iostream>
#include <list>
#include <string>
#include <strstream>
#include <vector>

#include "VectorsMath.h"

// memory layout should be RGBA and can be referred to as n since it's a union
struct color
{
    union {
        uint32_t n;
        struct
        {
            uint8_t r;
            uint8_t g;
            uint8_t b;
            uint8_t a;
        };
    };

    // def cosntructor
    color() : r(0), g(0), b(0), a(255) {}
    color(uint32_t newCol) : n() { n = newCol; }
    color(uint8_t red, uint8_t green, uint8_t blue)
    {
        r = red;
        g = green;
        b = blue;

    } // end function

	//for directional light, multiply each color by the intensity
    void newColorFromDirec(float lum)
    {
        r = r * lum;
        g = g * lum;
        b = b * lum;
        a = 0xFF;
    }

    ~color() {}
};

/*Our triangles have 3 vertices per standard,
3 texture coordinates, a color for the face, and an intensity 
to do: implement texture shading or delete the lum since it's un-used
*/

struct triangle
{
    vec3d p[3];
    vec2d t[3];            // added a texture coord per vertex
    color col{0x00F0F0FF}; // color of triangle: not used for texture shading
    float lum;             // for texture shading: each triangle carries info about the directional light hitting it
};

struct mesh
{
    std::vector<triangle> tris;

	//translates the object, used for initialization
	void translatePos(vec3d shift) {
		for (auto& tri : tris)
		{
            for (int i = 0; i < 3; ++i)
            {         
            tri.p[i].x += shift.x;
            tri.p[i].y += shift.y;
            tri.p[i].z += shift.z;
		}}
	}

    //this method constructs the mesh from an obj file
    bool LoadFromObjectFile(std::string sFilename, bool bHasTexture = true)
    {
        using namespace std;

		//open file
        ifstream f(sFilename);
        if (!f.is_open())
            return false;

        // Local cache of verts
        vector<vec3d> verts;
        vector<vec2d> texs;

		//read file
        while (!f.eof())
        {
			//buffer
            char line[128];
            f.getline(line, 128);
            strstream s;
            s << line;

            char junk;

			//reads lines starting with v, or vt
            if (line[0] == 'v')
            {
				//texture coordinates
                if (line[1] == 't')
                {
                    vec2d v;
                    s >> junk >> junk >> v.u >> v.v;
                    // A little hack for the spyro texture
                    // v.u = 1.0f - v.u;
                    // v.v = 1.0f - v.v;
                    texs.push_back(v);
                }
                else
                {
					//get vertice info
                    vec3d v;
                    s >> junk >> v.x >> v.y >> v.z;
                    verts.push_back(v);
                }
            }

			//below lines all read face info but with different formats
			//depending on if a texture is used or not
            if (!bHasTexture)
            {

                if (line[0] == 'f')
                {
                    int f[3];
                    s >> junk >> f[0] >> f[1] >> f[2];
                    tris.push_back({verts[f[0] - 1], verts[f[1] - 1], verts[f[2] - 1]});
                }
            }
            else
            {
                if (line[0] == 'f')
                {
                    s >> junk;

                    string tokens[6];
                    int nTokenCount = -1;

                    while (!s.eof())
                    {
                        char c = s.get();

                        if (c == ' ' || c == '/')
                            nTokenCount++;
                        else
                            tokens[nTokenCount].append(1, c);
                    }

                    tokens[nTokenCount].pop_back();

					//add the new triangle
                    tris.push_back({verts[stoi(tokens[0]) - 1], verts[stoi(tokens[2]) - 1], verts[stoi(tokens[4]) - 1],
                                    texs[stoi(tokens[1]) - 1], texs[stoi(tokens[3]) - 1], texs[stoi(tokens[5]) - 1]});
                }
            }
        }
        return true;
    }

	mesh(mesh const& other) : tris(other.tris) {}
    mesh():tris(){}

};