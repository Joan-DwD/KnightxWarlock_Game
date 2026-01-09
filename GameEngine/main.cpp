#include "Camera\camera.h"
#include "Shaders\shader.h"
#include "Model Loading\mesh.h"
#include "Model Loading\texture.h"
#include "Model Loading\meshLoaderObj.h"
#include <iostream>
#include <cmath>
#include <map>
#include <string>

// --- Custom Class Includes ---
#include "TextRenderer.h"
#include "Wall.h"
#include "Collision.h"

// ======================
// FUNCTION DECLARATIONS
// ======================
void processKeyboardInput();

// ======================
// TIME
// ======================
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// ======================
// WINDOW
// ======================
Window window("KNIGHTXWARLOCK", 1920, 1080);

// ======================
// CAMERA
// ======================
Camera camera(glm::vec3(0.0f, 15.0f, 4.2f)); // Above center of room

// ======================
// LIGHT
// ======================
glm::vec3 lightColor = glm::vec3(0.8f, 0.6f, 0.4f);
glm::vec3 lightPos = glm::vec3(0.0f, 6.5f, 1.0f);

int main()
{
    // ======================
    // OPENGL SETUP
    // ======================
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    // Enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // ======================
    // SHADERS
    // ======================
    Shader shader("Shaders/vertex_shader.glsl", "Shaders/fragment_shader.glsl");
    Shader sunShader("Shaders/sun_vertex_shader.glsl", "Shaders/sun_fragment_shader.glsl");
    Shader textShader("Shaders/text_vertex.glsl", "Shaders/text_fragment.glsl");
    Shader diagShader("Shaders/dialogue_vertex.glsl", "Shaders/dialogue_fragment.glsl");

    // ======================
    // TEXTURES
    // ======================
    GLuint woodTex = loadBMP("Resources/Textures/wood.bmp");
    GLuint rockTex = loadBMP("Resources/Textures/rock.bmp");
    GLuint orangeTex = loadBMP("Resources/Textures/orange.bmp");
    GLuint purpleTex = loadBMP("Resources/Textures/purple.bmp");
    GLuint goldTex = loadBMP("Resources/Textures/gold.bmp");

    std::vector<Texture> woodTextures = { { woodTex, "texture_diffuse" } };
    std::vector<Texture> stoneTextures = { { rockTex, "texture_diffuse" } };
    std::vector<Texture> orangeTextures = { { orangeTex, "texture_diffuse" } };
    std::vector<Texture> purpleTextures = { { purpleTex, "texture_diffuse" } };
    std::vector<Texture> goldTextures = { { goldTex, "texture_diffuse" } };

    // ======================
    // LOAD MODELS
    // ======================
    MeshLoaderObj loader;

    Mesh wallCube = loader.loadObj("Resources/Models/cube.obj", stoneTextures);
    Mesh floorCube = loader.loadObj("Resources/Models/cube.obj", stoneTextures);
    Mesh prisonWall = loader.loadObj("Resources/Models/cube.obj", stoneTextures);

    // Pawns
    Mesh warlock = loader.loadObj("Resources/Models/pawn.obj", purpleTextures);
    Mesh knight = loader.loadObj("Resources/Models/pawn.obj", goldTextures);

    // Key and door
    Mesh keyMesh = loader.loadObj("Resources/Models/key.obj", goldTextures);
    Mesh doorMesh = loader.loadObj("Resources/Models/cube.obj", woodTextures);

    // Dialogue Box: We pass an EMPTY texture list because the shader uses solid color only
    std::vector<Texture> noTextures;
    Mesh dialogueBoxMesh = loader.loadObj("Resources/Models/cube.obj", noTextures);

    // ======================
    // --- CREATE WALL OBJECTS ---
    // ======================
    // Back wall (+Z)
    Wall backWall(&wallCube, glm::vec3(0.0f, 3.5f, 7.0f), glm::vec3(7.0f, 3.5f, 0.1f));
    // Front wall (-Z)
    Wall frontWall(&wallCube, glm::vec3(0.0f, 3.5f, -7.0f), glm::vec3(7.0f, 3.5f, 0.1f));
    // Left wall (-X)
    Wall leftWall(&wallCube, glm::vec3(-7.0f, 3.5f, 0.0f), glm::vec3(0.1f, 3.5f, 7.0f));
    // Right wall (+X)
    Wall rightWall(&wallCube, glm::vec3(7.0f, 3.5f, 0.0f), glm::vec3(0.1f, 3.5f, 7.0f));
    // Middle wall
    Wall middleWall(&prisonWall, glm::vec3(-2.0f, 2.0f, 0.0f), glm::vec3(5.0f, 2.0f, 0.1f));

    // ======================
    // TEXT RENDERER SETUP
    // ======================
    // Initialize the TextRenderer class
    TextRenderer textRenderer("C:/Windows/Fonts/arial.ttf", 48);

    // Setup Text Projection (Orthographic) for the shader
    // We still do this here because we have access to 'window' dimensions
    glm::mat4 textProjection = glm::ortho(0.0f, static_cast<float>(window.getWidth()), 0.0f, static_cast<float>(window.getHeight()));
    textShader.use();
    glUniformMatrix4fv(glGetUniformLocation(textShader.getId(), "projection"), 1, GL_FALSE, glm::value_ptr(textProjection));

    // Setup Dialogue Shader (Important: Set projection here!)
    diagShader.use();
    glUniformMatrix4fv(glGetUniformLocation(diagShader.getId(), "projection"), 1, GL_FALSE, glm::value_ptr(textProjection));

    // ======================
    // OBJECT POSITIONS
    // ======================
    glm::vec3 warlockPos = glm::vec3(3.0f, 2.0f, 3.0f);
    glm::vec3 knightPos = glm::vec3(-3.0f, 2.0f, -3.0f);
    bool activeIsWarlock = true; // start controlling Warlock
    const glm::vec3 pawnHalfSize(0.3f, 1.0f, 0.3f); // collision box for player

    glm::vec3 keyPos = glm::vec3(-3.0f, 0.2f, -3.0f);   // on floor
    bool keyCollected = false;
    bool hasKey = false;

    glm::vec3 doorPos = glm::vec3(5.0f, 2.0f, 0.0f);
    bool doorUnlocked = false;

    glm::vec3 exitPos = glm::vec3(0.0f, 2.0f, -6.9f);

    // ======================
    // DIALOGUE SYSTEM STATE
    // ======================
    std::vector<std::string> dialogueLines = {
        "Hi!! Press R to advance through dialogue",
        "You made it yay!",
        "Use WASD to move your characters.",
        "The Warlock can reach places the Knight cannot.",
        "Press space to swap character control!",
    };
    int currentLineIndex = 0;
    bool rPressedLastFrame = false; // Prevents skipping 60 lines per second


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
        // MATRICES
        // ======================
        glm::mat4 ProjectionMatrix = glm::perspective(90.0f, window.getWidth() * 1.0f / window.getHeight(), 0.1f, 10000.0f);
        glm::mat4 ViewMatrix = glm::lookAt(
            camera.getCameraPosition(),
            glm::vec3(camera.getCameraPosition().x, camera.getCameraPosition().y - 0.8f, camera.getCameraPosition().z - 0.25f),
            glm::vec3(0.0f, 0.0f, -1.0f)
        );
        glm::mat4 ModelMatrix;
        glm::mat4 MVP;

        shader.use();
        GLuint MatrixID2 = glGetUniformLocation(shader.getId(), "MVP");
        GLuint ModelMatrixID = glGetUniformLocation(shader.getId(), "model");

        glUniform3f(glGetUniformLocation(shader.getId(), "lightColor"), lightColor.x, lightColor.y, lightColor.z);
        glUniform3f(glGetUniformLocation(shader.getId(), "lightPos"), lightPos.x, lightPos.y, lightPos.z);
        glUniform3f(glGetUniformLocation(shader.getId(), "viewPos"), camera.getCameraPosition().x,
            camera.getCameraPosition().y,
            camera.getCameraPosition().z);

        // ======================
        // WALLS
        // ======================
        backWall.draw(shader, ViewMatrix, ProjectionMatrix);
        frontWall.draw(shader, ViewMatrix, ProjectionMatrix);
        leftWall.draw(shader, ViewMatrix, ProjectionMatrix);
        rightWall.draw(shader, ViewMatrix, ProjectionMatrix);
        middleWall.draw(shader, ViewMatrix, ProjectionMatrix);

        // ======================
        // FLOOR
        // ======================
        ModelMatrix = glm::mat4(1.0f);
        ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0.0f, 0.0f, 0.0f));
        ModelMatrix = glm::scale(ModelMatrix, glm::vec3(7.0f, 0.1f, 7.0f));
        MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
        glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
        glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
        floorCube.draw(shader);

        // ======================
        // CHARACTER SWAP (SPACE)
        // ======================
        static bool spacePressedLastFrame = false;
        if (window.isPressed(GLFW_KEY_SPACE))
        {
            if (!spacePressedLastFrame)
            {
                activeIsWarlock = !activeIsWarlock;
                spacePressedLastFrame = true;
            }
        }
        else
        {
            spacePressedLastFrame = false;
        }

        // ======================
        // COLLIDERS
        // ======================

        std::vector<AABB> colliders;

        colliders.push_back(backWall.getAABB());
        colliders.push_back(frontWall.getAABB());
        colliders.push_back(leftWall.getAABB());
        colliders.push_back(rightWall.getAABB());
        colliders.push_back(middleWall.getAABB());

        if (!doorUnlocked)
        {
            colliders.push_back(
                makeAABB(doorPos, glm::vec3(2.0f, 2.0f, 0.1f))
            );
        }
        // ======================
        // PAWN MOVEMENT
        // ======================
        float speed = 5.0f * deltaTime;
        glm::vec3 delta(0.0f);

        if (window.isPressed(GLFW_KEY_W)) delta.z -= speed;
        if (window.isPressed(GLFW_KEY_S)) delta.z += speed;
        if (window.isPressed(GLFW_KEY_A)) delta.x -= speed;
        if (window.isPressed(GLFW_KEY_D)) delta.x += speed;

        if (activeIsWarlock)
        {
            movement(warlockPos, delta, pawnHalfSize, colliders);
        }
        else
        {
            movement(knightPos, delta, pawnHalfSize, colliders);
        }

        // ======================
        // KEY PICKUP
        // ======================
        glm::vec3 activePos = activeIsWarlock ? warlockPos : knightPos;
        float distToKey = glm::length(activePos - keyPos);
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

        // ======================
        // DOOR INTERACTION
        // ======================
        float distToDoor = glm::length(activePos - doorPos);
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
        }

        // =============================
        // DIALOGUE CYCLING (Press R)
        // =============================
        if (window.isPressed(GLFW_KEY_R))
        {
            if (!rPressedLastFrame) // Only trigger once per press
            {
                currentLineIndex++;
                // Loop back to start if we reach the end
                if (currentLineIndex >= dialogueLines.size()) {
                    currentLineIndex = 0;
                }
                rPressedLastFrame = true;
            }
        }
        else
        {
            rPressedLastFrame = false;
        }

        // ======================
        // DRAW PAWNS
        // ======================
        // Warlock
        ModelMatrix = glm::mat4(1.0f);
        ModelMatrix = glm::translate(ModelMatrix, warlockPos);
        ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.4f));
        MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
        glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
        glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
        warlock.draw(shader);

        // Knight
        ModelMatrix = glm::mat4(1.0f);
        ModelMatrix = glm::translate(ModelMatrix, knightPos);
        ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.4f));
        MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
        glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
        glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
        knight.draw(shader);

        // ======================
        // DRAW KEY
        // ======================
        if (!keyCollected)
        {
            ModelMatrix = glm::mat4(1.0f);
            ModelMatrix = glm::translate(ModelMatrix, keyPos);
            ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.02f));
            MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
            glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
            glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
            keyMesh.draw(shader);
        }

        // ======================
        // DRAW DOOR
        // ======================
        if (!doorUnlocked)
        {
            ModelMatrix = glm::mat4(1.0f);
            ModelMatrix = glm::translate(ModelMatrix, doorPos);
            ModelMatrix = glm::scale(ModelMatrix, glm::vec3(2.0f, 2.0f, 0.1f));
            MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
            glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
            glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
            doorMesh.draw(shader);
        }

        // ======================
        // DRAW EXIT
        // ======================
        ModelMatrix = glm::mat4(1.0f);
        ModelMatrix = glm::translate(ModelMatrix, exitPos);
        ModelMatrix = glm::scale(ModelMatrix, glm::vec3(2.0f, 2.0f, 0.1f));
        MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
        glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
        glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
        doorMesh.draw(shader);


        // ==================================
        // DRAW UI / DIALOGUE BOX
        // ==================================

        glDisable(GL_DEPTH_TEST);

        // --- 1. Render the Background Box ---
        diagShader.use();

        // Set Model Matrix for UI: Bottom Center
        glm::mat4 boxModel = glm::mat4(1.0f);
        boxModel = glm::translate(boxModel, glm::vec3(window.getWidth() / 2.0f, 100.0f, 0.0f));
        boxModel = glm::scale(boxModel, glm::vec3(1200.0f, 200.0f, 1.0f));

        // Pass "model" uniform
        glUniformMatrix4fv(glGetUniformLocation(diagShader.getId(), "model"), 1, GL_FALSE, &boxModel[0][0]);

        // Pass "color" uniform (Navy Blue) - R, G, B
        glUniform3f(glGetUniformLocation(diagShader.getId(), "color"), 0.1f, 0.15f, 0.5f);

        dialogueBoxMesh.draw(diagShader);


        // --- 2. Render Text On Top ---

        // Hints (Top Left)
        if (!hasKey) {
            textRenderer.RenderText(textShader, "Find the Key...", 25.0f, 1000.0f, 0.8f, glm::vec3(1.0f, 1.0f, 1.0f));
        }
        else if (!doorUnlocked) {
            textRenderer.RenderText(textShader, "Go to the Door!", 25.0f, 1000.0f, 0.8f, glm::vec3(0.2f, 1.0f, 0.2f));
        }
        else {
            textRenderer.RenderText(textShader, "YOU ESCAPED!", 25.0f, 1000.0f, 0.8f, glm::vec3(1.0f, 0.8f, 0.0f));
        }

        // --- DIALOGUE BOX TEXT ---
        // We use the string from our vector based on the current index
        textRenderer.RenderText(textShader, dialogueLines[currentLineIndex], 700.0f, 85.0f, 0.8f, glm::vec3(1.0f, 1.0f, 1.0f));

        glEnable(GL_DEPTH_TEST);

        window.update();
    }

    return 0;
}


// ======================
// CAMERA INPUT (commented out)
// ======================
void processKeyboardInput()
{
    /*
    float cameraSpeed = 30 * deltaTime;
    if (window.isPressed(GLFW_KEY_W)) camera.keyboardMoveFront(cameraSpeed);
    if (window.isPressed(GLFW_KEY_S)) camera.keyboardMoveBack(cameraSpeed);
    if (window.isPressed(GLFW_KEY_A)) camera.keyboardMoveLeft(cameraSpeed);
    if (window.isPressed(GLFW_KEY_D)) camera.keyboardMoveRight(cameraSpeed);
    if (window.isPressed(GLFW_KEY_R)) camera.keyboardMoveUp(cameraSpeed);
    if (window.isPressed(GLFW_KEY_F)) camera.keyboardMoveDown(cameraSpeed);
    if (window.isPressed(GLFW_KEY_LEFT)) camera.rotateOy(cameraSpeed);
    if (window.isPressed(GLFW_KEY_RIGHT)) camera.rotateOy(-cameraSpeed);
    if (window.isPressed(GLFW_KEY_UP)) camera.rotateOx(cameraSpeed);
    if (window.isPressed(GLFW_KEY_DOWN)) camera.rotateOx(-cameraSpeed);
    */
}