#pragma once

#include "boids.hpp"

#include <glm/glm.hpp>

#include <chrono>

class Simulation
{
public:
	Simulation(const Boids& initialBoids);
	~Simulation();

	void update(Boids* boids);
	bool isRunning() const;
	void stop();
	void start();

	void decreaseVelocity();
	void increaseVelocity();
	void setCursorPos(const glm::vec2& pos);

private:
	bool m_running = false;
	float m_velocity = 0.1f;
	glm::vec2 m_cursorPos{};

	static constexpr float m_dT = 0.01f;
	std::chrono::time_point<std::chrono::system_clock> m_timeLastStart{};
	float m_timeSinceLastStart{};

	static constexpr float m_range = 0.05f;
	static const int m_spaceGridDim;
	int* m_spaceGridHeads{};
	int* m_spaceGridNodes{};
	float* m_newDir{};

	void allocateCudaMemory(const Boids& initialBoids);
	void freeCudaMemory();
	void step(Boids* boids);
	float getTimeSinceLastStart() const;
};
