#include "Camera\camera.h"
#include "Shaders\shader.h"
#include "Model Loading\mesh.h"
#include "Model Loading\texture.h"
#include "Model Loading\meshLoaderObj.h"
#include <iostream>
#include <cmath>
#include <map>
#include <string>
#include <fstream>
#include <sstream>

// --- Custom Class Includes ---
#include "TextRenderer.h"
#include "Wall.h"
#include "Torch.h"
#include "Collision.h"

// ======================
// GLOBAL QUEST HANDLING VARIABLES
// ======================

int currentTask = 1;
int currentRoom = 5;
int firstLoad = 1;

// ======================
// DIALOGUE SYSTEM STATE
// ======================

struct DialogueLine {
    std::string CharacterName;
    std::string Text;
};

std::vector<DialogueLine> currentDialogue;
size_t currentLineIndex = 0;
bool rPressedLastFrame = false;

// ======================
// FUNCTION DECLARATIONS
// ======================
void processKeyboardInput(); // we dont even use this anymore

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
Camera camera(glm::vec3(0.0f, 15.0f, 5.0f));

// ======================
// LIGHT
// ======================
glm::vec3 lightColor = glm::vec3(0.8f, 0.6f, 0.4f);
glm::vec3 lightPos = glm::vec3(0.0f, 6.5f, 1.0f);


// =======================
// DRAWING FUNCTION
// =======================

void drawObject(Mesh& mesh, glm::vec3 position, glm::vec3 scale, Shader& shader, glm::mat4 viewMatrix, glm::mat4 projectionMatrix, float rotation) {
    // Calculate Model Matrix
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::rotate(model, rotation, glm::vec3(0, 1, 0));
    model = glm::scale(model, scale);

    glm::mat4 mvp = projectionMatrix * viewMatrix * model;

    GLuint matrixID = glGetUniformLocation(shader.getId(), "MVP");
    GLuint modelID = glGetUniformLocation(shader.getId(), "model");

    // Send to shader
    glUniformMatrix4fv(matrixID, 1, GL_FALSE, &mvp[0][0]);
    glUniformMatrix4fv(modelID, 1, GL_FALSE, &model[0][0]);

    // Draw the mesh
    mesh.draw(shader);
}

// =======================
// DIALOGUE FUNCTION
// =======================

void LoadDialogue(int taskId) {
    currentDialogue.clear();
    currentLineIndex = 0;

    // Construct filename: e.g: "Dialogue/dialogue_0.txt"
    std::string filename = "Dialogue/dialogue_" + std::to_string(taskId) + ".txt";

    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cout << "ERROR::DIALOGUE: Could not open file " << filename << std::endl;
        // Fallback line so the game doesn't break
        currentDialogue.push_back({ "System", "Error loading dialogue file." });
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string name;
        std::string text;

        // 1. Read first word as Character Name
        ss >> name;

        // 2. Read the rest of the line as the Dialogue Text
        std::getline(ss, text);

        // Remove leading space from text (leftover from >> operator)
        if (!text.empty() && text[0] == ' ') {
            text = text.substr(1);
        }

        currentDialogue.push_back({ name, text });
    }
    file.close();
    std::cout << "Loaded Dialogue Task " << taskId << ": " << currentDialogue.size() << " lines." << std::endl;
}

// final door puzzle struct

struct puzzleDoor
{
    bool isUnlocked;
    glm::vec3 position;
    glm::vec3 scale;
};

struct frogSwitch
{
    bool isPressed;
    glm::vec3 position;
    glm::vec3 scale;

    std::vector<int> links;
};

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
    //GLuint woodTex = loadBMP("Resources/Textures/wood.bmp");
    //GLuint rockTex = loadBMP("Resources/Textures/rock.bmp");
    //GLuint orangeTex = loadBMP("Resources/Textures/orange.bmp");
    //GLuint purpleTex = loadBMP("Resources/Textures/purple.bmp");
    //GLuint goldTex = loadBMP("Resources/Textures/gold.bmp");
    //GLuint bookTex = loadBMP("Resources/Textures/books.bmp");

    //std::vector<Texture> woodTextures = { { woodTex, "texture_diffuse" } };
    //std::vector<Texture> stoneTextures = { { rockTex, "texture_diffuse" } };
    //std::vector<Texture> orangeTextures = { { orangeTex, "texture_diffuse" } };
    //std::vector<Texture> purpleTextures = { { purpleTex, "texture_diffuse" } };
    //std::vector<Texture> goldTextures = { { goldTex, "texture_diffuse" } };
    //std::vector<Texture> bookTextures = { { bookTex, "texture_diffuse" } };

    GLuint paint_beige = loadBMP("Resources/Textures/PAINT_BEIGE.bmp");
    GLuint paint_black = loadBMP("Resources/Textures/PAINT_BLACK.bmp");
    GLuint paint_blue = loadBMP("Resources/Textures/PAINT_BLUE.bmp");
    GLuint paint_cyan = loadBMP("Resources/Textures/PAINT_CYAN.bmp");
    GLuint paint_darkblue = loadBMP("Resources/Textures/PAINT_DARKBLUE.bmp");
    GLuint paint_darkbrown = loadBMP("Resources/Textures/PAINT_DARKBROWN.bmp");
    GLuint paint_darkgray = loadBMP("Resources/Textures/PAINT_DARKGRAY.bmp");
    GLuint paint_gold = loadBMP("Resources/Textures/PAINT_GOLD.bmp");
    GLuint paint_green = loadBMP("Resources/Textures/PAINT_GREEN.bmp");
    GLuint paint_lavender = loadBMP("Resources/Textures/PAINT_LAVENDER.bmp");
    GLuint paint_lightblue = loadBMP("Resources/Textures/PAINT_LIGHTBLUE.bmp");
    GLuint paint_lightbrown = loadBMP("Resources/Textures/PAINT_LIGHTBROWN.bmp");
    GLuint paint_lightgray = loadBMP("Resources/Textures/PAINT_LIGHTGRAY.bmp");
    GLuint paint_lime = loadBMP("Resources/Textures/PAINT_LIME.bmp");
    GLuint paint_orange = loadBMP("Resources/Textures/PAINT_ORANGE.bmp");
    GLuint paint_pink = loadBMP("Resources/Textures/PAINT_PINK.bmp");
    GLuint paint_purple = loadBMP("Resources/Textures/PAINT_PURPLE.bmp");
    GLuint paint_red = loadBMP("Resources/Textures/PAINT_RED.bmp");
    GLuint paint_white = loadBMP("Resources/Textures/PAINT_WHITE.bmp");
    GLuint paint_yellow = loadBMP("Resources/Textures/PAINT_YELLOW.bmp");
    // Character portraits
    std::map<std::string, GLuint> portraits;
    portraits["Warlock"] = loadBMP("Resources/Textures/PAINT_PURPLE.bmp");
    portraits["Knight"] = loadBMP("Resources/Textures/PAINT_GOLD.bmp");
    portraits["Princess"] = loadBMP("Resources/Textures/PAINT_PINK.bmp");
    Shader portraitShader("Shaders/ui_texture_vertex.glsl", "Shaders/ui_texture_fragment.glsl");

    std::vector<Texture> paint_beige_texture = { { paint_beige, "texture_difuse" } };
    std::vector<Texture> paint_black_texture = { { paint_black, "texture_difuse" } };
    std::vector<Texture> paint_blue_texture = { { paint_blue, "texture_difuse" } };
    std::vector<Texture> paint_cyan_texture = { { paint_cyan, "texture_difuse" } };
    std::vector<Texture> paint_darkblue_texture = { { paint_darkblue, "texture_difuse" } };
    std::vector<Texture> paint_darkbrown_texture = { { paint_darkbrown, "texture_difuse" } };
    std::vector<Texture> paint_darkgray_texture = { { paint_darkgray, "texture_difuse" } };
    std::vector<Texture> paint_gold_texture = { { paint_gold, "texture_difuse" } };
    std::vector<Texture> paint_green_texture = { { paint_green, "texture_difuse" } };
    std::vector<Texture> paint_lavender_texture = { { paint_lavender, "texture_difuse" } };
    std::vector<Texture> paint_lightblue_texture = { { paint_lightblue, "texture_difuse" } };
    std::vector<Texture> paint_lightbrown_texture = { { paint_lightbrown, "texture_difuse" } };
    std::vector<Texture> paint_lightgray_texture = { { paint_lightgray, "texture_difuse" } };
    std::vector<Texture> paint_lime_texture = { { paint_lime, "texture_difuse" } };
    std::vector<Texture> paint_orange_texture = { { paint_orange, "texture_difuse" } };
    std::vector<Texture> paint_pink_texture = { { paint_pink, "texture_difuse" } };
    std::vector<Texture> paint_purple_texture = { { paint_purple, "texture_difuse" } };
    std::vector<Texture> paint_red_texture = { { paint_red, "texture_difuse" } };
    std::vector<Texture> paint_white_texture = { { paint_white, "texture_difuse" } };
    std::vector<Texture> paint_yellow_texture = { { paint_yellow, "texture_difuse" } };

    // ======================
    // LOAD MODELS
    // ======================
    MeshLoaderObj loader;

    // walls and floors
    Mesh wallCube = loader.loadObj("Resources/Models/cube.obj", paint_darkgray_texture);
    Mesh floorCube = loader.loadObj("Resources/Models/cube.obj", paint_darkgray_texture);
    Mesh prisonWall = loader.loadObj("Resources/Models/cube.obj", paint_black_texture);
    Mesh gardenFloorCube = loader.loadObj("Resources/Models/cube.obj", paint_lime_texture);

    // books
    Mesh bookcaseCube = loader.loadObj("Resources/Models/cube.obj", paint_darkbrown_texture);
    Mesh booksCube = loader.loadObj("Resources/Models/cube.obj", paint_yellow_texture);

    // garden things
    Mesh tree = loader.loadObj("Resources/Models/tree.obj", paint_green_texture);
    Mesh frog = loader.loadObj("Resources/Models/frog.obj", paint_orange_texture);

    // wardrobe buttons
    Mesh frogButton = loader.loadObj("Resources/Models/frog_button.obj", paint_gold_texture);

    // Pawns
    Mesh warlock = loader.loadObj("Resources/Models/pawn.obj", paint_purple_texture);
    Mesh knight = loader.loadObj("Resources/Models/pawn.obj", paint_gold_texture);

    // Key and door
    Mesh keyMesh = loader.loadObj("Resources/Models/key.obj", paint_lavender_texture);
    Mesh doorMesh = loader.loadObj("Resources/Models/cube.obj", paint_red_texture);

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
    Wall frontWall_2(&wallCube, glm::vec3(0.0f, 3.5f, -7.0f), glm::vec3(7.0f, 3.5f, 0.1f));
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
     Wall(&bookcaseCube, glm::vec3(-3.5f, 2.0f, -6.0f), glm::vec3(2.0f, 2.0f, 1.0f)),
     Wall(&bookcaseCube, glm::vec3(-3.5f, 2.0f, -2.0f), glm::vec3(2.0f, 2.0f, 1.0f)),
     Wall(&bookcaseCube, glm::vec3(-3.5f, 2.0f, 2.0f), glm::vec3(2.0f, 2.0f, 1.0f)),
     Wall(&bookcaseCube, glm::vec3(3.5f, 2.0f, -6.0f), glm::vec3(2.0f, 2.0f, 1.0f)),
     Wall(&bookcaseCube, glm::vec3(3.5f, 2.0f, -2.0f), glm::vec3(2.0f, 2.0f, 1.0f)),
     Wall(&bookcaseCube, glm::vec3(3.5f, 2.0f, 2.0f), glm::vec3(2.0f, 2.0f, 1.0f))
    };

    Wall books[]
    {
        Wall(&booksCube, glm::vec3(-3.5, 2.0f, -4.9f), glm::vec3(1.8f, 1.8f, 0.1f)),
        Wall(&booksCube, glm::vec3(-3.5, 2.0f, -0.9f), glm::vec3(1.8f, 1.8f, 0.1f)),
        Wall(&booksCube, glm::vec3(-3.5, 2.0f, 3.1f), glm::vec3(1.8f, 1.8f, 0.1f)),
        Wall(&booksCube, glm::vec3(3.5, 2.0f, -4.9f), glm::vec3(1.8f, 1.8f, 0.1f)),
        Wall(&booksCube, glm::vec3(3.5, 2.0f, -0.9f), glm::vec3(1.8f, 1.8f, 0.1f)),
        Wall(&booksCube, glm::vec3(3.5, 2.0f, 3.1f), glm::vec3(1.8f, 1.8f, 0.1f)),
    }; // lowkirkenuinely might remove this

    const int BOOKSHELF_COUNT = sizeof(bookshelves) / sizeof(bookshelves[0]);
    const int BOOK_COUNT = sizeof(books) / sizeof(books[0]);

    // ====================
    // WALLS FOR ROOM 4 - GARDEN
    // ====================

    // Back wall
    Wall backWall_4(&wallCube, glm::vec3(0.0f, 2.0f, 10.0f), glm::vec3(10.0f, 2.0f, 0.1f));
    // Front wall
    Wall frontWall_4(&wallCube, glm::vec3(0.0f, 2.0f, -10.0f), glm::vec3(10.0f, 2.0f, 0.1f));
    // Left wall
    Wall leftWall_4(&wallCube, glm::vec3(-10.0f, 2.0f, 0.0f), glm::vec3(0.1f, 2.0f, 10.0f));
    // Right wall
    Wall rightWall_4(&wallCube, glm::vec3(10.0f, 2.0f, 0.0f), glm::vec3(0.1f, 2.0f, 10.0f));

    // ====================
    // DOORS AND BUTTONS FOR ROOM 5 - WARDROBE
    // ====================

    Wall wall_5_a(&wallCube, glm::vec3(2.0f, 2.0f, -3.0f), glm::vec3(5.0f, 2.0f, 0.1f));
    Wall wall_5_b(&wallCube, glm::vec3(-2.0f, 2.0f, 1.0f), glm::vec3(5.0f, 2.0f, 0.1f));
    Wall wall_5_c(&wallCube, glm::vec3(2.0f, 2.0f, 5.0f), glm::vec3(5.0f, 2.0f, 0.1f));

    puzzleDoor doors[8] = 
    {
        { false, glm::vec3(-5.0f, 2.0f, 5.0f), glm::vec3(2.0f, 2.0f, 0.1f) },
        { false, glm::vec3(-3.0f, 2.0f, 3.0f), glm::vec3(0.1f, 2.0f, 2.0f) },
        { false, glm::vec3(3.0f, 2.0f, 3.0f), glm::vec3(0.1f, 2.0f, 2.0f) },
        { false, glm::vec3(5.0f, 2.0f, 1.0f), glm::vec3(2.0f, 2.0f, 0.1f) },
        { false, glm::vec3(3.0f, 2.0f, -1.0f), glm::vec3(0.1f, 2.0f, 2.0f) },
        { false, glm::vec3(-3.0f, 2.0f, -1.0f), glm::vec3(0.1f, 2.0f, 2.0f) },
        { false, glm::vec3(-5.0f, 2.0f, -3.0f), glm::vec3(2.0f, 2.0f, 0.1f) },
        { false, glm::vec3(-3.0f, 2.0f, -5.0f), glm::vec3(0.1f, 2.0f, 2.0f) }
    };

    // FROG PUZZLE

    frogSwitch buttons[7] =
    {
        { false, glm::vec3(6.0f, 2.0f, -6.8f), glm::vec3(0.2f), {0, 1, 2, 3, 4, 5, 6, 7}},
        { false, glm::vec3(-6.0f, 2.0f, -6.8f), glm::vec3(0.2f), {0, 1, 2, 3, 4, 5, 6} },
        { false, glm::vec3(6.0f, 2.0f, -2.8f), glm::vec3(0.2f), {2, 3, 4} },
        { false, glm::vec3(-1.0f, 2.0f, -2.8f), glm::vec3(0.2f), {4, 5, 6} },
        { false, glm::vec3(1.0f, 2.0f, 1.2f), glm::vec3(0.2f), {1, 2, 3} },
        { false, glm::vec3(-6.0f, 2.0f, 1.2f), glm::vec3(0.2f), {0, 1} },
        { false, glm::vec3(6.0f, 2.0f, 5.2f), glm::vec3(0.2f), {0, 7} }
    };

    const int DOOR_COUNT = sizeof(doors) / sizeof(doors[0]);
    const int BUTTON_COUNT = sizeof(buttons) / sizeof(buttons[0]);

    // ======================
    // TORCH
    // ======================
    Torch* wallTorch = nullptr;

    // currently on left wall a bit below cell
    wallTorch = new Torch(&wallCube, &wallCube, glm::vec3(-6.8, 3.0f, 2.0f), 180.0f);

    Mesh flameCube = loader.loadObj("Resources/Models/cube.obj", paint_orange_texture);
    Mesh stickCube = loader.loadObj("Resources/Models/cube.obj", paint_darkbrown_texture);

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

    // Setup Dialogue Shader
    diagShader.use();
    glUniformMatrix4fv(glGetUniformLocation(diagShader.getId(), "projection"), 1, GL_FALSE, glm::value_ptr(textProjection));

    // ======================
    // IMPORTANT VARIABLES
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

    bool isSolved_torch = false; // room 2 puzzle
    bool isSolved_books = false; // NEEDS ACTUAL PUZZLE LOL
    bool isSolved_frog = true; //TO BE ADDED
    bool isSolved_wardrobe = true; // TO BE ADDED

    // frog stuff
    float frogRadius = 5.0f;
    float frogHeight = 3.0f;
    float frogWalkSpeed = 2.0f;
    float frogJumpSpeed = 7.0f;
    float frogTime = 0.0f;


    glm::vec3 exitPos;

    // Load dialogue
    LoadDialogue(0);

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
                // Only increment if we haven't finished the dialogue yet
                if (currentLineIndex < currentDialogue.size()) {
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
        drawObject(warlock, warlockPos, glm::vec3(0.5f, 0.5f, 0.5f), shader, ViewMatrix, ProjectionMatrix, 0.0f);

        // Knight
        drawObject(knight, knightPos, glm::vec3(0.6f, 0.6f, 0.6f), shader, ViewMatrix, ProjectionMatrix, 0.0f);

        // active character position
        glm::vec3 activePos = activeIsWarlock ? warlockPos : knightPos;

        if (currentRoom == 1)
        {
            if (firstLoad == 1)
            {
                warlockPos = glm::vec3(3.0f, 0.5f, 3.0f);
                knightPos = glm::vec3(3.0f, 0.6f, -3.0f);
                firstLoad = 0;
            }

            exitPos = glm::vec3(0.0f, 2.0f, -6.8f);

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
            if (distToDoor < 2.0f)
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
                drawObject(keyMesh, keyPos, glm::vec3(0.02f), shader, ViewMatrix, ProjectionMatrix, 0.0f);
            }

            // ======================
            // DRAW DOOR
            // ======================
            if (!doorUnlocked)
            {
                drawObject(doorMesh, doorPos, glm::vec3(2.0f, 2.0f, 0.1f), shader, ViewMatrix, ProjectionMatrix, 0.0f);
            }

            // ======================
            // DRAW EXIT
            // ======================

            drawObject(doorMesh, exitPos, glm::vec3(2.0f, 2.0f, 0.1f), shader, ViewMatrix, ProjectionMatrix, 0.0f);

            // ======================
            // EXIT INTERACTION
            // ======================
            float distToExit = glm::length(warlockPos - exitPos);
            if (distToExit < 2.0f)
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

            drawObject(floorCube, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(7.0f, 0.1f, 7.0f), shader, ViewMatrix, ProjectionMatrix, 0.0f);

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
                warlockPos = glm::vec3(1.2f, 0.5f, 6.3f);
                knightPos = glm::vec3(-1.2f, 0.6f, 6.3f);
                firstLoad = 0;
            }
            exitPos = glm::vec3(4.0f, 2.0f, -6.8f);

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
            // DRAW FLOOR (T shape)
            // ======================

            drawObject(floorCube, glm::vec3(0.0f, 0.0f, -3.5f), glm::vec3(7.0f, 0.1f, 3.5f), shader, ViewMatrix, ProjectionMatrix, 0.0f);

            drawObject(floorCube, glm::vec3(0.0f, 0.0f, 3.5f), glm::vec3(2.0f, 0.1f, 3.5f), shader, ViewMatrix, ProjectionMatrix, 0.0f);
            
            // ======================
            // DRAW TORCHES
            // ======================
            
            for (int i = 0; i < HALL_TORCH_COUNT; i++) 
            {
                hallTorches[i]->draw(shader, ViewMatrix, ProjectionMatrix);

                static bool eKeyWasPressed = false;

                if (hallTorches[i]->isPlayerClose(activeIsWarlock ? warlockPos : knightPos, 3.0f))
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

            glm::vec3 leftPos = glm::vec3(-6.5f, 0.5f, -0.5f);
            glm::vec3 rightPos = glm::vec3(6.5f, 0.6f, -0.5f);

            float distToLeft = glm::length(warlockPos - leftPos);
            float distToRight = glm::length(knightPos - rightPos);

            if (hallTorches[0]->isOn && !hallTorches[1]->isOn && !hallTorches[2]->isOn
                && !hallTorches[3]->isOn && !hallTorches[4]->isOn && hallTorches[5]->isOn
                && hallTorches[6]->isOn && distToLeft < 1.5f && distToRight < 1.5f)
            {
                isSolved_torch = true;
            }

            if (isSolved_torch == true)
            {
                // ======================
                // DRAW EXIT
                // ======================

                drawObject(doorMesh, exitPos, glm::vec3(2.0f, 2.0f, 0.1f), shader, ViewMatrix, ProjectionMatrix, 0.0f);

                // ======================
                // EXIT INTERACTION
                // ======================
                float distToExit = glm::length(warlockPos - exitPos);
                if (distToExit < 2.0f)
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
                warlockPos = glm::vec3(1.2f, 0.5f, 6.3f);
                knightPos = glm::vec3(-1.2f, 0.6f, 6.3f);
                firstLoad = 0;
            }
            exitPos = glm::vec3(-3.5f, 2.0f, -6.8f);

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

            drawObject(floorCube, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(7.0f, 0.1f, 7.0f), shader, ViewMatrix, ProjectionMatrix, 0.0f);

            // =====================
            // DRAW BOOKSHELVES
            // =====================
            if (!isSolved_books)
            {
                bookshelves[0].draw(shader, ViewMatrix, ProjectionMatrix);
                books[0].draw(shader, ViewMatrix, ProjectionMatrix);
                colliders.push_back(bookshelves[0].getAABB());
            }
            for (int i = 1; i < BOOKSHELF_COUNT; i++) 
            {
                bookshelves[i].draw(shader, ViewMatrix, ProjectionMatrix);
                colliders.push_back(bookshelves[i].getAABB());
            }

            colliders.push_back(backWall.getAABB());
            colliders.push_back(frontWall.getAABB());
            colliders.push_back(leftWall.getAABB());
            colliders.push_back(rightWall.getAABB());

            for (int i = 1; i < BOOK_COUNT; i++)
            {
                books[i].draw(shader, ViewMatrix, ProjectionMatrix);
            }

            glm::vec3 endPos = glm::vec3(7.0f, 2.0f, -7.0f);

            float distToEnd = glm::length(warlockPos - endPos);

            if (distToEnd < 2.0f)
                isSolved_books = true;

            if (isSolved_books == true)
            {
                // ======================
                // DRAW EXIT
                // ======================

                drawObject(doorMesh, exitPos, glm::vec3(2.0f, 2.0f, 0.1f), shader, ViewMatrix, ProjectionMatrix, 0.0f);

                // ======================
                // EXIT INTERACTION
                // ======================
                float distToExit = glm::length(warlockPos - exitPos);
                if (distToExit < 2.0f)
                {
                    firstLoad = 1;
                    currentRoom = 4;
                }
            }
        }
        else
        if (currentRoom == 4)
        {
            if (firstLoad == 1)
            {
                warlockPos = glm::vec3(1.2f, 0.5f, 9.3f);
                knightPos = glm::vec3(-1.2f, 0.6f, 9.3f);
                firstLoad = 0;
            }
            exitPos = glm::vec3(0.0f, 2.0f, -9.8f);

            // ======================
            // DRAW FLOOR
            // ======================

            drawObject(gardenFloorCube, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(10.0f, 0.1f, 10.0f), shader, ViewMatrix, ProjectionMatrix, 0.0f);

            // ======================
            // DRAW WALLS
            // ======================
            backWall_4.draw(shader, ViewMatrix, ProjectionMatrix);
            frontWall_4.draw(shader, ViewMatrix, ProjectionMatrix);
            leftWall_4.draw(shader, ViewMatrix, ProjectionMatrix);
            rightWall_4.draw(shader, ViewMatrix, ProjectionMatrix);

            colliders.push_back(backWall_4.getAABB());
            colliders.push_back(frontWall_4.getAABB());
            colliders.push_back(leftWall_4.getAABB());
            colliders.push_back(rightWall_4.getAABB());

            // ======================
            // DRAW TREES
            // ======================
            for (int i = 0; i < 10; i++)
            {
                glm::vec3 treePos;
                glm::vec3 treeScale;
                float treeRotation;

                switch (i)
                {
                case 0:
                    treePos = glm::vec3(1.0f, 0.0f, 6.0f); treeScale = glm::vec3(0.05f); treeRotation = 0.0f;
                    break;
                case 1:
                    treePos = glm::vec3(5.0f, 0.0f, 7.0f); treeScale = glm::vec3(0.04f); treeRotation = 0.3f;
                    break;
                case 2:
                    treePos = glm::vec3(-4.0f, 0.0f, -3.0f); treeScale = glm::vec3(0.06f); treeRotation = 0.8f;
                    break;
                case 3:
                    treePos = glm::vec3(-8.0f, 0.0f, -4.0f); treeScale = glm::vec3(0.05f); treeRotation = 0.4f;
                    break;
                case 4:
                    treePos = glm::vec3(3.0f, 0.0f, -7.0f); treeScale = glm::vec3(0.04f); treeRotation = 0.1f;
                    break;
                case 5:
                    treePos = glm::vec3(9.0f, 0.0f, -6.0f); treeScale = glm::vec3(0.06f); treeRotation = 0.8f;
                    break;
                case 6:
                    treePos = glm::vec3(-8.0f, 0.0f, 5.0f); treeScale = glm::vec3(0.05f); treeRotation = 0.2f;
                    break;
                case 7:
                    treePos = glm::vec3(-2.0f, 0.0f, 8.0f); treeScale = glm::vec3(0.04f); treeRotation = 0.9f;
                    break;
                case 8:
                    treePos = glm::vec3(0.0f, 0.0f, 0.0f); treeScale = glm::vec3(0.05f); treeRotation = 0.5f;
                    break;
                case 9:
                    treePos = glm::vec3(2.0f, 0.0f, -2.0f); treeScale = glm::vec3(0.06f); treeRotation = 0.7f;
                    break;
                default:
                    treePos = glm::vec3(0.0f);
                    treeScale = glm::vec3(0.05f);
                    treeRotation = 0.0f;
                    break;
                }

                drawObject(tree, treePos, treeScale, shader, ViewMatrix, ProjectionMatrix, treeRotation);
                colliders.push_back(makeAABB(treePos, glm::vec3(0.5f, 2.0f, 0.5f)));
            }
            

            // =======================
            // FROG
            // =======================
            frogTime += deltaTime;

            float frogAngle = frogWalkSpeed * frogTime;
            float frog_x = frogRadius * cos(frogAngle);
            float frog_z = frogRadius * sin(frogAngle);
            float frog_y = frogHeight * abs(sin(frogJumpSpeed * frogTime));
            float frogRotation = frogAngle + glm::half_pi<float>();

            glm::vec3 frogPos(frog_x, frog_y + 0.2f, frog_z);
            glm::vec3 frogScale(0.2f, 0.2f, 0.2f);

            drawObject(frog, frogPos, frogScale, shader, ViewMatrix, ProjectionMatrix, frogRotation * -58 - glm::half_pi<float>());

            colliders.push_back(makeAABB(frogPos, glm::vec3(1.0f, 1.0f, 1.0f)));

            if (isSolved_frog)
            {
                // ======================
                // DRAW EXIT
                // ======================

                drawObject(doorMesh, exitPos, glm::vec3(2.0f, 2.0f, 0.1f), shader, ViewMatrix, ProjectionMatrix, 0.0f);

                // ======================
                // EXIT INTERACTION
                // ======================
                float distToExit = glm::length(warlockPos - exitPos);
                if (distToExit < 2.0f)
                {
                    firstLoad = 1;
                    currentRoom = 5;
                }
            }

        }
        else
        if (currentRoom == 5)
        {
            if (firstLoad == 1)
            {
                warlockPos = glm::vec3(1.2f, 0.5f, 6.3f);
                knightPos = glm::vec3(-1.2f, 0.6f, 6.3f);
                firstLoad = 0;
            }
            exitPos = glm::vec3(0.0f, 2.0f, -6.8f);

            // ======================
            // DRAW FLOOR
            // ======================

            drawObject(floorCube, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(7.0f, 0.1f, 7.0f), shader, ViewMatrix, ProjectionMatrix, 0.0f);

            // ======================
            // DRAW WALLS
            // ======================
            backWall.draw(shader, ViewMatrix, ProjectionMatrix);
            frontWall.draw(shader, ViewMatrix, ProjectionMatrix);
            leftWall.draw(shader, ViewMatrix, ProjectionMatrix);
            rightWall.draw(shader, ViewMatrix, ProjectionMatrix);
            wall_5_a.draw(shader, ViewMatrix, ProjectionMatrix);
            wall_5_b.draw(shader, ViewMatrix, ProjectionMatrix);
            wall_5_c.draw(shader, ViewMatrix, ProjectionMatrix);

            colliders.push_back(backWall.getAABB());
            colliders.push_back(frontWall.getAABB());
            colliders.push_back(leftWall.getAABB());
            colliders.push_back(rightWall.getAABB());
            colliders.push_back(wall_5_a.getAABB());
            colliders.push_back(wall_5_b.getAABB());
            colliders.push_back(wall_5_c.getAABB());

            // ==================
            // DRAW DOORS LOCK PUZZLE
            // ==================

            for (int i = 0; i < DOOR_COUNT; i++)
            {
                if (doors[i].isUnlocked == false)
                {
                    drawObject(doorMesh, doors[i].position, doors[i].scale, shader, ViewMatrix, ProjectionMatrix, 0.0f);
                    colliders.push_back(makeAABB(doors[i].position, doors[i].scale));
                }
                doors[i].isUnlocked = false;
            }

            // =======================
            // FROG BUTTONS
            // =======================

            for (int i = 0; i < BUTTON_COUNT; i++)
            {
                drawObject(frogButton, buttons[i].position, buttons[i].scale, shader, ViewMatrix, ProjectionMatrix, 0.0f);

                float distToButton_W = glm::length(warlockPos - buttons[i].position);
                float distToButton_K = glm::length(knightPos - buttons[i].position);
                if (distToButton_W < 2.0f || distToButton_K < 2.0f)
                {
                    for (int link : buttons[i].links)
                    {
                        doors[link].isUnlocked = true;
                    }
                }
            }

            if (isSolved_wardrobe)
            {
                // ======================
                // DRAW EXIT
                // ======================

                drawObject(doorMesh, exitPos, glm::vec3(2.0f, 2.0f, 0.1f), shader, ViewMatrix, ProjectionMatrix, 0.0f);

                // ======================
                // EXIT INTERACTION
                // ======================
                float distToExit = glm::length(warlockPos - exitPos);
                if (distToExit < 2.0f)
                {
                    firstLoad = 1;
                    currentRoom = 5;
                }
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

        // Only render if we have lines left
        if (!currentDialogue.empty() && currentLineIndex < currentDialogue.size())
        {
            // Get current line data
            DialogueLine& line = currentDialogue[currentLineIndex];

            // --------------------------
            // 1. Draw Background Box
            // --------------------------
            diagShader.use();
            // Pass Color (Navy)
            glUniform3f(glGetUniformLocation(diagShader.getId(), "color"), 0.1f, 0.15f, 0.5f);


            // Use your helper function!
            // ViewMatrix is Identity (glm::mat4(1.0f)) for UI
            drawObject(dialogueBoxMesh, glm::vec3(window.getWidth() / 2.0f, 100.0f, 0.0f), glm::vec3(1200.0f, 200.0f, 1.0f), diagShader, glm::mat4(1.0f), textProjection, 0.0f);


            // --------------------------
            // 2. Draw Character Portrait
            // --------------------------
            if (portraits.find(line.CharacterName) != portraits.end())
            {
                GLuint portraitTex = portraits[line.CharacterName];

                portraitShader.use();

                // Bind the texture to Unit 0
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, portraitTex);
                glUniform1i(glGetUniformLocation(portraitShader.getId(), "image"), 0);

                // Position: Left side of the dialogue box
                // Center X is 960. Box width is 1200. Left edge approx 360.
                // Placed it at x=450, y=100
                if(line.CharacterName=="Warlock")
                    drawObject(dialogueBoxMesh, glm::vec3(360.0f, 100.0f, 0.0f), glm::vec3(150.0f, 150.0f, 1.0f), portraitShader, glm::mat4(1.0f), textProjection, 0.0f);
                else
                    drawObject(dialogueBoxMesh, glm::vec3(1200.0f, 100.0f, 0.0f), glm::vec3(150.0f, 150.0f, 1.0f), portraitShader, glm::mat4(1.0f), textProjection, 0.0f);
            }

            // --------------------------
            // 3. Render Text
            // --------------------------
            if(line.CharacterName!="none")
                // Character Name (Yellow)
                textRenderer.RenderText(textShader, line.CharacterName, 550.0f, 130.0f, 1.0f, glm::vec3(1.0f, 1.0f, 0.0f));

            // Dialogue Line (White)
            textRenderer.RenderText(textShader, line.Text, 550.0f, 85.0f, 0.8f, glm::vec3(1.0f, 1.0f, 1.0f));
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