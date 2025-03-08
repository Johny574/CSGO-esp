#pragma once
#include <numbers>
#include <cmath>

extern float screenWidth;
extern float screenHeight;

struct ViewMatrix
{
	float matrix[4][4];

	float* operator[](int index)
	{
		return matrix[index];
	}
};


struct Vector3
{
	constexpr Vector3(
	const float x = 0,
	const float y = 0,
	const float z = 0) noexcept:
	x(x), y(y), z(z) { }

	constexpr const Vector3& operator- (const Vector3& other)
	{
		return Vector3{ x - other.x, y - other.y, z - other.z };
	}

	constexpr const Vector3& operator+ (const Vector3& other)
	{
		return Vector3{ x + other.x, y + other.y, z + other.z };
	}

	constexpr const Vector3& operator/ (const Vector3& other)
	{
		return Vector3{ x / other.x, y / other.y, z / other.z };
	}

	constexpr const Vector3& operator* (const Vector3& other)
	{
		return Vector3{ x * other.x, y * other.y, z * other.z };
	}

	Vector3 WorldToScreen(ViewMatrix matrix)
	{
		float _x = matrix[0][0] * x + matrix[0][1] * y + matrix[0][2] * z + matrix[0][3];
		float _y = matrix[1][0] * x + matrix[1][1] * y + matrix[1][2] * z + matrix[1][3];
		float _w = matrix[3][0] * x + matrix[3][1] * y + matrix[3][2] * z + matrix[3][3];

		if (_w < 0.01f) {
			return false;
		}

		float inv_w = 1 / _w;
		_x *= inv_w;
		_y *= inv_w;

		float x = screenWidth / 2;
		float y = screenHeight / 2;

		x += .5f * _x * screenWidth + .5f;
		y -= .5f * _y * screenHeight + .5f;
		return { x, y, _w };
	};

	float x, y, z;
};
