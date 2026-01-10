#include "camera.h"

Camera::Camera(glm::vec3 cameraPosition)
{
	this->cameraPosition = cameraPosition;
	// Look toward center/back of cell - straight ahead, slightly down
	this->cameraViewDirection = glm::normalize(glm::vec3(0.0f, -0.1f, -1.0f));
	this->cameraViewDirection1 = glm::vec3(-1.0f, 0.0f, 0.0f);
	this->cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
	this->cameraRight = glm::normalize(glm::cross(cameraViewDirection, cameraUp));
	this->rotationOx = 0.0f;
	this->rotationOy = -90.0f;
}

Camera::Camera()
{
	this->cameraPosition = glm::vec3(0.0f, 0.0f, 100.0f);
	this->cameraViewDirection = glm::vec3(0.0f, 0.0f, -1.0f);
	this->cameraViewDirection1 = glm::vec3(-1.0f, 0.0f, 0.0f);
	this->cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
	this->cameraRight = glm::cross(cameraViewDirection, cameraUp);
	this->rotationOx = 0.0f;
	this->rotationOy = -90.0f;
}

Camera::Camera(glm::vec3 cameraPosition, glm::vec3 cameraViewDirection, glm::vec3 cameraUp)
{
	this->cameraPosition = cameraPosition;
	this->cameraViewDirection = cameraViewDirection;
	this->cameraUp = cameraUp;
	this->cameraRight = glm::cross(cameraViewDirection, cameraUp);
}

Camera::~Camera()
{
}

void Camera::keyboardMoveFront(float cameraSpeed)
{
	cameraPosition += cameraViewDirection * cameraSpeed;
}

void Camera::keyboardMoveBack(float cameraSpeed)
{
	cameraPosition -= cameraViewDirection * cameraSpeed;
}

void Camera::keyboardMoveLeft(float cameraSpeed)
{
	// Move left by moving in the negative right direction
	cameraPosition -= cameraRight * cameraSpeed;
}

void Camera::keyboardMoveRight(float cameraSpeed)
{
	// Move right by moving in the positive right direction
	cameraPosition += cameraRight * cameraSpeed;
}

void Camera::keyboardMoveUp(float cameraSpeed)
{
	cameraPosition += cameraUp * cameraSpeed;
}

void Camera::keyboardMoveDown(float cameraSpeed)
{
	cameraPosition -= cameraUp * cameraSpeed;
}

void Camera::rotateOx(float angle)
{
	cameraViewDirection = glm::normalize(glm::vec3((glm::rotate(glm::mat4(1.0f), angle, cameraRight) * glm::vec4(cameraViewDirection, 1))));
	cameraUp = glm::normalize(glm::cross(cameraRight, cameraViewDirection));
	cameraRight = glm::cross(cameraViewDirection, cameraUp);
}

void Camera::rotateOy(float angle)
{
	// Create rotation matrix around Y axis
	glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f));

	// Rotate the view direction
	cameraViewDirection = glm::normalize(glm::vec3(rotationMatrix * glm::vec4(cameraViewDirection, 0.0f)));

	cameraRight = glm::normalize(glm::cross(cameraViewDirection, cameraUp));

	cameraUp = glm::normalize(glm::cross(cameraRight, cameraViewDirection));
}

glm::mat4 Camera::getViewMatrix()
{
	return glm::lookAt(cameraPosition, cameraPosition + cameraViewDirection, cameraUp);
}

glm::vec3 Camera::getCameraPosition()
{
	return cameraPosition;
}

glm::vec3 Camera::getCameraViewDirection()
{
	return cameraViewDirection;
}
glm::vec3 Camera::getCameraViewDirection1()
{
	return cameraViewDirection1;
}


glm::vec3 Camera::getCameraUp()
{
	return cameraUp;
}


