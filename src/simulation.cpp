#include "simulation.hpp"

#include "kernels.cuh"

#include <cuda_runtime_api.h>

#include <algorithm>
#include <cmath>

static constexpr float velocityMultiplier = 1.2f;

Simulation::Simulation(const Boids& initialBoids)
{
	allocateCudaMemory(initialBoids);
	start();
}

Simulation::~Simulation()
{
	freeCudaMemory();
}

void Simulation::update(Boids* boids)
{
	if (!m_running) return;

	float timeSinceLastStart = getTimeSinceLastStart();
	while (m_timeSinceLastStart < timeSinceLastStart)
	{
		step(boids);
		m_timeSinceLastStart += m_dT;
	}
}

bool Simulation::isRunning() const
{
	return m_running;
}

void Simulation::stop()
{
	if (!isRunning()) return;

	m_running = false;
}

void Simulation::start()
{
	if (isRunning()) return;

	m_timeLastStart = std::chrono::system_clock::now();
	m_timeSinceLastStart = 0;
	m_running = true;
}

void Simulation::decreaseVelocity()
{
	m_velocity = std::max(m_velocity / velocityMultiplier, 0.001f);
}

void Simulation::increaseVelocity()
{
	m_velocity = std::min(m_velocity * velocityMultiplier, 0.25f);
}

void Simulation::setCursorPos(const glm::vec2& pos)
{
	m_cursorPos = pos;
}

const int Simulation::m_spaceGridDim = static_cast<int>(std::round(1.0f / m_range));

void Simulation::allocateCudaMemory(const Boids& initialBoids)
{
	cudaMalloc(reinterpret_cast<void**>(&m_spaceGridHeads),
		static_cast<unsigned long long>(m_spaceGridDim) * m_spaceGridDim * sizeof(int));
	cudaMalloc(reinterpret_cast<void**>(&m_spaceGridNodes), boidCount * sizeof(int));
	cudaMalloc(reinterpret_cast<void**>(&m_newDir), boidCount * sizeof(float));
	cudaMemcpy(m_newDir, initialBoids.dir, boidCount * sizeof(float), cudaMemcpyHostToDevice);
}

void Simulation::freeCudaMemory()
{
	cudaFree(m_spaceGridHeads);
	cudaFree(m_spaceGridNodes);
	cudaFree(m_newDir);
}

void Simulation::step(Boids* boids)
{
	Kernels::initializeSpaceGrid(m_spaceGridHeads, m_spaceGridDim);
	Kernels::fillSpaceGrid(boids, m_spaceGridHeads, m_spaceGridNodes, m_spaceGridDim, m_range);
	Kernels::boidsInteraction(boids, m_spaceGridHeads, m_spaceGridNodes, m_newDir, m_spaceGridDim,
		m_range, m_cursorPos);
	Kernels::boidsMove(boids, m_dT, m_newDir, m_velocity);
	cudaDeviceSynchronize();
}

float Simulation::getTimeSinceLastStart() const
{
	std::chrono::time_point<std::chrono::system_clock> t = std::chrono::system_clock::now();
	std::chrono::duration<float> simulationT = t - m_timeLastStart;
	return simulationT.count();
}
