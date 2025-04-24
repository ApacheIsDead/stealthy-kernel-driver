#pragma once

struct vector3
{
	float x, y, z;

	constexpr vector3()
		: x(), y(), z() {
	}

	constexpr vector3(float x, float y, float z)
		: x(x), y(y), z(z) {
	}

	constexpr vector3& operator+(const vector3& v)
	{
		x += v.x;
		y += v.y;
		z += v.z;
		return *this;
	}

	constexpr vector3& operator-(const vector3& v)
	{
		x -= v.x;
		y -= v.y;
		z -= v.z;
		return *this;
	}
};

struct viewMatrix
{
	float data[16];
};