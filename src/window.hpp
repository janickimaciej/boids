#pragma once

#include "scene.hpp"

#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <glm/glm.hpp>

class Window
{
public:
	Window();
	~Window();

	void init(Scene& scene);
	bool shouldClose() const;
	void swapBuffers() const;
	void pollEvents() const;

private:
	static const glm::ivec2 m_size;

	GLFWwindow* m_windowPtr{};
	Scene* m_scene{};

	void cursorMovementCallback(double x, double y);
	void keyCallback(int key, int, int action, int);

	template <typename Callback, Callback callback, typename... Args>
	static void callbackWrapper(GLFWwindow* windowPtr, Args... args);
};

template <typename Callback, Callback callback, typename... Args>
void Window::callbackWrapper(GLFWwindow* windowPtr, Args... args)
{
	Window* window = static_cast<Window*>(glfwGetWindowUserPointer(windowPtr));
	(window->*callback)(args...);
}
