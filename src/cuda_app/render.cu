#include "render.hpp"
//#include "../app/colors.hpp"





void render(image_t const& dst, AppState& state)
{
    pixel_t p = {};
    p.alpha = 255;
    p.red = 255;
    p.green = 255;
    p.blue = 255;

    for(u32 y = 0; y < dst.height; ++y)
    {
        auto row = dst.data + y * dst.width;
        for(u32 x = 0; x < dst.width; ++x)
        {
            row[x] = p;
        }
    }
}