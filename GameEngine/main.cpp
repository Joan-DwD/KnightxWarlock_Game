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
#include "Torch.h"
#include "Collision.h"

// ======================
// QUEST HANDLING VARIABLES
// ======================

int currentTask = 1;
int currentRoom = 3;
int firstLoad = 1;

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
Window window("KNIGHTXWARLOCK", 1600, 900);

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
    GLuint bookTex = loadBMP("Resources/Textures/books.bmp");

    std::vector<Texture> woodTextures = { { woodTex, "texture_diffuse" } };
    std::vector<Texture> stoneTextures = { { rockTex, "texture_diffuse" } };
    std::vector<Texture> orangeTextures = { { orangeTex, "texture_diffuse" } };
    std::vector<Texture> purpleTextures = { { purpleTex, "texture_diffuse" } };
    std::vector<Texture> goldTextures = { { goldTex, "texture_diffuse" } };
    std::vector<Texture> bookTextures = { { bookTex, "texture_diffuse" } };

    // ======================
    // LOAD MODELS
    // ======================
    MeshLoaderObj loader;

    Mesh wallCube = loader.loadObj("Resources/Models/cube.obj", stoneTextures);
    Mesh floorCube = loader.loadObj("Resources/Models/cube.obj", stoneTextures);
    Mesh prisonWall = loader.loadObj("Resources/Models/cube.obj", stoneTextures);
    Mesh bookcaseCube = loader.loadObj("Resources/Models/cube.obj", woodTextures);
    Mesh booksCube = loader.loadObj("Resources/Models/cube.obj", bookTextures);

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
    // WALLS FOR ROOM 1 - PRISON
    // ======================
    // Back wall
    Wall backWall(&wallCube, glm::vec3(0.0f, 3.5f, 7.0f), glm::vec3(7.0f, 3.5f, 0.1f)); //back is down
    // Front wall
    Wall frontWall(&wallCube, glm::vec3(0.0f, 3.5f, -7.0f), glm::vec3(7.0f, 3.5f, 0.1f)); //front is up
    // Left wall
    Wall leftWall(&wallCube, glm::vec3(-7.0f, 3.5f, 0.0f), glm::vec3(0.1f, 3.5f, 7.0f));
    // Right wall
    Wall rightWall(&wallCube, glm::vec3(7.0f, 3.5f, 0.0f), glm::vec3(0.1f, 3.5f, 7.0f));
    // Middle wall
    Wall middleWall(&prisonWall, glm::vec3(-2.0f, 2.0f, 0.0f), glm::vec3(5.0f, 2.0f, 0.1f));

    // ======================
    // WALLS FOR ROOM 2 - HALLWAY
    // ======================
    // Back wall
    Wall backWall_2(&wallCube, glm::vec3(0.0f, 3.5f, 7.0f), glm::vec3(2.0f, 3.5f, 0.1f));
    // Front wallec)
    // same as above
    Wall frontWall_2(&wallCube, glm::vec3(0.0f, 3.5f, -7.0f), glm::vec3(7.0f, 3.5f, 0.1f)); //front is up
    //Left side walls
    Wall leftWall_2_a(&wallCube, glm::vec3(-7.0f, 3.5f, -3.5f), glm::vec3(0.1f, 3.5f, 3.5f));
    Wall leftWall_2_b(&wallCube, glm::vec3(-4.5f, 3.5f, 0.0f), glm::vec3(2.5f, 3.5f, 0.1f));
    Wall leftWall_2_c(&wallCube, glm::vec3(-2.0f, 3.5f, 3.5f), glm::vec3(0.1f, 3.5f, 3.5f));
    //Right side walls
    Wall rightWall_2_a(&wallCube, glm::vec3(7.0f, 3.5f, -3.5f), glm::vec3(0.1f, 3.5f, 3.5f));
    Wall rightWall_2_b(&wallCube, glm::vec3(4.5f, 3.5f, 0.0f), glm::vec3(2.5f, 3.5f, 0.1f));
    Wall rightWall_2_c(&wallCube, glm::vec3(2.0f, 3.5f, 3.5f), glm::vec3(0.1f, 3.5f, 3.5f));

    // ====================
    // BOOKSHELVES FOR ROOM 3 - LIBRARY
    // ====================

    Wall bookshelves[] = 
    {
     Wall(&bookcaseCube, glm::vec3(-3.5f, 2.0f, -4.5f), glm::vec3(2.0f, 2.0f, 1.0f)),
     Wall(&bookcaseCube, glm::vec3(-3.5f, 2.0f, 0.0f), glm::vec3(2.0f, 2.0f, 1.0f)),
     Wall(&bookcaseCube, glm::vec3(-3.5f, 2.0f, 4.5f), glm::vec3(2.0f, 2.0f, 1.0f)),
     Wall(&bookcaseCube, glm::vec3(3.5f, 2.0f, -4.5f), glm::vec3(2.0f, 2.0f, 1.0f)),
     Wall(&bookcaseCube, glm::vec3(3.5f, 2.0f, 0.0f), glm::vec3(2.0f, 2.0f, 1.0f)),
     Wall(&bookcaseCube, glm::vec3(3.5f, 2.0f, 4.5f), glm::vec3(2.0f, 2.0f, 1.0f))
    };

    Wall books[]
    {
        Wall(&booksCube, glm::vec3(-3.5, 2.0f, -3.4f), glm::vec3(1.8f, 1.8f, 0.1f)),
        Wall(&booksCube, glm::vec3(-3.5, 2.0f, 1.1f), glm::vec3(1.8f, 1.8f, 0.1f)),
        Wall(&booksCube, glm::vec3(3.5, 2.0f, -3.4f), glm::vec3(1.8f, 1.8f, 0.1f)),
        Wall(&booksCube, glm::vec3(3.5, 2.0f, 1.1f), glm::vec3(1.8f, 1.8f, 0.1f)),
    };

    const int BOOKSHELF_COUNT = sizeof(bookshelves) / sizeof(bookshelves[0]);
    const int BOOK_COUNT = sizeof(books) / sizeof(books[0]);



    // ======================
    // TORCH
    // ======================
    Torch* wallTorch = nullptr;

    // currently on left wall a bit below cell
    wallTorch = new Torch(&wallCube, &wallCube, glm::vec3(-6.8, 3.0f, 2.0f), 180.0f);

    Mesh flameCube = loader.loadObj("Resources/Models/cube.obj", orangeTextures);
    Mesh stickCube = loader.loadObj("Resources/Models/cube.obj", woodTextures);

    // re-initialize using specific textures
    delete wallTorch;

    // room 1 torch
    wallTorch = new Torch(&stickCube, &flameCube, glm::vec3(-6.8, 3.0f, 2.0f), 180.0f);

    // room 2 torches
    Torch* hallTorches[] = 
    {
     new Torch(&stickCube, &flameCube, glm::vec3(-6.0f, 3.0f, -6.8f), 180.0f),
     new Torch(&stickCube, &flameCube, glm::vec3(-4.0f, 3.0f, -6.8f), 180.0f),
     new Torch(&stickCube, &flameCube, glm::vec3(-2.0f, 3.0f, -6.8f), 180.0f),
     new Torch(&stickCube, &flameCube, glm::vec3(0.0f, 3.0f, -6.8f), 180.0f),
     new Torch(&stickCube, &flameCube, glm::vec3(2.0f, 3.0f, -6.8f), 180.0f),
     new Torch(&stickCube, &flameCube, glm::vec3(4.0f, 3.0f, -6.8f), 180.0f),
     new Torch(&stickCube, &flameCube, glm::vec3(6.0f, 3.0f, -6.8f), 180.0f)
    };

    const int HALL_TORCH_COUNT = sizeof(hallTorches) / sizeof(hallTorches[0]);

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
    glm::vec3 knightPos = glm::vec3(3.0f, 2.0f, -3.0f);
    bool activeIsWarlock = true; // start controlling Warlock
    const glm::vec3 pawnHalfSize(0.3f, 1.0f, 0.3f); // collision box for player

    glm::vec3 keyPos = glm::vec3(-3.0f, 0.2f, -3.0f);
    bool keyCollected = false;
    bool hasKey = false;

    glm::vec3 doorPos = glm::vec3(5.0f, 2.0f, 0.0f);
    bool doorUnlocked = false;

    bool isSolved = false; // room 2 puzzle

    glm::vec3 exitPos;

    // ======================
    // DIALOGUE SYSTEM STATE (needs changing to read from .txt)
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

        //  offset to be "inside" the flame
        glm::vec3 flameLightPos = wallTorch->position + glm::vec3(0.0f, 0.4f, 0.0f);
        glUniform3f(glGetUniformLocation(shader.getId(), "torchPos"), flameLightPos.x, flameLightPos.y, flameLightPos.z);
        glUniform3f(glGetUniformLocation(shader.getId(), "torchColor"), 1.0f, 0.5f, 0.0f);
        // torch state
        glUniform1i(glGetUniformLocation(shader.getId(), "torchOn"), wallTorch->isOn);

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

        // =============================
        // DIALOGUE CYCLING (Press R)
        // =============================
        if (window.isPressed(GLFW_KEY_R))
        {
            if (!rPressedLastFrame)
            {
                // Only increment if we haven't finished the dialogue yet.
                // When currentLineIndex equals dialogueLines.size(), dialogue ends
                if (currentLineIndex < dialogueLines.size()) {
                    currentLineIndex++;
                }
                rPressedLastFrame = true;
            }
        }
        else
        {
            rPressedLastFrame = false;
        }

        // ======================
        // COLLIDERS
        // ======================

        std::vector<AABB> colliders;

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

        // active character position
        glm::vec3 activePos = activeIsWarlock ? warlockPos : knightPos;

        if (currentRoom == 1)
        {
            if (firstLoad == 1)
            {
                glm::vec3 warlockPos = glm::vec3(3.0f, 2.0f, 3.0f);
                glm::vec3 knightPos = glm::vec3(3.0f, 2.0f, -3.0f);
                firstLoad = 0;
            }

            exitPos = glm::vec3(0.0f, 2.0f, -6.9f);

            // room 1 colliders
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
            // KEY PICKUP
            // ======================
            
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
                    }
                    else if (!hasKey)
                    {
                        std::cout << "door locked";
                    }
                }
            }

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

            // ======================
            // EXIT INTERACTION
            // ======================
            float distToExit = glm::length(warlockPos - exitPos);
            if (distToExit < 1.0f)
            {
                firstLoad = 1;
                currentRoom = 2;
            }

            // ======================
            // DRAW WALLS
            // ======================
            backWall.draw(shader, ViewMatrix, ProjectionMatrix);
            frontWall.draw(shader, ViewMatrix, ProjectionMatrix);
            leftWall.draw(shader, ViewMatrix, ProjectionMatrix);
            rightWall.draw(shader, ViewMatrix, ProjectionMatrix);
            middleWall.draw(shader, ViewMatrix, ProjectionMatrix);

            // ======================
            // DRAW FLOOR
            // ======================
            ModelMatrix = glm::mat4(1.0f);
            ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0.0f, 0.0f, 0.0f));
            ModelMatrix = glm::scale(ModelMatrix, glm::vec3(7.0f, 0.1f, 7.0f));
            MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
            glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
            glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
            floorCube.draw(shader);

            // ======================
            // TORCH
            // ======================
            wallTorch->draw(shader, ViewMatrix, ProjectionMatrix);
        }
        else 
        if (currentRoom == 2)
        {
            if (firstLoad == 1)
            {
                warlockPos = glm::vec3(1.2f, 2.0f, 6.3f);
                knightPos = glm::vec3(-1.2f, 2.0f, 6.3f);
                firstLoad = 0;
            }
            exitPos = glm::vec3(4.0f, 2.0f, -6.9f);

            // room 2 colliders
            colliders.push_back(backWall_2.getAABB());
            colliders.push_back(frontWall_2.getAABB());
            colliders.push_back(leftWall_2_a.getAABB());
            colliders.push_back(leftWall_2_b.getAABB());
            colliders.push_back(leftWall_2_c.getAABB());
            colliders.push_back(rightWall_2_a.getAABB());
            colliders.push_back(rightWall_2_b.getAABB());
            colliders.push_back(rightWall_2_c.getAABB());

            // ======================
            // DRAW WALLS
            // ======================
            backWall_2.draw(shader, ViewMatrix, ProjectionMatrix);
            frontWall_2.draw(shader, ViewMatrix, ProjectionMatrix);
            leftWall_2_a.draw(shader, ViewMatrix, ProjectionMatrix);
            leftWall_2_b.draw(shader, ViewMatrix, ProjectionMatrix);
            leftWall_2_c.draw(shader, ViewMatrix, ProjectionMatrix);
            rightWall_2_a.draw(shader, ViewMatrix, ProjectionMatrix);
            rightWall_2_b.draw(shader, ViewMatrix, ProjectionMatrix);
            rightWall_2_c.draw(shader, ViewMatrix, ProjectionMatrix);

            // ======================
            // DRAW FLOOR
            // ======================
            ModelMatrix = glm::mat4(1.0f);
            ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0.0f, 0.0f, -3.5f));
            ModelMatrix = glm::scale(ModelMatrix, glm::vec3(7.0f, 0.1f, 3.5f));
            MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
            glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
            glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
            floorCube.draw(shader);

            ModelMatrix = glm::mat4(1.0f);
            ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0.0f, 0.0f, 3.5f));
            ModelMatrix = glm::scale(ModelMatrix, glm::vec3(2.0f, 0.1f, 3.5f));
            MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
            glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
            glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
            floorCube.draw(shader);
            
            // ======================
            // DRAW TORCHES
            // ======================
            
            for (int i = 0; i < HALL_TORCH_COUNT; i++) 
            {
                hallTorches[i]->draw(shader, ViewMatrix, ProjectionMatrix);

                static bool eKeyWasPressed = false;

                if (hallTorches[i]->isPlayerClose(activeIsWarlock ? warlockPos : knightPos, 2.0f))
                {
                    if (window.isPressed(GLFW_KEY_E))
                    {
                        // debounce
                        if (!eKeyWasPressed) {
                            hallTorches[i]->toggle();
                            std::cout << ">>> Torch toggled!" << std::endl;
                            eKeyWasPressed = true;
                        }
                    }
                    else {
                        eKeyWasPressed = false;
                    }
                }
            }

            glm::vec3 leftPos = glm::vec3(-6.5f, 2.0f, -0.5f);
            glm::vec3 rightPos = glm::vec3(6.5f, 2.0f, -0.5f);

            float distToLeft = glm::length(warlockPos - leftPos);
            float distToRight = glm::length(knightPos - rightPos);

            if (hallTorches[0]->isOn && !hallTorches[1]->isOn && !hallTorches[2]->isOn
                && !hallTorches[3]->isOn && !hallTorches[4]->isOn && hallTorches[5]->isOn
                && hallTorches[6]->isOn && distToLeft < 1.5f && distToRight < 1.5f)
            {
                isSolved = true;
            }

            if (isSolved == true)
            {
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

                // ======================
                // EXIT INTERACTION
                // ======================
                float distToExit = glm::length(warlockPos - exitPos);
                if (distToExit < 1.0f)
                {
                    firstLoad = 1;
                    currentRoom = 3;
                }
            } 
        }
        else
        if (currentRoom == 3)
        {
            if (firstLoad == 1)
            {
                warlockPos = glm::vec3(1.2f, 2.0f, 6.3f);
                knightPos = glm::vec3(-1.2f, 2.0f, 6.3f);
                firstLoad = 0;
            }
            exitPos = glm::vec3(0.0f, 2.0f, -6.9f);

            // ======================
            // DRAW WALLS
            // ======================
            backWall.draw(shader, ViewMatrix, ProjectionMatrix);
            frontWall.draw(shader, ViewMatrix, ProjectionMatrix);
            leftWall.draw(shader, ViewMatrix, ProjectionMatrix);
            rightWall.draw(shader, ViewMatrix, ProjectionMatrix);

            // ======================
            // DRAW FLOOR
            // ======================
            ModelMatrix = glm::mat4(1.0f);
            ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0.0f, 0.0f, 0.0f));
            ModelMatrix = glm::scale(ModelMatrix, glm::vec3(7.0f, 0.1f, 7.0f));
            MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
            glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
            glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
            floorCube.draw(shader);

            // =====================
            // DRAW BOOKSHELVES
            // =====================
            for (int i = 0; i < BOOKSHELF_COUNT; i++) 
            {
                bookshelves[i].draw(shader, ViewMatrix, ProjectionMatrix);
                colliders.push_back(bookshelves[i].getAABB());
            }

            colliders.push_back(backWall.getAABB());
            colliders.push_back(frontWall.getAABB());
            colliders.push_back(leftWall.getAABB());
            colliders.push_back(rightWall.getAABB());

            for (int i = 0; i < BOOK_COUNT; i++)
            {
                books[i].draw(shader, ViewMatrix, ProjectionMatrix);
            }
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



        // ==================================
        // DRAW UI / DIALOGUE BOX
        // ==================================

        glDisable(GL_DEPTH_TEST);

        // Only draw the box and text if we haven't reached the end of the list
        if (currentLineIndex + 67 < dialogueLines.size())
        {
            // --- Render the Background Box ---
            diagShader.use();

            // Set Model Matrix for UI: Bottom Center
            glm::mat4 boxModel = glm::mat4(1.0f);
            boxModel = glm::translate(boxModel, glm::vec3(window.getWidth() / 2.0f, 100.0f, 0.0f));
            boxModel = glm::scale(boxModel, glm::vec3(1200.0f, 200.0f, 1.0f));

            // Pass "model" uniform
            glUniformMatrix4fv(glGetUniformLocation(diagShader.getId(), "model"), 1, GL_FALSE, &boxModel[0][0]);

            // Pass "color" uniform (Navy Blue)
            glUniform3f(glGetUniformLocation(diagShader.getId(), "color"), 0.1f, 0.15f, 0.5f);

            dialogueBoxMesh.draw(diagShader);

            // --- Render Text On Top ---
            // We use the string from our vector based on the current index
            textRenderer.RenderText(textShader, dialogueLines[currentLineIndex], 700.0f, 85.0f, 0.8f, glm::vec3(1.0f, 1.0f, 1.0f));
        }

        // --- Render Hints (Always visible) ---
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