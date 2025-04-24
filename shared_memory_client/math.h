#pragma once
#include "struct.h"
#include "imgui/imgui.h"
#include <cmath>
#include <iostream>

bool w2scn(const vector3& world, vector3& screen, const viewMatrix& vm, ImVec2& scn);
float dist2D(const ImVec2& scnSize, const vector3& Hpos, float& x_c, float& y_c);

