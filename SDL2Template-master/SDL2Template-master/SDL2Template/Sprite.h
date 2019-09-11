#pragma once

/* This header file is in charge of creating a sprite from a picture loaded into a bitmap
    so that I can perform texture mapping

    Credits to Microsoft's forum for GDIPlus image loading to bitmap
    Code adapted from
   https://social.msdn.microsoft.com/Forums/vstudio/en-US/0a61f0f3-3aed-4bef-a18b-d2919cf78a09/how-to-load-images-bmp-jpeg-gif-tiff?forum=vclanguage
    and modified by me to include wide string characters
*/

// the following headers must be included in this order (windows.h first)
	//note: using edit to format the code changes the order it seems
#include <windows.h>
#include <codecvt>
#include <gdiplus.h>
#include <iostream>
#include <locale>
#include <objidl.h>
#include <string>
#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

// forward declare a color conversion func
uint32_t ColorConv(int r, int g, int b);

// class used to load images for texture mapping
class Sprite
{
    // constructors
  public:
    Sprite();
    Sprite(std::string sImageFile);
    Sprite(uint32_t w, uint32_t h);
    ~Sprite();

    // called by the constructor
    void LoadFromFile(std::string sImageFile);

    uint32_t width = 0;
    uint32_t height = 0;

    uint32_t GetPixel(uint32_t x, uint32_t y);

    uint32_t Sample(float x, float y);

    bool SetPixel(uint32_t x, uint32_t y, uint32_t col);

  private:
    // a bitmap representing our sprite
    uint32_t* pColData = nullptr;
};

Sprite::Sprite()
{
    pColData = nullptr;
    width = 0;
    height = 0;
}

// constructor called from main to load img
Sprite::Sprite(std::string sImageFile)
{
    // used to initialize GDIPlus
    // stolen from microsoft's GDIplus documentation

    Gdiplus::GdiplusStartupInput startupInput;
    ULONG_PTR token;
    Gdiplus::GdiplusStartup(&token, &startupInput, NULL);

    LoadFromFile(sImageFile);
}

Sprite::Sprite(uint32_t w, uint32_t h)
{
    if (pColData)
        delete[] pColData;
    width = w;
    height = h;
    pColData = new uint32_t[width * height];
    for (uint32_t i = 0; i < width * height; i++)
        pColData[i] = (uint32_t)0;
}

Sprite::~Sprite()
{
    if (pColData)
        delete pColData;
}

std::wstring ConvertS2W(std::string s)
{

    int count = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, NULL, 0);
    wchar_t* buffer = new wchar_t[count];
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, buffer, count);
    std::wstring w(buffer);
    delete[] buffer;
    return w;
}

//the bread and butter of this class
//it converts a pure png or jpg picture to
//an array of pixel data in memory
void Sprite::LoadFromFile(std::string sImageFile)
{

    // Use GDI+ (windows only)

    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::wstring wsImageFile = converter.from_bytes(sImageFile);

    Gdiplus::Bitmap* bmp = Gdiplus::Bitmap::FromFile(wsImageFile.c_str());
    if (bmp == nullptr) // problem here
        return;         // error

	//get stats
    width = bmp->GetWidth();
    height = bmp->GetHeight();
    pColData = new uint32_t[width * height];

	//fill the bitmap
    for (uint32_t x = 0; x < width; x++)
        for (uint32_t y = 0; y < height; y++)
        {
            Gdiplus::Color c;
            bmp->GetPixel(x, y, &c); //from GDIplus, gets the image pixel
            SetPixel(x, y, ColorConv(c.GetRed(), c.GetGreen(), c.GetBlue()));	//and set it in memory
        }
    delete bmp;	//clean up
}

//returns texture pixel at a location
uint32_t Sprite::GetPixel(uint32_t x, uint32_t y)
{
    if (x >= 0 && x < width && y >= 0 && y < height)
        return pColData[y * width + x];
    else
        return 0x000000FF; // black by default
}

//gets pixel from tex map info (x,y)
uint32_t Sprite::Sample(float x, float y)
{
    uint32_t sx = min((uint32_t)((x * (float)width)), width - 1);
    uint32_t sy = min((uint32_t)((y * (float)height)), height - 1);
    return GetPixel(sx, sy);
}

// from my assignment 3
// Converts three integer values corresponding to RGB components into a 32-bit RGBA value
// Technique: uses bit shifts so that the final value is 0xRRGGBBFF where FF is the alpha
uint32_t ColorConv(int r, int g, int b)
{
    // set to red initially
    uint32_t color = 0x000000FF & r;
    color = color << 8;
    // add the green value
    color = color | g;
    color = color << 8;
    // add blue value
    color = color | b;
    // now set alpha
    color = color << 8;
    // add alpha and now the color binary value is properly formatted
    color = color | 0x000000FF;
    return color;
} // end function

// writes to our texture in memory
bool Sprite::SetPixel(uint32_t x, uint32_t y, uint32_t col)
{
    if (x >= 0 && x < width && y >= 0 && y < height)
    {
        pColData[y * width + x] = col;
        return true;
    }
    else
        std::cout << "memory error" << std::endl;
    return false;
}