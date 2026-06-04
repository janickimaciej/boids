#include "scene.hpp"
#include "window.hpp"

int main()
{
	Window window{};
	Scene scene{};
	window.init(scene);

	while (!window.shouldClose())
	{
		scene.update();
		scene.render();
		window.swapBuffers();
		window.pollEvents();
	}

	return 0;
}
