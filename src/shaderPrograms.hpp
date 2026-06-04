#pragma once

#include "shaderProgram.hpp"

#include <memory>

namespace ShaderPrograms
{
	void init();

	extern std::unique_ptr<const ShaderProgram> boid;
}
