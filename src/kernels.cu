#include "kernels.cuh"

#include <algorithm>
#include <cmath>

static constexpr float pi = 3.14159265f;
static constexpr float legLength = 0.01f;
static constexpr float legAngleDeg = 15;
static constexpr float legAngleRad = legAngleDeg / 180 * pi;
static constexpr float FOVDeg = 300;
static constexpr float FOVRad = FOVDeg / 180 * pi;
static constexpr float maxTurningSpeed = 0.05f;

static __global__ void initializeSpaceGridKernel(int* spaceGridHeads, int spaceGridCount);
static __global__ void fillSpaceGridKernel(Boids* boids, int* spaceGridHeads, int* spaceGridNodes,
	int spaceGridDim, float range);
static __global__ void boidsDirectionKernel(Boids* boids, int* spaceGridHeads, int* spaceGridNodes,
	int spaceGridDim, float* newDir, float range, float cursorX, float cursorY);
static __global__ void boidsMoveKernel(Boids* boids, float velocity, float deltaTime,
	float* newDir);

static __device__ bool wallAvoidance(int tid, Boids* boids, float* newDir, float range);
static __device__ bool cursorAvoidance(int tid, Boids* boids, float* newDir, float range,
	float cursorX, float cursorY);
static __device__ void interaction(int tid, int spaceGridCellX, int spaceGridCellY, Boids* boids,
	int* spaceGridHeads, int* spaceGridNodes, float* newDir, int spaceGridDim, float range);
static __device__ bool areInteracting(Boids* boids, int receiver, int sender, float range);
static __device__ bool areInteracting(float receiverX, float receiverY, float receiverDir,
	float senderX, float senderY, float range);
static __device__ int getTID();
static __device__ void normalizeVector(float* x, float* y);
static __device__ float getNormalizedAngle(float angle);
static __device__ bool isDirLeft(float dir);
static __device__ bool isDirRight(float dir);
static __device__ bool isDirDown(float dir);
static __device__ bool isDirUp(float dir);

namespace Kernels
{
	void initializeSpaceGrid(int* spaceGridHeads, int spaceGridDim)
	{
		static constexpr int blockSize = 256;
		static const int numBlocks = (spaceGridDim * spaceGridDim + blockSize - 1) / blockSize;
		initializeSpaceGridKernel<<<numBlocks, blockSize>>>(spaceGridHeads,
			spaceGridDim * spaceGridDim);
	}

	void fillSpaceGrid(Boids* boids, int* spaceGridHeads, int* spaceGridNodes, int spaceGridDim,
		float range)
	{
		static constexpr int blockSize = 256;
		static constexpr int numBlocks = (boidCount + blockSize - 1) / blockSize;
		fillSpaceGridKernel<<<numBlocks, blockSize>>>(boids, spaceGridHeads, spaceGridNodes,
			spaceGridDim, range);
	}

	void boidsInteraction(Boids* boids, int* spaceGridHeads, int* spaceGridNodes, float* newDir,
		int spaceGridDim, float range, const glm::vec2& cursorPos)
	{
		static constexpr int blockSize = 256;
		static constexpr int numBlocks = (boidCount + blockSize - 1) / blockSize;
		boidsDirectionKernel<<<numBlocks, blockSize>>>(boids, spaceGridHeads, spaceGridNodes,
			spaceGridDim, newDir, range, cursorPos.x, cursorPos.y);
	}

	void boidsMove(Boids* boids, float deltaTime, float* newDir, float velocity)
	{
		static constexpr int blockSize = 256;
		static constexpr int numBlocks = (boidCount + blockSize - 1) / blockSize;
		boidsMoveKernel<<<numBlocks, blockSize>>>(boids, velocity, deltaTime, newDir);
	}
}

static __global__ void initializeSpaceGridKernel(int* spaceGridHeads, int spaceGridCount)
{
	int tid = getTID();
	if (tid >= spaceGridCount) return;

	spaceGridHeads[tid] = -1;
}

static __global__ void fillSpaceGridKernel(Boids* boids, int* spaceGridHeads, int* spaceGridNodes,
	int spaceGridDim, float range)
{
	int tid = getTID();
	if (tid >= boidCount) return;

	int spaceGridCellX = boids->x[tid] / range;
	int spaceGridCellY = boids->y[tid] / range;
	int spaceGridCell = spaceGridCellY * spaceGridDim + spaceGridCellX;

	int nextNode = ::atomicExch(spaceGridHeads + spaceGridCell, tid);
	spaceGridNodes[tid] = nextNode;
}

static __global__ void boidsDirectionKernel(Boids* boids, int* spaceGridHeads, int* spaceGridNodes,
	int spaceGridDim, float* newDir, float range, float cursorX, float cursorY)
{
	int tid = getTID();
	if (tid >= boidCount) return;

	if (wallAvoidance(tid, boids, newDir, range)) return;
	if (cursorAvoidance(tid, boids, newDir, range, cursorX, cursorY)) return;

	int spaceGridCellX = boids->x[tid] / range;
	int spaceGridCellY = boids->y[tid] / range;

	interaction(tid, spaceGridCellX, spaceGridCellY, boids, spaceGridHeads, spaceGridNodes, newDir,
		spaceGridDim, range);
}

static __global__ void boidsMoveKernel(Boids* boids, float velocity, float deltaTime, float* newDir)
{
	int tid = getTID();
	if (tid >= boidCount) return;

	float x = boids->x[tid];
	float y = boids->y[tid];
	float dir = newDir[tid];

	x = x + std::cos(dir) * velocity * deltaTime;
	y = y + std::sin(dir) * velocity * deltaTime;

	if (x < 0) x = 0;
	if (y < 0) y = 0;
	if (x > 1) x = 1;
	if (y > 1) y = 1;

	boids->x[tid] = x;
	boids->y[tid] = y;
	boids->dir[tid] = dir;

	boids->lx[tid] = x - legLength * std::cos(dir - legAngleRad);
	boids->ly[tid] = y - legLength * std::sin(dir - legAngleRad);
	boids->rx[tid] = x - legLength * std::cos(dir + legAngleRad);
	boids->ry[tid] = y - legLength * std::sin(dir + legAngleRad);
}

static __device__ bool wallAvoidance(int tid, Boids* boids, float* newDir, float range)
{
	static constexpr float marginDeg = 25;
	static constexpr float marginRad = marginDeg / 180 * pi;

	float x = boids->x[tid];
	float y = boids->y[tid];
	float dir = boids->dir[tid];

	if (x < range && (isDirLeft(dir - marginRad) || isDirLeft(dir + marginRad)))
	{
		newDir[tid] += isDirUp(dir) ? -maxTurningSpeed : maxTurningSpeed;
		return true;
	}
	if (x > 1 - range && (isDirRight(dir - marginRad) || isDirRight(dir + marginRad)))
	{
		newDir[tid] += isDirDown(dir) ? -maxTurningSpeed : maxTurningSpeed;
		return true;
	}
	if (y < range && (isDirDown(dir - marginRad) || isDirDown(dir + marginRad)))
	{
		newDir[tid] += isDirLeft(dir) ? -maxTurningSpeed : maxTurningSpeed;
		return true;
	}
	if (y > 1 - range && (isDirUp(dir - marginRad) || isDirUp(dir + marginRad)))
	{
		newDir[tid] += isDirRight(dir) ? -maxTurningSpeed : maxTurningSpeed;
		return true;
	}
	return false;
}

static __device__ bool cursorAvoidance(int tid, Boids* boids, float* newDir, float range,
	float cursorX, float cursorY)
{
	float x = boids->x[tid];
	float y = boids->y[tid];
	float dir = boids->dir[tid];

	if (!areInteracting(x, y, dir, cursorX, cursorY, range)) return false;

	float interactionDir = std::atan2(y - cursorY, x - cursorX);
	float dirDifference = getNormalizedAngle(interactionDir - boids->dir[tid]);
	float turn = ::min(::max(dirDifference, -maxTurningSpeed), maxTurningSpeed);
	newDir[tid] += turn;
	return true;
}

static __device__ void interaction(int tid, int spaceGridCellX, int spaceGridCellY, Boids* boids,
	int* spaceGridHeads, int* spaceGridNodes, float* newDir, int spaceGridDim, float range)
{
	float currX = boids->x[tid];
	float currY = boids->y[tid];
	float currDir = boids->dir[tid];

	static constexpr float separationWeight = 0.12f;
	static constexpr float alignmentWeight = 0.1f;
	static constexpr float cohesionWeight = 0.1f;
	static constexpr float currWeight = 1.0f - separationWeight - alignmentWeight - cohesionWeight;

	int counter = 0;
	float separationDirX = 0;
	float separationDirY = 0;
	float alignmentDirX = 0;
	float alignmentDirY = 0;
	float posSumX = 0;
	float posSumY = 0;

	for (int i = -1; i <= 1; ++i)
	{
		for (int j = -1; j <= 1; ++j)
		{
			if (spaceGridCellX + i < 0 || spaceGridCellX + i >= spaceGridDim ||
				spaceGridCellY + j < 0 || spaceGridCellY + j >= spaceGridDim) continue;
			int spaceGridCell = (spaceGridCellY + j) * spaceGridDim + spaceGridCellX + i;

			int node = spaceGridHeads[spaceGridCell];
			while (node != -1)
			{
				if (areInteracting(boids, tid, node, range))
				{
					float nodeX = boids->x[node];
					float nodeY = boids->y[node];
					float nodeDir = boids->dir[node];
					float distanceSquared =
						(nodeX - currX) * (nodeX - currX) + (nodeY - currY) * (nodeY - currY);
					static constexpr float eps = 1e-9f;
					float distanceSquaredInverse = 1.0f / (distanceSquared + eps);

					separationDirX += (currX - nodeX) * distanceSquaredInverse;
					separationDirY += (currY - nodeY) * distanceSquaredInverse;
					alignmentDirX += std::cos(nodeDir);
					alignmentDirY += std::sin(nodeDir);
					posSumX += nodeX;
					posSumY += nodeY;

					++counter;
				}
				node = spaceGridNodes[node];
			}
		}
	}

	if (counter == 0) return;

	normalizeVector(&separationDirX, &separationDirY);

	normalizeVector(&alignmentDirX, &alignmentDirY);

	float averageX = posSumX / counter;
	float averageY = posSumY / counter;
	float cohesionDirX = averageX - currX;
	float cohesionDirY = averageY - currY;
	normalizeVector(&cohesionDirX, &cohesionDirY);

	float currDirX = std::cos(currDir);
	float currDirY = std::sin(currDir);
	normalizeVector(&currDirX, &currDirY);

	float interactionDirX = separationWeight * separationDirX + alignmentWeight * alignmentDirX +
		cohesionWeight * cohesionDirX + currWeight * currDirX;
	float interactionDirY = separationWeight * separationDirY + alignmentWeight * alignmentDirY +
		cohesionWeight * cohesionDirY + currWeight * currDirY;
	float interactionDir = std::atan2(interactionDirY, interactionDirX);

	float dirDifference = getNormalizedAngle(interactionDir - boids->dir[tid]);
	float turn = ::min(::max(dirDifference, -maxTurningSpeed), maxTurningSpeed);
	newDir[tid] += turn;
}

static __device__ bool areInteracting(Boids* boids, int receiver, int sender, float range)
{
	if (receiver == sender) return false;

	return areInteracting(boids->x[receiver], boids->y[receiver], boids->dir[receiver],
		boids->x[sender], boids->y[sender], range);
}

static __device__ bool areInteracting(float receiverX, float receiverY, float receiverDir,
	float senderX, float senderY, float range)
{
	float dX = senderX - receiverX;
	float dY = senderY - receiverY;
	if (dX * dX + dY * dY > range * range) return false;

	float relativeAngle = std::atan2(dY, dX) - receiverDir;
	relativeAngle = getNormalizedAngle(relativeAngle);
	if (relativeAngle < -FOVRad / 2.0f || relativeAngle > FOVRad / 2.0f) return false;

	return true;
}

static __device__ int getTID()
{
	return blockIdx.x * blockDim.x + threadIdx.x;
}

static __device__ void normalizeVector(float* x, float* y)
{
	float norm = std::sqrt((*x) * (*x) + (*y) * (*y));
	*x /= norm;
	*y /= norm;
}

static __device__ float getNormalizedAngle(float angle)
{
	while (angle < -pi) angle += 2 * pi;
	while (angle >= pi) angle -= 2 * pi;
	return angle;
}

static __device__ bool isDirLeft(float dir)
{
	dir = getNormalizedAngle(dir);
	return dir >= pi / 2.0f || dir <= -pi / 2.0f;
}

static __device__ bool isDirRight(float dir)
{
	return !isDirLeft(dir);
}

static __device__ bool isDirDown(float dir)
{
	dir = getNormalizedAngle(dir);
	return dir < 0;
}

static __device__ bool isDirUp(float dir)
{
	return !isDirDown(dir);
}
