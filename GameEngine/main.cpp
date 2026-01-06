#include "Camera\camera.h"
#include "Shaders\shader.h"
#include "Model Loading\mesh.h"
#include "Model Loading\texture.h"
#include "Model Loading\meshLoaderObj.h"
#include <iostream>
#include <cmath>

void processKeyboardInput();

float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

Window window("Simple Room", 800, 800);
Camera camera(glm::vec3(0.0f, 2.0f, 5.0f)); // Start position: center, eye level, looking into room

// Light setup - dim prison cell atmosphere
glm::vec3 lightColor = glm::vec3(0.8f, 0.6f, 0.4f); // Warm, dim torchlight
glm::vec3 lightPos = glm::vec3(6.5f, 4.0f, 20.0f); // Near door, above

int main()
{
	glClearColor(0.1f, 0.1f, 0.15f, 1.0f);

	//building and compiling shader program
	Shader shader("Shaders/vertex_shader.glsl", "Shaders/fragment_shader.glsl");
	Shader sunShader("Shaders/sun_vertex_shader.glsl", "Shaders/sun_fragment_shader.glsl");

	// Load textures
	GLuint woodTex = loadBMP("Resources/Textures/wood.bmp");
	GLuint rockTex = loadBMP("Resources/Textures/rock.bmp");
	GLuint orangeTex = loadBMP("Resources/Textures/orange.bmp");

	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE); // Disable backface culling so we can see walls from inside

	// Prepare textures
	std::vector<Texture> woodTextures;
	woodTextures.push_back(Texture());
	woodTextures[0].id = woodTex;
	woodTextures[0].type = "texture_diffuse";

	std::vector<Texture> stoneTextures;
	stoneTextures.push_back(Texture());
	stoneTextures[0].id = rockTex;
	stoneTextures[0].type = "texture_diffuse";

	std::vector<Texture> orangeTextures;
	orangeTextures.push_back(Texture());
	orangeTextures[0].id = orangeTex;
	orangeTextures[0].type = "texture_diffuse";

	// Load 3D models
	MeshLoaderObj loader;

	// Light source (sun) - dim orange glow
	Mesh sun = loader.loadObj("Resources/Models/sphere.obj");

	// Simple room - walls and floor
	Mesh wallCube = loader.loadObj("Resources/Models/cube.obj", stoneTextures);
	Mesh wallCube1 = loader.loadObj("Resources/Models/cube.obj", woodTextures);
	Mesh floorCube = loader.loadObj("Resources/Models/cube.obj", stoneTextures);

	// Prison cell objects
	Mesh bed = loader.loadObj("Resources/Models/cube.obj", woodTextures);
	Mesh key = loader.loadObj("Resources/Models/cube.obj", orangeTextures);
	Mesh door = loader.loadObj("Resources/Models/cube.obj", woodTextures);
	Mesh castle = loader.loadObj("Resources/Models/Castle/Castle/Castle OBJ.obj", stoneTextures);

	// Wooden beams and pillars (for structure)
	Mesh beam = loader.loadObj("Resources/Models/cube.obj", woodTextures);
	Mesh pillar = loader.loadObj("Resources/Models/cube.obj", woodTextures);

	// Wooden bars/beams for cage-like effect
	Mesh bar = loader.loadObj("Resources/Models/cube.obj", woodTextures);

	// Object positions (prison cell)
	glm::vec3 bedPos = glm::vec3(-3.0f, 0.5f, 3.0f);      // Back-left corner
	glm::vec3 keyPos = glm::vec3(-2.0f, 0.1f, 4.0f);     // Near bed, on floor
	glm::vec3 doorPos = glm::vec3(6.5f, 1.5f, 24.0f);    // Center of front wall, door height

	// Game state
	bool hasKey = false;
	bool keyCollected = false;
	bool doorUnlocked = false;

	std::cout << "=== PRISON CELL ===" << std::endl;
	std::cout << "You awaken in a cold stone cell..." << std::endl;
	std::cout << "Controls: WASD to move, Arrow keys to look around, E to interact" << std::endl;

	//check if we close the window or press the escape button
	while (!window.isPressed(GLFW_KEY_ESCAPE) &&
		glfwWindowShouldClose(window.getWindow()) == 0)
	{
		window.clear();
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		processKeyboardInput();

		// Calculate matrices
		glm::mat4 ProjectionMatrix = glm::perspective(90.0f, window.getWidth() * 1.0f / window.getHeight(), 0.1f, 10000.0f);
		glm::mat4 ViewMatrix = glm::lookAt(camera.getCameraPosition(), camera.getCameraPosition() + camera.getCameraViewDirection(), camera.getCameraUp());
		glm::mat4 ModelMatrix;
		glm::mat4 MVP;
	
		// Switch to main shader
		shader.use();
		GLuint MatrixID2 = glGetUniformLocation(shader.getId(), "MVP");
		GLuint ModelMatrixID = glGetUniformLocation(shader.getId(), "model");

		// Send light information to shader
		glUniform3f(glGetUniformLocation(shader.getId(), "lightColor"), lightColor.x, lightColor.y, lightColor.z);
		glUniform3f(glGetUniformLocation(shader.getId(), "lightPos"), lightPos.x, lightPos.y, lightPos.z);
		glUniform3f(glGetUniformLocation(shader.getId(), "viewPos"), camera.getCameraPosition().x, camera.getCameraPosition().y, camera.getCameraPosition().z);

		// back wall
		ModelMatrix = glm::mat4(1.0);
		ModelMatrix = glm::translate(ModelMatrix, glm::vec3(5.8f, 2.5f, 1.0f));
		ModelMatrix = glm::scale(ModelMatrix, glm::vec3(4.4f, 2.5f, 0.2f)); // width=8, height=5
		MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
		glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
		wallCube.draw(shader);

		// front wall
		// Left part of wall
		ModelMatrix = glm::mat4(1.0);
		ModelMatrix = glm::translate(ModelMatrix, glm::vec3(-1.7f, 2.5f, 24.0f));
		ModelMatrix = glm::scale(ModelMatrix, glm::vec3(1.5f, 2.5f, 0.2f)); // Left side of door
		MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
		glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
		wallCube1.draw(shader);
		// Right part of wall
		ModelMatrix = glm::mat4(1.0);
		ModelMatrix = glm::translate(ModelMatrix, glm::vec3(14.0f, 2.5f, 24.0f));
		ModelMatrix = glm::scale(ModelMatrix, glm::vec3(1.66f, 2.5f, 0.2f)); // Right side of door 3 5 0.4
		MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
		glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
		wallCube1.draw(shader);
		// Top 
		ModelMatrix = glm::mat4(1.0);
		ModelMatrix = glm::translate(ModelMatrix, glm::vec3(5.91f, 11.9f, 24.0f));
		ModelMatrix = glm::scale(ModelMatrix, glm::vec3(1.24f, 0.5f, 0.2f)); // Above door 2 1 0.4
		MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
		glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
		wallCube1.draw(shader);
		// DOOR 
		ModelMatrix = glm::mat4(1.0);
		ModelMatrix = glm::translate(ModelMatrix, glm::vec3(5.91f, 2.5f, 24.0f));
		ModelMatrix = glm::scale(ModelMatrix, glm::vec3(1.24f, 2.0f, 0.2f)); // Door size (fits opening)
		MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
		glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
		key.draw(shader);

		// L WALL
		ModelMatrix = glm::mat4(1.0);
		ModelMatrix = glm::translate(ModelMatrix, glm::vec3(-5.5f, 2.5f, 0.0f));
		ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.2f, 2.5f, 5.0f)); // depth=10, height=5
		MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
		glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
		wallCube.draw(shader);

		// RIGHT WALL
		ModelMatrix = glm::mat4(1.0);
		ModelMatrix = glm::translate(ModelMatrix, glm::vec3(18.4f, 2.5f, 0.0f));
		ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.2f, 2.5f, 5.0f)); // depth=10, height=5
		MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
		glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
		wallCube.draw(shader);

		// Floor
		ModelMatrix = glm::mat4(1.0);
		ModelMatrix = glm::translate(ModelMatrix, glm::vec3(6.5f, 2.5f, -10.5f)); // Center: X=(18-5)/2+(-5)=6.5, Z=(24+1)/2=12.5
		ModelMatrix = glm::scale(ModelMatrix, glm::vec3(24.5f, 0.1f, 20.5f)); // width=23 (11.5*2), depth=23 (11.5*2), thin floor
		MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
		glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
		floorCube.draw(shader);

		// BED not complete
		ModelMatrix = glm::mat4(1.0);
		ModelMatrix = glm::translate(ModelMatrix, bedPos);
		ModelMatrix = glm::scale(ModelMatrix, glm::vec3(1.5f, 0.5f, 2.0f)); // Bed size
		MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
		glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
		bed.draw(shader);

		// KEY (on floor, near bed - only if not collected)
		if (!keyCollected)
		{
			ModelMatrix = glm::mat4(1.0);
			ModelMatrix = glm::translate(ModelMatrix, keyPos);
			ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.2f, 0.1f, 0.4f)); // Small key
			MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
			glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
			glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
			key.draw(shader);
		}

		glm::vec3 cameraPos = camera.getCameraPosition();

		// look for key
		float distToKey = glm::length(cameraPos - keyPos);
		if (distToKey < 2.0f && !keyCollected)
		{
			if (window.isPressed(GLFW_KEY_E))
			{
				keyCollected = true;
				hasKey = true;
				std::cout << ">>> You picked up the key!" << std::endl;
			}
			else
			{
				static bool shownPrompt = false;
				if (!shownPrompt)
				{
					std::cout << "Press E to pick up the key" << std::endl;
					shownPrompt = true;
				}
			}
		}

		// Check distance to door
		float distToDoor = glm::length(cameraPos - doorPos);
		if (distToDoor < 3.0f)
		{
			if (window.isPressed(GLFW_KEY_E))
			{
				if (hasKey && !doorUnlocked)
				{
					doorUnlocked = true;
					std::cout << ">>> The door unlocks with a click!" << std::endl;
					std::cout << ">>> You are free to leave..." << std::endl;
				}
				else if (!hasKey)
				{
					std::cout << ">>> The door is locked. You need a key." << std::endl;
				}
			}
			else
			{
				static bool shownDoorPrompt = false;
				if (!shownDoorPrompt && !doorUnlocked)
				{
					if (hasKey)
					{
						std::cout << "Press E to unlock the door" << std::endl;
					}
					else
					{
						std::cout << "Press E to try the door (locked)" << std::endl;
					}
					shownDoorPrompt = true;
				}
			}
		}

		window.update();
	}

	return 0;
}

void processKeyboardInput()
{
	float cameraSpeed = 30 * deltaTime;

	//translation
	if (window.isPressed(GLFW_KEY_W))
		camera.keyboardMoveFront(cameraSpeed);
	if (window.isPressed(GLFW_KEY_S))
		camera.keyboardMoveBack(cameraSpeed);
	if (window.isPressed(GLFW_KEY_A))
		camera.keyboardMoveLeft(cameraSpeed);
	if (window.isPressed(GLFW_KEY_D))
		camera.keyboardMoveRight(cameraSpeed);
	if (window.isPressed(GLFW_KEY_R))
		camera.keyboardMoveUp(cameraSpeed);
	if (window.isPressed(GLFW_KEY_F))
		camera.keyboardMoveDown(cameraSpeed);

	//rotation
	if (window.isPressed(GLFW_KEY_LEFT))
		camera.rotateOy(cameraSpeed);
	if (window.isPressed(GLFW_KEY_RIGHT))
		camera.rotateOy(-cameraSpeed);
	if (window.isPressed(GLFW_KEY_UP))
		camera.rotateOx(cameraSpeed);
	if (window.isPressed(GLFW_KEY_DOWN))
		camera.rotateOx(-cameraSpeed);
}