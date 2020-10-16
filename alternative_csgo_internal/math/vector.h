#include "../includes.h"

class Vector
{
public:
	Vector(void) {};
	Vector(float *p) : x(p[0]), y(p[1]), z(p[2]) {};
	Vector(float x, float y, float z) : x(x), y(y), z(z) {};
	float x, y, z;
};