#include "scene.hpp"

#include "shaderPrograms.hpp"

#include <glm/gtc/constants.hpp>

#include <random>

Scene::Scene()
{
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_MULTISAMPLE);

	std::unique_ptr<Boids> initialBoids = std::make_unique<Boids>();
	std::vector<unsigned int> indices(3 * boidCount);
	initializeBoidsAndIndices(*initialBoids, indices);

	m_mesh = std::make_unique<Mesh>(*initialBoids, indices);
	m_mesh->registerVBOInCuda(m_cudaVBO);

	m_simulation = std::make_unique<Simulation>(*initialBoids);
}

void Scene::update()
{
	cudaGraphicsMapResources(1, &m_cudaVBO, 0);
	cudaGraphicsResourceGetMappedPointer(reinterpret_cast<void**>(&m_boids), nullptr, m_cudaVBO);
	m_simulation->update(m_boids);
	cudaGraphicsUnmapResources(1, &m_cudaVBO, 0);
}

void Scene::render()
{
	glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	ShaderPrograms::boid->use();
	m_mesh->render();
}

void Scene::togglePause()
{
	if (m_simulation->isRunning())
	{
		m_simulation->stop();
	}
	else
	{
		m_simulation->start();
	}
}

void Scene::decreaseVelocity()
{
	m_simulation->decreaseVelocity();
}

void Scene::increaseVelocity()
{
	m_simulation->increaseVelocity();
}

void Scene::setCursorPos(const glm::vec2& pos)
{
	m_simulation->setCursorPos(pos);
}

void Scene::initializeBoidsAndIndices(Boids& boids, std::vector<unsigned int>& indices)
{
	std::random_device randomDevice{};
	std::mt19937 randomGenerator{randomDevice()};
	std::uniform_real_distribution<float> posDistribution{0.0f, 1.0f};
	std::uniform_real_distribution<float> dirDistribution{-glm::pi<float>(), glm::pi<float>()};

	for (int i = 0; i < boidCount; ++i)
	{
		boids.dir[i] = dirDistribution(randomGenerator);
		boids.x[i] = posDistribution(randomGenerator);
		boids.y[i] = posDistribution(randomGenerator);
	}

	for (int i = 0; i < boidCount; ++i)
	{
		for (int j = 0; j < 3; ++j)
		{
			indices[3ll * i + j] = j * boidCount + i;
		}
	}
}
