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
	static const glm::ivec2 m_initialSize;

	GLFWwindow* m_windowPtr{};
	glm::ivec2 m_viewportSize = m_initialSize;
	Scene* m_scene{};

	void resizeCallback(int width, int height);
	void cursorMovementCallback(double x, double y);
	void keyCallback(int key, int, int action, int);

	void updateViewport() const;
	glm::vec2 getCursorPos() const;
	bool isKeyPressed(int key);

	template <typename Callback, Callback callback, typename... Args>
	static void callbackWrapper(GLFWwindow* windowPtr, Args... args);
};

template <typename Callback, Callback callback, typename... Args>
void Window::callbackWrapper(GLFWwindow* windowPtr, Args... args)
{
	Window* window = static_cast<Window*>(glfwGetWindowUserPointer(windowPtr));
	(window->*callback)(args...);
}
