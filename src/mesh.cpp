#include "mesh.hpp"

Mesh::Mesh(const Boids& boids, const std::vector<unsigned int>& indices)
{
	createVBO(boids);
	createEBO(indices);
	createVAO();
}

Mesh::~Mesh()
{
	glDeleteVertexArrays(1, &m_VAO);
	glDeleteBuffers(1, &m_EBO);
	glDeleteBuffers(1, &m_VBO);
}

void Mesh::render() const
{
	glBindVertexArray(m_VAO);
	glDrawElements(GL_TRIANGLES, 3 * static_cast<GLsizei>(boidCount), GL_UNSIGNED_INT, nullptr);
	glBindVertexArray(0);
}

void Mesh::registerVBOInCuda(cudaGraphicsResource_t& cudaBuffer) const
{
	cudaGraphicsGLRegisterBuffer(&cudaBuffer, m_VBO, cudaGraphicsMapFlagsNone);
}

void Mesh::createVBO(const Boids& boids)
{
	glGenBuffers(1, &m_VBO);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Boids), &boids, GL_DYNAMIC_DRAW);
}

void Mesh::createEBO(const std::vector<unsigned int>& indices)
{
	glGenBuffers(1, &m_EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
		static_cast<GLsizeiptr>(3ull * boidCount * sizeof(unsigned int)), indices.data(),
		GL_STATIC_DRAW);
}

void Mesh::createVAO()
{
	glGenVertexArrays(1, &m_VAO);

	glBindVertexArray(m_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
	glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, sizeof(float),
		reinterpret_cast<void*>(boidCount * sizeof(float)));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(float),
		reinterpret_cast<void*>(4ull * boidCount * sizeof(float)));
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);

	glBindVertexArray(0);
}
