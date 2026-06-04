#pragma once

#include "boids.hpp"
#include "mesh.hpp"
#include "simulation.hpp"

#include <glad/glad.h>
#include <cuda_gl_interop.h>
#include <glm/glm.hpp>

#include <memory>
#include <vector>

class Scene
{
public:
	Scene();

	void update();
	void render();
	void togglePause();
	void decreaseVelocity();
	void increaseVelocity();
	void setCursorPos(const glm::vec2& pos);

private:
	cudaGraphicsResource_t m_cudaVBO{};
	Boids* m_boids{};
	std::unique_ptr<Mesh> m_mesh{};
	std::unique_ptr<Simulation> m_simulation{};

	void initializeBoidsAndIndices(Boids& boids, std::vector<unsigned int>& indices);
};
