#pragma once

#include "boids.hpp"

#include <glad/glad.h>
#include <cuda_gl_interop.h>

#include <vector>

class Mesh
{
public:
	Mesh(const Boids& boids, const std::vector<unsigned int>& indices);
	~Mesh();

	void render() const;
	void registerVBOInCuda(cudaGraphicsResource_t& cudaBuffer) const;

private:
	unsigned int m_VBO{};
	unsigned int m_EBO{};
	unsigned int m_VAO{};

	void createVBO(const Boids& boids);
	void createEBO(const std::vector<unsigned int>& indices);
	void createVAO();
};
