#ifndef JENJIN_CAMERA_H
#define JENJIN_CAMERA_H

#include "shader.h"

#include <gtc/matrix_transform.hpp>
#include <glm.hpp>

#include <GLFW/glfw3.h>

namespace Jenjin {
// A 2D camera that can be moved around and zoomed
// in and out.
class Camera {
private:
	// View matrix
	glm::mat4 m_view = glm::lookAt(position, position + cameraFront, cameraUp);

public:
	Camera();

	// Positions
	glm::vec3 position = glm::vec3(0.0f, 0.0f, 100.0f);

	// Camera vectors
	const glm::vec3 cameraUp;
	const glm::vec3 cameraRight;
	const glm::vec3 cameraFront;
	float zoom = 1.0f;

	// Camera settings
	const float cameraSpeed;

	// Functions for camera movement
	void processInput(GLFWwindow* window);
	void bind_uniforms(Shader& shader);
	void setup_proj(Shader& shader);
};
}

#endif // JENJIN_CAMERA_H
