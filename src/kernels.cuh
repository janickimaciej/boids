#pragma once

#include <glm/glm.hpp>

#include "boids.hpp"

namespace Kernels
{
	void initializeSpaceGrid(int* spaceGridHeads, int spaceGridDim);
	void fillSpaceGrid(Boids* boids, int* spaceGridHeads, int* spaceGridNodes, int spaceGridDim,
		float range);
	void boidsInteraction(Boids* boids, int* spaceGridHeads, int* spaceGridNodes, float* newDir,
		int spaceGridDim, float range, const glm::vec2& cursorPos);
	void boidsMove(Boids* boids, float deltaTime, float* newDir, float velocity);
}
