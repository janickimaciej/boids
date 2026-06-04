#include "window.hpp"

#include "shaderPrograms.hpp"

#include <string>

Window::Window()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	glfwWindowHint(GLFW_SAMPLES, 4);
	static const std::string windowTitle = "boids";
	m_windowPtr = glfwCreateWindow(m_initialSize.x, m_initialSize.y, windowTitle.c_str(), nullptr,
		nullptr);
	glfwSetWindowUserPointer(m_windowPtr, this);
	glfwMakeContextCurrent(m_windowPtr);
	glfwSwapInterval(1);

	glfwSetFramebufferSizeCallback(m_windowPtr,
		callbackWrapper<decltype(&Window::resizeCallback), &Window::resizeCallback>);
	glfwSetCursorPosCallback(m_windowPtr, callbackWrapper<decltype(&Window::cursorMovementCallback),
		&Window::cursorMovementCallback>);
	glfwSetKeyCallback(m_windowPtr, callbackWrapper<decltype(&Window::keyCallback),
		&Window::keyCallback>);

	gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress));

	updateViewport();
	ShaderPrograms::init();
}

Window::~Window()
{
	glfwTerminate();
}

void Window::init(Scene& scene)
{
	m_scene = &scene;
}

bool Window::shouldClose() const
{
	return glfwWindowShouldClose(m_windowPtr);
}

void Window::swapBuffers() const
{
	glfwSwapBuffers(m_windowPtr);
}

void Window::pollEvents() const
{
	glfwPollEvents();
}

const glm::ivec2 Window::m_initialSize{950, 950};

void Window::resizeCallback(int width, int height)
{
	if (width == 0 || height == 0)
	{
		return;
	}

	m_viewportSize = {width, height};
	updateViewport();
}

void Window::cursorMovementCallback(double x, double y)
{
	glm::vec2 pos{static_cast<float>(x), static_cast<float>(y)};
	glm::vec2 relativePos = pos / static_cast<glm::vec2>(m_initialSize);
	m_scene->setCursorPos({relativePos.x, 1.0f - relativePos.y});
}

void Window::keyCallback(int key, int, int action, int)
{
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
	{
		m_scene->togglePause();
    }
	if (key == GLFW_KEY_DOWN && action == GLFW_PRESS)
	{
		m_scene->decreaseVelocity();
	}
	if (key == GLFW_KEY_UP && action == GLFW_PRESS)
	{
		m_scene->increaseVelocity();
	}
}

void Window::updateViewport() const
{
	glViewport(0, 0, m_viewportSize.x, m_viewportSize.y);
}

glm::vec2 Window::getCursorPos() const
{
	double x{};
	double y{};
	glfwGetCursorPos(m_windowPtr, &x, &y);
	return {static_cast<float>(x), static_cast<float>(y)};
}

bool Window::isKeyPressed(int key)
{
	return glfwGetKey(m_windowPtr, key) == GLFW_PRESS;
}
