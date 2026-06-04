#pragma once

inline constexpr unsigned int boidCount = 1000;

struct Boids
{
	float dir[boidCount];
	float x[boidCount];
	float lx[boidCount];
	float rx[boidCount];
	float y[boidCount];
	float ly[boidCount];
	float ry[boidCount];
};
