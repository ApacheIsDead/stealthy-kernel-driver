#include "aimbot.h"




void aimbot(float x, float y, const float& ScreenCenterX, const float& ScreenCenterY)
{
    float TargetX = 0;
    float TargetY = 0;

    if (x != 0)
    {
        if (x > ScreenCenterX)
        {
            TargetX = -(ScreenCenterX - x);
            //TargetX /= AimSpeed;
            if (TargetX + ScreenCenterX > ScreenCenterX * 2) TargetX = 0;
        }

        if (x < ScreenCenterX)
        {
            TargetX = x - ScreenCenterX;
            //TargetX /= AimSpeed;
            if (TargetX + ScreenCenterX < 0) TargetX = 0;
        }
    }
    if (y != 0)
    {
        if (y > ScreenCenterY)
        {
            TargetY = -(ScreenCenterY - y);
            //TargetY /= AimSpeed;
            if (TargetY + ScreenCenterY > ScreenCenterY * 2) TargetY = 0;
        }

        if (y < ScreenCenterY)
        {
            TargetY = y - ScreenCenterY;
            //TargetY /= AimSpeed;
            if (TargetY + ScreenCenterY < 0) TargetY = 0;
        }
    }

    INPUT input = { 0 };
    input.type = INPUT_MOUSE;
    input.mi.dx = static_cast<long>(TargetX);  // Relative movement on X-axis
    input.mi.dy = static_cast<long>(TargetY);  // Relative movement on Y-axis
    input.mi.dwFlags = MOUSEEVENTF_MOVE;  // Flag to specify mouse move event

    SendInput(1, &input, sizeof(INPUT));

    return;
}