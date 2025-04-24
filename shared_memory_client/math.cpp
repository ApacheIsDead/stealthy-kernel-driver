#include "math.h"



bool w2scn(const vector3& world, vector3& screen, const viewMatrix& vm, ImVec2& scn)
{

    float w = world.x * vm.data[3] + world.y * vm.data[7] + world.z * vm.data[11] + vm.data[15];

    if (w < 0.001f)
    {
        return false;
    }

    float x = world.x * vm.data[0] + world.y * vm.data[4] + world.z * vm.data[8] + vm.data[12];
    float y = world.x * vm.data[1] + world.y * vm.data[5] + world.z * vm.data[9] + vm.data[13];

    float nx = x / w;
    float ny = y / w;


    screen.x = (scn.x * 0.5f * nx) + (scn.x * 0.5f + nx);
    screen.y = -(scn.y * 0.5f * ny) + (scn.y * 0.5f + ny);

    return true;
}

float dist2D(const ImVec2& scnSize, const vector3& Hpos, float& x_c, float& y_c)
{
    x_c = scnSize.x / 2;
    y_c = scnSize.y / 2;
    return static_cast<float>(sqrt(pow((x_c - Hpos.x), 2) + pow((y_c - Hpos.y), 2)));
}