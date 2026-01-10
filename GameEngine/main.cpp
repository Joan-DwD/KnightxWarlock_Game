#define NOMINMAX // To prevent Windows.h from defining min/max macros

#include "Camera\camera.h"
#include "Shaders\shader.h"
#include "Model Loading\mesh.h"
#include "Model Loading\texture.h"
#include "Model Loading\meshLoaderObj.h"
#include <iostream>
#include <cmath>
#include <map>
#include <string>
#include "TextRenderer.h"
#include "Wall.h"
#include "OctreeCollision.h"
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

// ======================
// RAY CASTING 
// ======================
struct Ray {
    glm::vec3 origin;
    glm::vec3 direction;
};

// Returns distance to intersection, or -1 if no intersection
inline float rayAABBIntersect(const Ray& ray, const AABB& box) {
    glm::vec3 invDir = 1.0f / ray.direction;

    glm::vec3 t1 = (box.min - ray.origin) * invDir;
    glm::vec3 t2 = (box.max - ray.origin) * invDir;

    glm::vec3 tmin = glm::min(t1, t2);
    glm::vec3 tmax = glm::max(t1, t2);

    float tNear = glm::max(glm::max(tmin.x, tmin.y), tmin.z);
    float tFar = glm::min(glm::min(tmax.x, tmax.y), tmax.z);

    // Check if ray intersects box
    if (tNear > tFar || tFar < 0.0f) {
        return -1.0f;
    }

    return tNear > 0.0f ? tNear : tFar;
}

// Convert screen coordinates to world ray
inline Ray screenToWorldRay(double mouseX, double mouseY, int screenWidth, int screenHeight,
    const glm::mat4& projectionMatrix, const glm::mat4& viewMatrix,
    const glm::vec3& cameraPos) {
    // Convert mouse coordinates to normalized device coordinates as in -1 to +1
    float x = (2.0f * static_cast<float>(mouseX)) / screenWidth - 1.0f;
    float y = 1.0f - (2.0f * static_cast<float>(mouseY)) / screenHeight;

    // Create ray in clip space, a point 
    glm::vec4 rayClip(x, y, -1.0f, 1.0f);

	// Convert to eye space so our can travel into the 3D world that is flattened to 2D in our perspective projection
    glm::vec4 rayEye = glm::inverse(projectionMatrix) * rayClip;
	rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f); // this is now a direction vector pointing forward beause we set w = 0

	// Convert to world space so that the ray is relative to the world and not the camera
    glm::vec3 rayWorld = glm::vec3(glm::inverse(viewMatrix) * rayEye);
    rayWorld = glm::normalize(rayWorld);

    Ray ray;
    ray.origin = cameraPos;
    ray.direction = rayWorld;
    return ray;
}

// ======================
// SKYBOX DATA
// ======================
float skyboxVertices[] = {
    // positions          
    -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

    -1.0f,  1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f,  1.0f
};

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
Camera camera(glm::vec3(0.0f, 12.0f, 4.2f)); // Above center of room

// ======================
// LIGHT
// ======================
glm::vec3 lightColor = glm::vec3(0.6f, 0.3f, 0.4f);  // Eerie reddish-purple light for Upside Down
glm::vec3 lightPos = glm::vec3(6.5f, 4.0f, 0.0f);

// ======================
// AUDIO ENGINE (Global for cleanup)
// ======================
ma_engine g_audioEngine;
bool g_audioInitialized = false;

int main()
{
    // ======================
    // AUDIO SETUP 
    // ======================
    ma_result audioResult = ma_engine_init(NULL, &g_audioEngine);
    if (audioResult == MA_SUCCESS) {
        g_audioInitialized = true;
        std::cout << ">>> Audio engine initialized successfully!" << std::endl;
    }
    else {
        std::cerr << ">>> Failed to initialize audio engine!" << std::endl;
    }

    // ======================
    // OPENGL SETUP
    // ======================
    glClearColor(0.02f, 0.0f, 0.05f, 1.0f); 
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
    Shader skyboxShader("Shaders/skybox_vertex.glsl", "Shaders/skybox_fragment.glsl");

    // ======================
    // SKYBOX SETUP
    // ======================
    GLuint skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);

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

    // ======================
    // OCTREE COLLISION SETUP
    // ======================
    // Create world bounds for the octree 
    AABB worldBounds = { glm::vec3(-50.0f, -50.0f, -50.0f), glm::vec3(50.0f, 50.0f, 50.0f) };
    OctreeNode* worldOctree = new OctreeNode(worldBounds);

    // Create colliders for walls and insert into octree
    Collider* backWallCollider = new Collider();
    backWallCollider->box = backWall.getAABB();
    backWallCollider->typeID = COLLIDER_WALL;
    backWallCollider->userData = &backWall;
    worldOctree->insert(backWallCollider);

    Collider* frontWallCollider = new Collider();
    frontWallCollider->box = frontWall.getAABB();
    frontWallCollider->typeID = COLLIDER_WALL;
    frontWallCollider->userData = &frontWall;
    worldOctree->insert(frontWallCollider);

    Collider* leftWallCollider = new Collider();
    leftWallCollider->box = leftWall.getAABB();
    leftWallCollider->typeID = COLLIDER_WALL;
    leftWallCollider->userData = &leftWall;
    worldOctree->insert(leftWallCollider);

    Collider* rightWallCollider = new Collider();
    rightWallCollider->box = rightWall.getAABB();
    rightWallCollider->typeID = COLLIDER_WALL;
    rightWallCollider->userData = &rightWall;
    worldOctree->insert(rightWallCollider);

    Collider* middleWallCollider = new Collider();
    middleWallCollider->box = middleWall.getAABB();
    middleWallCollider->typeID = COLLIDER_WALL;
    middleWallCollider->userData = &middleWall;
    worldOctree->insert(middleWallCollider);

    // ======================
    // OBJECT POSITIONS
    // ======================
    glm::vec3 warlockPos = glm::vec3(3.0f, 2.0f, 3.0f);
    glm::vec3 knightPos = glm::vec3(-3.0f, 2.0f, -3.0f);
    bool activeIsWarlock = true; // start controlling Warlock
    const glm::vec3 pawnHalfSize(0.3f, 1.0f, 0.3f); 

    glm::vec3 keyBasePos = glm::vec3(-3.0f, 1.5f, -3.0f);   
    glm::vec3 keyPos = keyBasePos;
    bool keyCollected = false;
    bool hasKey = false;

    // Key state
    float keyRotation = 0.0f;
    float keyPickupTimer = 0.0f;
    bool keyPickingUp = false;
    const float KEY_PICKUP_DURATION = 1.5f;

    // Create key collider 
    Collider* keyCollider = new Collider();
    keyCollider->box = makeAABB(keyBasePos, glm::vec3(1.0f, 2.5f, 1.0f)); 
    keyCollider->typeID = COLLIDER_KEY;
    keyCollider->active = true;
    worldOctree->insert(keyCollider);

    glm::vec3 doorPos = glm::vec3(5.0f, 2.0f, 0.0f);
    glm::vec3 doorHingePos = glm::vec3(7.0f, 2.0f, 0.0f);  
    bool doorUnlocked = false;

    // Door state
    float doorRotation = 0.0f;
    float doorTargetRotation = 0.0f;
    bool doorOpening = false;
    const float DOOR_OPEN_ANGLE = 90.0f;  // Door opens 90 degrees
    const float DOOR_OPEN_SPEED = 30.0f; 
    bool doorSoundPlayed = false;

    // Create door collider and add to octree
    Collider* doorCollider = new Collider();
    doorCollider->box = makeAABB(doorPos, glm::vec3(2.0f, 2.0f, 0.1f));
    doorCollider->typeID = COLLIDER_DOOR;
    doorCollider->active = true;
    worldOctree->insert(doorCollider);

    glm::vec3 exitPos = glm::vec3(0.0f, 2.0f, -6.9f);

    // Mouse picking state
    bool mouseClickedLastFrame = false;
    const float RAY_MAX_DISTANCE = 100.0f;


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

        // ======================
        // SKYBOX
        // ======================
        glDepthFunc(GL_LEQUAL);  // Change depth function so skybox passes at depth 1.0
        skyboxShader.use();

        // Remove translation from view matrix for skybox (so it stays centered on camera)
        glm::mat4 skyboxView = glm::mat4(glm::mat3(ViewMatrix));
        glm::mat4 skyboxVP = ProjectionMatrix * skyboxView;

        glUniformMatrix4fv(glGetUniformLocation(skyboxShader.getId(), "VP"), 1, GL_FALSE, glm::value_ptr(skyboxVP));
        glUniform1f(glGetUniformLocation(skyboxShader.getId(), "time"), currentFrame);

        // Render skybox cube
        glBindVertexArray(skyboxVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);

        glDepthFunc(GL_LESS);  

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
        // COLLIDER STATE UPDATE
        // ======================
        // Update door collider state based on whether door is unlocked
        doorCollider->active = !doorUnlocked;

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
            movementOctree(warlockPos, delta, pawnHalfSize, worldOctree);
        }
        else
        {
            movementOctree(knightPos, delta, pawnHalfSize, worldOctree);
        }

        // ======================
        // KEY ANIMATION UPDATE
        // ======================
        if (!keyCollected && !keyPickingUp)
        {
            // Floating effect
            float Offset = sin(currentFrame * 2.0f) * 0.15f;
            keyPos = keyBasePos + glm::vec3(0.0f, Offset, 0.0f);

            // rotation
            keyRotation += deltaTime * 90.0f; // 90 degrees per second
            if (keyRotation > 360.0f) keyRotation -= 360.0f;

            // Update collider position
            keyCollider->box = makeAABB(keyPos, glm::vec3(0.5f, 0.5f, 0.5f));
        }

        // Pickup animation
        if (keyPickingUp)
        {
            keyPickupTimer += deltaTime;
            float t = keyPickupTimer / KEY_PICKUP_DURATION;

            if (t >= 1.0f)
            {
                keyCollected = true;
                hasKey = true;
                keyPickingUp = false;
            }
            else
            {
                keyRotation += deltaTime * (180.0f + t * 720.0f);

                // Rise up and shrink
                float easeOut = 1.0f - (1.0f - t) * (1.0f - t);
                keyPos = keyBasePos + glm::vec3(0.0f, easeOut * 3.0f, 0.0f);
            }
        }

        // ======================
        // DOOR ANIMATION UPDATE
        // ======================
        if (doorOpening)
        {
            doorRotation += DOOR_OPEN_SPEED * deltaTime;
            if (doorRotation >= DOOR_OPEN_ANGLE)
            {
                doorRotation = DOOR_OPEN_ANGLE;
                doorOpening = false;
            }
        }

        // ======================
        // RAY CASTING MOUSE PICKING
        // ======================
        glm::vec3 activePos = activeIsWarlock ? warlockPos : knightPos;

        // Get mouse position and check for click
        double mouseX, mouseY;
        window.getMousePos(mouseX, mouseY);
        bool mouseClicked = window.isMousePressed(GLFW_MOUSE_BUTTON_LEFT);

        // Create ray from mouse position
        Ray pickRay = screenToWorldRay(mouseX, mouseY, window.getWidth(), window.getHeight(),
            ProjectionMatrix, ViewMatrix, camera.getCameraPosition());

        // Check what the ray hits
        float hitDistance = RAY_MAX_DISTANCE;

        // Ray cast for key
        if (!keyCollected && !keyPickingUp)
        {
            float keyDist = rayAABBIntersect(pickRay, keyCollider->box);
            if (keyDist > 0.0f && keyDist < RAY_MAX_DISTANCE)
            {
                // Mouse is hovering over key
                if (mouseClicked && !mouseClickedLastFrame)
                {
                    // Check if player is close enough to pick up
                    float distToKey = glm::length(activePos - keyPos);
                    if (distToKey < 5.0f) // Interaction range
                    {
                        keyPickingUp = true;
                        keyPickupTimer = 0.0f;
                        keyCollider->active = false;
                        std::cout << ">>> You picked up the key!" << std::endl;
                    }
                    else
                    {
                        std::cout << ">>> Too far away to pick up the key!" << std::endl;
                    }
                }
            }
        }

        // Ray cast for door
        if (!doorUnlocked && doorCollider->active)
        {
            float doorDist = rayAABBIntersect(pickRay, doorCollider->box);
            if (doorDist > 0.0f && doorDist < RAY_MAX_DISTANCE)
            {
                // Mouse is hovering over door
                if (mouseClicked && !mouseClickedLastFrame)
                {
                    // Check if player is close enough to interact
                    float distToDoor = glm::length(activePos - doorPos);
                    if (distToDoor < 6.0f) // Interaction range
                    {
                        if (hasKey)
                        {
                            doorUnlocked = true;
                            doorOpening = true;
                            doorCollider->active = false;

                            // Play creaky door sound
                            if (g_audioInitialized && !doorSoundPlayed)
                            {
                                ma_engine_play_sound(&g_audioEngine, "audio/creakydoor.wav", NULL);
                                doorSoundPlayed = true;
                                std::cout << ">>> *CREEEEAK* The door swings open!" << std::endl;
                            }
                            std::cout << ">>> You are free to leave..." << std::endl;
                        }
                        else
                        {
                            std::cout << ">>> The door is locked. You need a key." << std::endl;
                        }
                    }
                    else
                    {
                        std::cout << ">>> Too far away to reach the door!" << std::endl;
                    }
                }
            }
        }

        mouseClickedLastFrame = mouseClicked;

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
            ModelMatrix = glm::rotate(ModelMatrix, keyRotation, glm::vec3(0.0f, 1.0f, 0.0f));

            float keyScale = 0.02f;
            if (keyPickingUp)
            {
                float t = keyPickupTimer / KEY_PICKUP_DURATION;
                float scaleMod = 1.0f + sin(t * 3.14159f) * 0.5f - t * t;
                keyScale *= std::max(0.0f, scaleMod);
            }
            ModelMatrix = glm::scale(ModelMatrix, glm::vec3(keyScale));

            MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
            glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
            glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
            keyMesh.draw(shader);
        }

        // ======================
        // DRAW DOOR with animation
        // ======================
        {
            ModelMatrix = glm::mat4(1.0f);

            // Door rotates around its hinge (right edge, near the wall)
            // First translate to hinge position
            ModelMatrix = glm::translate(ModelMatrix, doorHingePos);

            // Rotate around Y axis (hinge)
            ModelMatrix = glm::rotate(ModelMatrix, doorRotation, glm::vec3(0.0f, 1.0f, 0.0f));

            // Translate back so door center is correct relative to hinge
            // Door is 2 units wide (half-width = 2), hinge is at edge
            ModelMatrix = glm::translate(ModelMatrix, glm::vec3(-2.0f, 0.0f, 0.0f));

            ModelMatrix = glm::scale(ModelMatrix, glm::vec3(2.0f, 2.0f, 0.1f));
            MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
            glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
            glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
            doorMesh.draw(shader);
        }

        // ======================
        // DRAW DOOR
        // ======================
        ModelMatrix = glm::mat4(1.0f);
        ModelMatrix = glm::translate(ModelMatrix, exitPos);
        ModelMatrix = glm::scale(ModelMatrix, glm::vec3(2.0f, 2.0f, 0.1f));
        MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
        glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
        glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
        doorMesh.draw(shader);


        if (!hasKey) {
            textRenderer.RenderText(textShader, "Find the Key...", 25.0f, 1000.0f, 0.8f, glm::vec3(1.0f, 1.0f, 1.0f));
        }
        else if (!doorUnlocked) {
            textRenderer.RenderText(textShader, "Go to the Door!", 25.0f, 1000.0f, 0.8f, glm::vec3(0.2f, 1.0f, 0.2f));
        }
        else {
            textRenderer.RenderText(textShader, "YOU ESCAPED!", 25.0f, 1000.0f, 0.8f, glm::vec3(1.0f, 0.8f, 0.0f));
        }

        window.update();
    }

    // ======================
    // CLEANUP
    // ======================
    // Cleanup audio engine
    if (g_audioInitialized) {
        ma_engine_uninit(&g_audioEngine);
        std::cout << ">>> Audio engine shut down." << std::endl;
    }

    // Delete colliders
    delete backWallCollider;
    delete frontWallCollider;
    delete leftWallCollider;
    delete rightWallCollider;
    delete middleWallCollider;
    delete keyCollider;
    delete doorCollider;

    // Delete octree (this also cleans up internal nodes)
    delete worldOctree;

    // Cleanup skybox
    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteBuffers(1, &skyboxVBO);

    return 0;
}


// ======================
// CAMERA INPUT 
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