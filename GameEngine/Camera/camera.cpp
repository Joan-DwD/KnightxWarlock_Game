#include "camera.h"

Camera::Camera(glm::vec3 cameraPosition)
{
	this->cameraPosition = cameraPosition;
	rotationOy = 180.0f;
	updateVectors();
}


Camera::Camera()
{
	cameraPosition = glm::vec3(0.0f, 0.0f, 100.0f);
	rotationOy = 180.0f; // look toward -Z
	updateVectors();
}


Camera::Camera(glm::vec3 cameraPosition, glm::vec3, glm::vec3)
{
	this->cameraPosition = cameraPosition;
	rotationOy = 180.0f;
	updateVectors();
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
	rotationOy += angle;
	updateVectors();
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


glm::vec3 Camera::getCameraUp()
{
	return cameraUp;
}

void Camera::setCameraPosition(const glm::vec3& newPos)
{
	cameraPosition = newPos;
}

float Camera::getYaw() const
{
	return rotationOy;
}

void Camera::setYaw(float yaw)
{
	rotationOy = yaw;
	updateVectors();
}

void Camera::updateVectors()
{
	float yawRad = glm::radians(rotationOy);

	cameraViewDirection = glm::normalize(glm::vec3(
		sin(yawRad),
		0.0f,
		cos(yawRad)
	));

	cameraRight = glm::normalize(glm::cross(cameraViewDirection, glm::vec3(0, 1, 0)));
	cameraUp = glm::normalize(glm::cross(cameraRight, cameraViewDirection));
}



