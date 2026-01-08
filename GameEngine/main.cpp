#include "Camera\camera.h"
#include "Shaders\shader.h"
#include "Model Loading\mesh.h"
#include "Model Loading\texture.h"
#include "Model Loading\meshLoaderObj.h"
#include <iostream>
#include <cmath>

// ======================
// FUNCTION DECLARATIONS
// ======================
void processKeyboardInput();

// ======================
// TIME
// ======================
float deltaTime = 0.0f; // Time between current frame and last frame
float lastFrame = 0.0f;

// ======================
// WINDOW
// ======================
Window window("Simple Room", 800, 800);

// ======================
// CAMERA
// ======================
Camera camera(glm::vec3(0.0f, 10.0f, 0.0f)); // Above center of room

// ======================
// LIGHT
// ======================
glm::vec3 lightColor = glm::vec3(0.8f, 0.6f, 0.4f); // Warm, dim torchlight
glm::vec3 lightPos = glm::vec3(6.5f, 4.0f, 20.0f); // Near door, above

int main()
{
    // ======================
    // OPENGL SETUP
    // ======================
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE); // Disable backface culling to see walls from inside

    // ======================
    // SHADERS
    // ======================
    Shader shader("Shaders/vertex_shader.glsl", "Shaders/fragment_shader.glsl");
    Shader sunShader("Shaders/sun_vertex_shader.glsl", "Shaders/sun_fragment_shader.glsl");

    // ======================
    // TEXTURES
    // ======================
    GLuint woodTex = loadBMP("Resources/Textures/wood.bmp");
    GLuint rockTex = loadBMP("Resources/Textures/rock.bmp");
    GLuint orangeTex = loadBMP("Resources/Textures/orange.bmp");

    // Prepare textures
    std::vector<Texture> woodTextures = { { woodTex, "texture_diffuse" } };
    std::vector<Texture> stoneTextures = { { rockTex, "texture_diffuse" } };
    std::vector<Texture> orangeTextures = { { orangeTex, "texture_diffuse" } };

    // ======================
    // LOAD MODELS
    // ======================
    MeshLoaderObj loader;

    // Simple room - walls and floor
    Mesh wallCube = loader.loadObj("Resources/Models/cube.obj", stoneTextures);
    Mesh wallCube1 = loader.loadObj("Resources/Models/cube.obj", woodTextures);
    Mesh floorCube = loader.loadObj("Resources/Models/cube.obj", stoneTextures);

    // Prison cell objects
    Mesh key = loader.loadObj("Resources/Models/key.obj", orangeTextures);
    Mesh door = loader.loadObj("Resources/Models/cube.obj", woodTextures);

    // Pawn
    Mesh pawn = loader.loadObj("Resources/Models/pawn.obj", woodTextures);

    // Wooden beams/pillars and bars
    Mesh beam = loader.loadObj("Resources/Models/cube.obj", woodTextures);
    Mesh pillar = loader.loadObj("Resources/Models/cube.obj", woodTextures);
    Mesh bar = loader.loadObj("Resources/Models/cube.obj", woodTextures);

    // ======================
    // OBJECT POSITIONS
    // ======================
    glm::vec3 keyPos = glm::vec3(0.0f, 0.1f, 0.0f);   // Key on floor
    glm::vec3 doorPos = glm::vec3(6.5f, 1.5f, 24.0f);  // Center of front wall, door height
    glm::vec3 pawnPos = glm::vec3(6.5f, 0.1f, 10.0f);  // Pawn on floor

    // ======================
    // GAME STATE
    // ======================
    bool hasKey = false;
    bool keyCollected = false;
    bool doorUnlocked = false;

    std::cout << "=== PRISON CELL ===" << std::endl;
    std::cout << "You awaken in a cold stone cell..." << std::endl;
    std::cout << "Controls: WASD to move, Arrow keys to look around, E to interact" << std::endl;

    // ======================
    // MAIN LOOP
    // ======================
    while (!window.isPressed(GLFW_KEY_ESCAPE) && glfwWindowShouldClose(window.getWindow()) == 0)
    {
        window.clear();

        // ======================
        // TIME UPDATE
        // ======================
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // ======================
        // CAMERA INPUT
        // ======================
        processKeyboardInput();

        // ======================
        // MATRICES
        // ======================
        glm::mat4 ProjectionMatrix = glm::perspective(90.0f, window.getWidth() * 1.0f / window.getHeight(), 0.1f, 10000.0f);
        glm::mat4 ViewMatrix = glm::lookAt(
            camera.getCameraPosition(),
            glm::vec3(camera.getCameraPosition().x, 0.0f, camera.getCameraPosition().z), // Look at floor
            glm::vec3(0.0f, 0.0f, -1.0f) // Up vector pointing along -Z so "forward" is consistent
        );
        glm::mat4 ModelMatrix;
        glm::mat4 MVP;

        // ======================
        // SHADER UNIFORMS
        // ======================
        shader.use();
        GLuint MatrixID2 = glGetUniformLocation(shader.getId(), "MVP");
        GLuint ModelMatrixID = glGetUniformLocation(shader.getId(), "model");

        glUniform3f(glGetUniformLocation(shader.getId(), "lightColor"), lightColor.x, lightColor.y, lightColor.z);
        glUniform3f(glGetUniformLocation(shader.getId(), "lightPos"), lightPos.x, lightPos.y, lightPos.z);
        glUniform3f(glGetUniformLocation(shader.getId(), "viewPos"), camera.getCameraPosition().x,
            camera.getCameraPosition().y,
            camera.getCameraPosition().z);

        // Back wall (+Z)
        ModelMatrix = glm::mat4(1.0f);
        ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0.0f, 1.5f, 5.0f)); // center Y=1.5, Z=far edge
        ModelMatrix = glm::scale(ModelMatrix, glm::vec3(5.0f, 1.5f, 0.2f));       // width X=5, height Y=3, depth Z=0.2
        MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
        glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
        glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
        wallCube.draw(shader);

        // Front wall (-Z)
        ModelMatrix = glm::mat4(1.0f);
        ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0.0f, 1.5f, -5.0f));
        ModelMatrix = glm::scale(ModelMatrix, glm::vec3(5.0f, 1.5f, 0.2f));
        MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
        glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
        glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
        wallCube.draw(shader);

        // Left wall (-X)
        ModelMatrix = glm::mat4(1.0f);
        ModelMatrix = glm::translate(ModelMatrix, glm::vec3(-5.0f, 1.5f, 0.0f));
        ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.2f, 1.5f, 5.0f));
        MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
        glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
        glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
        wallCube.draw(shader);

        // Right wall (+X)
        ModelMatrix = glm::mat4(1.0f);
        ModelMatrix = glm::translate(ModelMatrix, glm::vec3(5.0f, 1.5f, 0.0f));
        ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.2f, 3.0f, 5.0f));
        MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
        glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
        glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
        wallCube.draw(shader);


        // ======================
        // FLOOR
        // ======================
        ModelMatrix = glm::mat4(1.0);
        ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0.0f, 0.0f, 0.0f));
        ModelMatrix = glm::scale(ModelMatrix, glm::vec3(5.0f, 0.1f, 5.0f));
        MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
        glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
        glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
        floorCube.draw(shader);

        // ======================
        // KEY
        // ======================
        if (!keyCollected)
        {
            ModelMatrix = glm::mat4(1.0);
            ModelMatrix = glm::translate(ModelMatrix, keyPos);
            ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.03f));
            MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
            glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
            glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
            key.draw(shader);
        }

        // ======================
        // PAWN
        // ======================
        ModelMatrix = glm::mat4(1.0);
        ModelMatrix = glm::translate(ModelMatrix, pawnPos);
        ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.8f));
        MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
        glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
        glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
        pawn.draw(shader);

        // ======================
        // INTERACTIONS
        // ======================
        glm::vec3 cameraPos = camera.getCameraPosition();

        // Key pickup
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

        // Door interaction
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
                        std::cout << "Press E to unlock the door" << std::endl;
                    else
                        std::cout << "Press E to try the door (locked)" << std::endl;

                    shownDoorPrompt = true;
                }
            }
        }

        window.update();
    }

    return 0;
}

// ======================
// CAMERA INPUT
// ======================
void processKeyboardInput()
{
    float cameraSpeed = 30 * deltaTime;

    // Translation
    if (window.isPressed(GLFW_KEY_W)) camera.keyboardMoveFront(cameraSpeed);
    if (window.isPressed(GLFW_KEY_S)) camera.keyboardMoveBack(cameraSpeed);
    if (window.isPressed(GLFW_KEY_A)) camera.keyboardMoveLeft(cameraSpeed);
    if (window.isPressed(GLFW_KEY_D)) camera.keyboardMoveRight(cameraSpeed);
    if (window.isPressed(GLFW_KEY_R)) camera.keyboardMoveUp(cameraSpeed);
    if (window.isPressed(GLFW_KEY_F)) camera.keyboardMoveDown(cameraSpeed);

    // Rotation
    if (window.isPressed(GLFW_KEY_LEFT)) camera.rotateOy(cameraSpeed);
    if (window.isPressed(GLFW_KEY_RIGHT)) camera.rotateOy(-cameraSpeed);
    if (window.isPressed(GLFW_KEY_UP)) camera.rotateOx(cameraSpeed);
    if (window.isPressed(GLFW_KEY_DOWN)) camera.rotateOx(-cameraSpeed);
}
