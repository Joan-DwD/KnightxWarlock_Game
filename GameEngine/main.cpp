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

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// --- Custom Class Includes ---
#include "TextRenderer.h"
#include "Wall.h"
#include "Torch.h"
#include "Collision.h"

// ======================
// GLOBAL QUEST HANDLING VARIABLES
// ======================

int currentTask = 1;
int currentRoom = 1;
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
// CAMERA COLLIDER
// ======================

struct CameraCollider
{
    glm::vec3 position;
    glm::vec3 halfSize;
};

// ======================
// LIGHT
// ======================
glm::vec3 lightColor = glm::vec3(0.8f, 0.6f, 0.4f);
glm::vec3 lightPos = glm::vec3(0.0f, 6.5f, 1.0f);


// =======================
// DRAWING FUNCTION
// =======================

void drawObject(Mesh& mesh, glm::vec3 position, glm::vec3 scale, Shader& shader, glm::mat4 viewMatrix, glm::mat4 projectionMatrix, float rotation, float tiling = 1.0f) {
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

    // Tiling
    glUniform2f(glGetUniformLocation(shader.getId(), "uvScale"), tiling, tiling);

    // Draw the mesh
    mesh.draw(shader);
}

void drawObject(std::vector<Mesh>& meshes, glm::vec3 position, glm::vec3 scale, Shader& shader, glm::mat4 viewMatrix, glm::mat4 projectionMatrix, float rotation, float tiling = 1.0f) {
    // 1. Calculate Matrix *once* for the whole object
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::rotate(model, rotation, glm::vec3(0, 1, 0));
    model = glm::scale(model, scale);

    glm::mat4 mvp = projectionMatrix * viewMatrix * model;

    GLuint matrixID = glGetUniformLocation(shader.getId(), "MVP");
    GLuint modelID = glGetUniformLocation(shader.getId(), "model");

    glUniformMatrix4fv(matrixID, 1, GL_FALSE, &mvp[0][0]);
    glUniformMatrix4fv(modelID, 1, GL_FALSE, &model[0][0]);
    glUniform2f(glGetUniformLocation(shader.getId(), "uvScale"), tiling, tiling);

    // 2. Loop and Draw all sub-meshes
    for (auto& mesh : meshes) {
        mesh.draw(shader);
    }
}

void drawObjectSideways(Mesh& mesh, glm::vec3 position, glm::vec3 scale, Shader& shader, glm::mat4 viewMatrix, glm::mat4 projectionMatrix, float rotation) {
    // Calculate Model Matrix
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::rotate(model, rotation, glm::vec3(0, 0, 1));
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

// ==========================================
// ASSIMP FUNCTIONS
// ==========================================
// Helper to create a 1x1 texture from a color
GLuint CreateTextureFromColor(float r, float g, float b) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    unsigned char data[3];
    data[0] = (unsigned char)(r * 255.0f);
    data[1] = (unsigned char)(g * 255.0f);
    data[2] = (unsigned char)(b * 255.0f);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    return textureID;
}

std::vector<Mesh> loadAssimpMesh(std::string path, GLuint overrideTextureID = 0, int overrideMeshIndex = -1) {
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(path,
        aiProcess_Triangulate |
        aiProcess_FlipUVs |
        aiProcess_GenSmoothNormals
    );

    std::vector<Mesh> meshList;

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
        return meshList;
    }

    // Process every sub-mesh
    for (unsigned int m = 0; m < scene->mNumMeshes; m++)
    {
        aiMesh* mesh = scene->mMeshes[m];
        std::vector<Vertex> vertices;
        std::vector<int> indices;
        std::vector<Texture> textures;

        // 1. Process Vertices
        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            Vertex vertex;

            vertex.pos.x = mesh->mVertices[i].x;
            vertex.pos.y = mesh->mVertices[i].y;
            vertex.pos.z = mesh->mVertices[i].z;

            if (mesh->HasNormals()) {
                vertex.normals.x = mesh->mNormals[i].x;
                vertex.normals.y = mesh->mNormals[i].y;
                vertex.normals.z = mesh->mNormals[i].z;
            }

            if (mesh->mTextureCoords[0]) {
                vertex.textureCoords.x = mesh->mTextureCoords[0][i].x;
                vertex.textureCoords.y = mesh->mTextureCoords[0][i].y;
            }
            else {
                vertex.textureCoords = glm::vec2(0.0f, 0.0f);
            }
            vertices.push_back(vertex);
        }

        // 2. Process Indices
        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back((int)face.mIndices[j]);
        }

        // 3. Process Material (the color extractor)
        bool materialAssigned = false;

        // A. Check for Override
        if (overrideTextureID != 0 && (int)m == overrideMeshIndex)
        {
            Texture t;
            t.id = overrideTextureID;
            t.type = "texture_diffuse";
            textures.push_back(t);
            materialAssigned = true;
        }

        // B. If no override, try to get color from MTL
        if (!materialAssigned && mesh->mMaterialIndex >= 0) {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

            // Get the Diffuse Color (Kd) directly from the MTL
            aiColor3D color(0.f, 0.f, 0.f);
            if (material->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {

                // Create 1x1 texture from color
                GLuint texID = CreateTextureFromColor(color.r, color.g, color.b);

                Texture t;
                t.id = texID;
                t.type = "texture_diffuse";
                textures.push_back(t);
            }
        }
        meshList.push_back(Mesh(vertices, indices, textures));
    }

    return meshList;
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

// =======================
// ROOM TRANSITION FUNCTION
// =======================
float fadeAlpha = 0.0f;
bool isFadingOut = false;
bool isFadingIn = true;

float fadeSpeed = 3.0;
float fadeInSpeed = 1.0;
void TriggerRoomChange() {
    if (!isFadingOut && !isFadingIn) {
        isFadingOut = true;
    }
}

// final door puzzle struct
struct puzzleDoor
{
    bool isUnlocked;
    glm::vec3 position;
    glm::vec3 scale;
    float rotation;
};

struct frogSwitch
{
    bool isPressed;
    glm::vec3 position;
    glm::vec3 scale;

    std::vector<int> links;
};

// bookshelf struct
struct bookshelf
{
    glm::vec3 position;
    glm::vec3 scale;
};

// tree struct
struct TreeStruct
{
    glm::vec3 position;
    glm::vec3 scale;
    float rotation;
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
    Shader room2shader("Shaders/vertex_shader_room2.glsl", "Shaders/fragment_shader_room2.glsl");
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

    GLuint polishedAndesite = loadBMP("Resources/Textures/polished_andesite.bmp");
    GLuint deepslateBricks = loadBMP("Resources/Textures/deepslate_bricks.bmp");
    GLuint ironBlock = loadBMP("Resources/Textures/iron_block.bmp");
    GLuint copper = loadBMP("Resources/Textures/copper.bmp");
    GLuint spruce = loadBMP("Resources/Textures/spruce.bmp");
    GLuint grass = loadBMP("Resources/Textures/grass.bmp");
    GLuint magenta = loadBMP("Resources/Textures/magenta.bmp");
    GLuint oak = loadBMP("Resources/Textures/oak.bmp");
    GLuint concrete = loadBMP("Resources/Textures/concrete.bmp");
    GLuint mangrove = loadBMP("Resources/Textures/mangrove.bmp");
    GLuint glass = loadBMP("Resources/Textures/glass.bmp");

    // Character portraits
    std::map<std::string, GLuint> portraits;
    portraits["Warlock"] = loadBMP("Resources/Textures/WarlockTest.bmp");
    portraits["Knight"] = loadBMP("Resources/Textures/PAINT_GOLD.bmp");
    portraits["Princess"] = loadBMP("Resources/Textures/PAINT_PINK.bmp");
    portraits["Skelly"] = loadBMP("Resources/Textures/skellyPortrait.bmp");
    Shader portraitShader("Shaders/ui_texture_vertex.glsl", "Shaders/ui_texture_fragment.glsl");
    // Delven pack textures
    //GLuint floor_brick = loadBMP("Resources/Textures/WarlockTest.bmp"); // not a correct bmp file..?

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

    //std::vector<Texture> brick_texture = { { floor_brick, "texture_diffuse" } };

    std::vector<Texture> floor_texture = { { polishedAndesite, "texture_diffuse" } };
    std::vector<Texture> wall_texture = { { deepslateBricks, "texture_diffuse" } };
    std::vector<Texture> bars_texture = { { ironBlock, "texture_diffuse" } };
    std::vector<Texture> prison_door_texture = { { copper, "texture_diffuse" } };
    std::vector<Texture> door_texture = { { mangrove, "texture_diffuse" } };
    std::vector<Texture> deco_door_texture = { { spruce, "texture_diffuse" } };
    std::vector<Texture> garden_floor_texture = { { grass, "texture_diffuse" } };
    std::vector<Texture> bedroom_floor_texture = { { oak, "texture_diffuse" } };
    std::vector<Texture> bedroom_carpet_texture = { { magenta, "texture_diffuse" } };
    std::vector<Texture> bedroom_wall_texture = { { concrete, "texture_diffuse" } };
    std::vector<Texture> window_texture = { { glass, "texture_diffuse" } };

    // ======================
    // LOAD UNIVERSAL MODELS
    // ======================
    MeshLoaderObj loader;
    std::vector<Texture> noTextures;

    // walls and floor
    Mesh wallCube = loader.loadObj("Resources/Models/cube.obj", wall_texture);
    Mesh floorCube = loader.loadObj("Resources/Models/cube.obj", floor_texture);
    Mesh prisonWall = loader.loadObj("Resources/Models/barsCube.obj", bars_texture);

    // window
    Mesh windowCube = loader.loadObj("Resources/Models/cube.obj", window_texture);

    // Pawns
    Mesh warlock = loader.loadObj("Resources/Models/warlock.obj", paint_purple_texture);
    Mesh knight = loader.loadObj("Resources/Models/knight.obj", paint_gold_texture);
    Mesh knife = loader.loadObj("Resources/Models/dagger_common.obj", paint_darkgray_texture);

    Mesh princess;

    // Doors
    Mesh doorMesh = loader.loadObj("Resources/Models/standardDoor.obj", door_texture);
    Mesh prisonDoor = loader.loadObj("Resources/Models/prisonDoorCube.obj", prison_door_texture);
    Mesh decoDoor = loader.loadObj("Resources/Models/standardDoor.obj", deco_door_texture);

    // Dialogue Box: We pass an EMPTY texture list because the shader uses solid color only
    Mesh dialogueBoxMesh = loader.loadObj("Resources/Models/cube.obj", noTextures);

    // ======================
    // ROOM 1 - PRISON
    // ======================

    Mesh keyMesh;

    Mesh torch;
    torch = loader.loadObj("Resources/Models/torch.obj", paint_black_texture);

    Mesh mrSkelly = loader.loadObj("Resources/Models/mr_skelly.obj", paint_white_texture);
    Mesh evilSkelly = loader.loadObj("Resources/Models/mr_skelly.obj", paint_red_texture);
    // Back wall
    Wall backWall(&wallCube, glm::vec3(0.0f, 0.1f, 7.0f), glm::vec3(7.0f, 7.0f, 0.1f)); //back is down
    // Front wall
    Wall frontWall(&wallCube, glm::vec3(0.0f, 0.1f, -7.0f), glm::vec3(7.0f, 7.0f, 0.1f)); //front is up
    // Left wall
    Wall leftWall(&wallCube, glm::vec3(-7.0f, 0.1f, 0.0f), glm::vec3(0.1f, 7.0f, 7.0f));
    // Right wall
    Wall rightWall(&wallCube, glm::vec3(7.0f, 0.1f, 0.0f), glm::vec3(0.1f, 7.0f, 7.0f));
    // Middle wall
    Wall middleWall(&prisonWall, glm::vec3(-2.0f, 2.0f, 0.0f), glm::vec3(5.0f, 2.0f, 0.1f));

    // ======================
    // ROOM 2 - HALLWAY
    // ======================

    std::vector<Mesh> barrel;
    Mesh book;

    // ====================
    // ROOM 3 - LIBRARY
    // ====================

    // books
    std::vector<Mesh> bookcaseParts;

    bookshelf bookshelves[8] =
    {
        {glm::vec3(-4.0f, 0.0f, -5.8f), glm::vec3(2.5f) },
        {glm::vec3(4.0f, 0.0f, -4.5f), glm::vec3(2.5f) },
        {glm::vec3(4.0f, 0.0f, -1.5f), glm::vec3(2.5f) },
        {glm::vec3(4.0f, 0.0f, 1.5f), glm::vec3(2.5f) },
        {glm::vec3(4.0f, 0.0f, 4.5f), glm::vec3(2.5f) },
        {glm::vec3(-4.0f, 0.0f, -2.5f), glm::vec3(2.5f) },
        {glm::vec3(-4.0f, 0.0f, 0.5f), glm::vec3(2.5f) },
        {glm::vec3(-4.0f, 0.0f, 3.5f), glm::vec3(2.5f) }
    };


    const int BOOKSHELF_COUNT = sizeof(bookshelves) / sizeof(bookshelves[0]);

    // ====================
    // ROOM 4 - GARDEN
    // ====================

    Mesh gardenFloorCube;
    std::vector<Mesh> tree_a, tree_b, tree_c, tree_d, tree_e, tree_f;
    Mesh frog;
    std::vector<Mesh> bush_a, bush_b, plant_a, plant_b, rock_a, rock_b;
    Mesh pond_water, pond_frame;

    // Back wall
    Wall backWall_4(&wallCube, glm::vec3(0.0f, -9.9f, 15.0f), glm::vec3(15.0f, 15.0f, 0.1f));
    // Front wall
    Wall frontWall_4(&wallCube, glm::vec3(0.0f, -9.9f, -15.0f), glm::vec3(15.0f, 15.0f, 0.1f));
    // Left wall
    Wall leftWall_4(&wallCube, glm::vec3(-15.0f, -9.9f, 0.0f), glm::vec3(0.1f, 15.0f, 15.0f));
    // Right wall
    Wall rightWall_4(&wallCube, glm::vec3(15.0f, -9.9f, 0.0f), glm::vec3(0.1f, 15.0f, 15.0f));


    TreeStruct trees[25] =
    {
        { glm::vec3(-13.5f, 0.0f,  0.0f), glm::vec3(3.2f),  45.0f },
        { glm::vec3(3.0f, 0.0f, -9.5f), glm::vec3(2.7f), 102.0f },
        { glm::vec3(10.5f, 0.0f,  5.5f), glm::vec3(3.1f),  75.0f },
        { glm::vec3(-8.0f, 0.0f, -7.0f), glm::vec3(2.9f), 160.0f },
        { glm::vec3(6.5f, 0.0f, 10.0f), glm::vec3(2.8f),  33.0f },

        { glm::vec3(-11.0f,0.0f,  4.5f), glm::vec3(3.3f),  88.0f },
        { glm::vec3(12.0f,0.0f, -6.0f), glm::vec3(2.6f),  12.0f },
        { glm::vec3(-9.5f, 0.0f, -10.5f), glm::vec3(3.0f), 141.0f },
        { glm::vec3(8.0f, 0.0f, 12.0f), glm::vec3(3.1f),  64.0f },
        { glm::vec3(11.2f,0.0f, -2.0f), glm::vec3(2.5f), 170.0f },

        { glm::vec3(-7.0f, 0.0f, 12.5f), glm::vec3(2.8f),  20.0f },
        { glm::vec3(7.5f,0.0f, -8.5f), glm::vec3(3.0f), 112.0f },
        { glm::vec3(-10.0f,0.0f,  6.0f), glm::vec3(3.3f),  96.0f },
        { glm::vec3(6.0f,0.0f, -11.0f), glm::vec3(2.6f),  51.0f },
        { glm::vec3(13.0f,0.0f,  7.0f), glm::vec3(3.0f), 178.0f },

        { glm::vec3(-12.0f,0.0f,  8.5f), glm::vec3(3.5f), 137.0f },
        { glm::vec3(9.0f,0.0f, -5.0f), glm::vec3(2.8f),  18.0f },
        { glm::vec3(10.0f,0.0f, -12.0f), glm::vec3(2.5f), 163.0f },
        { glm::vec3(-6.5f, 0.0f, 10.0f), glm::vec3(3.3f),  72.0f },
        { glm::vec3(12.5f,0.0f, -7.5f), glm::vec3(3.0f), 102.0f },

        { glm::vec3(-6.0f, 0.0f, 12.0f), glm::vec3(2.7f), 150.0f },
        { glm::vec3(-12.5f,0.0f, -5.0f), glm::vec3(3.2f),  45.0f },
        { glm::vec3(11.5f,0.0f, -10.0f), glm::vec3(2.9f), 125.0f },
        { glm::vec3(-10.5f,0.0f, -12.0f), glm::vec3(3.1f),  82.0f },
        { glm::vec3(9.0f,0.0f,  3.0f), glm::vec3(2.6f), 155.0f }
    };


    // ====================
    // ROOM 5 - WARDROBE
    // ====================

    Wall wall_5_a(&prisonWall, glm::vec3(2.0f, 2.0f, -3.0f), glm::vec3(5.0f, 2.0f, 0.1f));
    Wall wall_5_b(&prisonWall, glm::vec3(-2.0f, 2.0f, 1.0f), glm::vec3(5.0f, 2.0f, 0.1f));
    Wall wall_5_c(&prisonWall, glm::vec3(2.0f, 2.0f, 5.0f), glm::vec3(5.0f, 2.0f, 0.1f));

    Wall leftWall_5(&wallCube, glm::vec3(-7.0f, -4.0f, 0.0f), glm::vec3(0.1f, 12.0f, 12.0f));
    Wall rightWall_5(&wallCube, glm::vec3(7.0f, -4.0f, 0.0f), glm::vec3(0.1f, 12.0f, 12.0f));
    Wall backWall_5(&wallCube, glm::vec3(0.0f, -4.0f, 12.0f), glm::vec3(12.0f, 12.0f, 0.1f));
    Wall frontWall_5(&wallCube, glm::vec3(0.0f, -4.0f, -7.0f), glm::vec3(12.0f, 12.0f, 0.1f));

    Mesh frogButton;

    puzzleDoor doors[8] = 
    {
        { false, glm::vec3(-5.0f, 2.0f, 5.0f), glm::vec3(2.0f, 2.0f, 0.1f), 0.0f },
        { false, glm::vec3(-3.0f, 2.0f, 3.0f), glm::vec3(2.0f, 2.0f, 0.1f), 90.0f },
        { false, glm::vec3(3.0f, 2.0f, 3.0f), glm::vec3(2.0f, 2.0f, 0.1f), 90.0f},
        { false, glm::vec3(5.0f, 2.0f, 1.0f), glm::vec3(2.0f, 2.0f, 0.1f), 0.0f },
        { false, glm::vec3(3.0f, 2.0f, -1.0f), glm::vec3(2.0f, 2.0f, 0.1f), 90.0f },
        { false, glm::vec3(-3.0f, 2.0f, -1.0f), glm::vec3(2.0f, 2.0f, 0.1f), 90.0f },
        { false, glm::vec3(-5.0f, 2.0f, -3.0f), glm::vec3(2.0f, 2.0f, 0.1f), 0.0f },
        { false, glm::vec3(-3.0f, 2.0f, -5.0f), glm::vec3(2.0f, 2.0f, 0.1f), 90.0f }
    };

    // FROG PUZZLE

    frogSwitch buttons[5] =
    {
        { false, glm::vec3(6.0f, 2.0f, -6.8f), glm::vec3(0.1f), {0, 1, 2, 3, 4, 5, 6, 7}},
        { false, glm::vec3(-5.5f, 2.0f, 11.8f), glm::vec3(0.1f), {0, 6} },
        { false, glm::vec3(-3.0f, 2.0f, 11.8f), glm::vec3(0.1f), {1, 5, 7} },
        { false, glm::vec3(3.0f, 2.0f, 11.8f), glm::vec3(0.1f), {2, 4} },
        { false, glm::vec3(5.5f, 2.0f, 11.8f), glm::vec3(0.1f), {3} }
    };

    const int DOOR_COUNT = sizeof(doors) / sizeof(doors[0]);
    const int BUTTON_COUNT = sizeof(buttons) / sizeof(buttons[0]);

    //=======================
    // ROOM 6 - BEDROOM
    // ======================
    
    Mesh bedroomWallCube = loader.loadObj("Resources/Models/cube.obj", bedroom_wall_texture);

    Mesh bedroomFloorCube;
    Mesh bedroomFloorCarpet;

    Mesh bedroomBed;
    Mesh bedroomPiano;
    Mesh bedroomDresser;
    Mesh bedroomTeddy;

    // Back wall
    Wall backWall_6(&bedroomWallCube, glm::vec3(0.0f, -0.9f, 9.0f), glm::vec3(9.0f, 9.0f, 0.1f));
    // Front wall
    Wall frontWall_6(&bedroomWallCube, glm::vec3(0.0f, -0.9f, -9.0f), glm::vec3(9.0f, 9.0f, 0.1f));
    // Left wall
    Wall leftWall_6(&bedroomWallCube, glm::vec3(-9.0f, -0.9f, 0.0f), glm::vec3(0.1f, 9.0f, 9.0f));
    // Right wall
    Wall rightWall_6(&bedroomWallCube, glm::vec3(9.0f, -0.9f, 0.0f), glm::vec3(0.1f, 9.0f, 9.0f));

    // ======================
    // TORCH
    // ======================
    Torch* wallTorch = nullptr;

    Mesh flameCube = loader.loadObj("Resources/Models/cube.obj", paint_orange_texture);
    Mesh stickCube = loader.loadObj("Resources/Models/cube.obj", paint_darkbrown_texture);

    // room 1 torch
    wallTorch = new Torch(&torch, &flameCube, glm::vec3(-6.8, 3.0f, 2.0f), 180.0f);

    // room 2 torches
    Torch* hallTorches[] = 
    {
     new Torch(&torch, &flameCube, glm::vec3(-6.0f, 3.0f, -6.8f), 180.0f),
     new Torch(&torch, &flameCube, glm::vec3(-4.0f, 3.0f, -6.8f), 180.0f),
     new Torch(&torch, &flameCube, glm::vec3(-2.0f, 3.0f, -6.8f), 180.0f),
     new Torch(&torch, &flameCube, glm::vec3(0.0f, 3.0f, -6.8f), 180.0f),
     new Torch(&torch, &flameCube, glm::vec3(2.0f, 3.0f, -6.8f), 180.0f),
     new Torch(&torch, &flameCube, glm::vec3(4.0f, 3.0f, -6.8f), 180.0f),
     new Torch(&torch, &flameCube, glm::vec3(6.0f, 3.0f, -6.8f), 180.0f)
    };

    const int HALL_TORCH_COUNT = sizeof(hallTorches) / sizeof(hallTorches[0]);

    // room 3 torch
    Torch* hallTorches3[] =
    {
     new Torch(&torch, &flameCube, glm::vec3(0.0f, 2.0f, -6.8f), 180.0f),
     new Torch(&torch, &flameCube, glm::vec3(0.0f, 2.0f, 6.8f), 180.0f)
    };

    const int HALL_TORCH_COUNT3 = sizeof(hallTorches3) / sizeof(hallTorches3[0]);

    // room 5 torch
    Torch* hallTorches5[] =
    {
     new Torch(&torch, &flameCube, glm::vec3(-6.8f, 3.0f, 8.0f), 180.0f),
     new Torch(&torch, &flameCube, glm::vec3(6.8f, 3.0f, 8.0f), 180.0f)
    };

    const int HALL_TORCH_COUNT5 = sizeof(hallTorches5) / sizeof(hallTorches5[0]);

    // ======================
    // TEXT RENDERER SETUP
    // ======================
    // Initialize the TextRenderer class
    TextRenderer textRenderer("C:/Windows/Fonts/georgia.ttf", 48);

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
    glm::vec3 warlockPos;
    glm::vec3 knightPos;
    float warlockYaw = 0.0f;
    float knightYaw = 0.0f;
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
    bool isSolved_wardrobe = false; // room 5 puzzle

    // frog stuff
    float frogRadius = 5.0f;
    float frogHeight = 3.0f;
    float frogWalkSpeed = 2.0f;
    float frogJumpSpeed = 7.0f;
    float frogTime = 0.0f;


    glm::vec3 exitPos;

    // Load dialogue
    LoadDialogue(0);

    Camera camera(warlockPos);

    CameraCollider cameraCube;
    cameraCube.halfSize = glm::vec3(0.3f, 1.0f, 0.3f);
    cameraCube.position = camera.getCameraPosition();

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

        // keyboard input
        processKeyboardInput();

        // ======================
        // MATRICES
        // ======================
        glm::mat4 ProjectionMatrix = glm::perspective(90.0f, window.getWidth() * 1.0f / window.getHeight(), 0.1f, 10000.0f);
        glm::mat4 ViewMatrix = glm::lookAt(
            camera.getCameraPosition(),
            camera.getCameraPosition() + camera.getCameraViewDirection(),
            camera.getCameraUp()
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

        if(!activeIsWarlock)
            drawObject(warlock, warlockPos - glm::vec3(0.0f, 1.9f, 0.0f), glm::vec3(1.7f), shader, ViewMatrix, ProjectionMatrix, warlockYaw);

        // Knight
        if (currentRoom != 6)
        {
            if(activeIsWarlock)
                drawObject(knight, knightPos - glm::vec3(0.0f, 1.9f, 0.0f), glm::vec3(1.7f), shader, ViewMatrix, ProjectionMatrix, knightYaw);
        }

        // active character position
        glm::vec3 activePos = activeIsWarlock ? warlockPos : knightPos;

        if (currentRoom == 1)
        {
            shader.use();
            if (firstLoad == 1)
            {
                // ======================
                // ROOM 1 - PRISON
                // ======================

                keyMesh = loader.loadObj("Resources/Models/key.obj", paint_lavender_texture);

                warlockPos = glm::vec3(0.0f, 2.0f, 6.0f);
                knightPos = glm::vec3(0.0f, 2.0f, -6.0f);
                warlockYaw = 180.0f;
                knightYaw = 0.0f;

                if (activeIsWarlock)
                    cameraCube.position = warlockPos;
                else
                    cameraCube.position = knightPos;

                camera.setCameraPosition(cameraCube.position);
                camera.setYaw(warlockYaw);

                firstLoad = 0;

            }

            exitPos = glm::vec3(0.0f, 2.0f, -6.8f);

            // ======================
            // DRAW WALLS
            // ======================
            backWall.draw(shader, ViewMatrix, ProjectionMatrix, 32.0f);
            frontWall.draw(shader, ViewMatrix, ProjectionMatrix, 32.0f);
            leftWall.draw(shader, ViewMatrix, ProjectionMatrix, 32.0f);
            rightWall.draw(shader, ViewMatrix, ProjectionMatrix, 32.0f);

            middleWall.draw(shader, ViewMatrix, ProjectionMatrix, 4.0f);

            // ======================
            // DRAW FLOOR
            // ======================

            drawObject(floorCube, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(7.0f, 0.1f, 7.0f), shader, ViewMatrix, ProjectionMatrix, 0.0f, 32.0f);
            drawObject(floorCube, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(7.0f, 7.1f, 7.0f), shader, ViewMatrix, ProjectionMatrix, 0.0f, 32.0f);

            // window
            drawObject(windowCube, glm::vec3(3.5f, 3.0f, 6.8f), glm::vec3(1.8f, 1.8f, 0.1f), shader, ViewMatrix, ProjectionMatrix, 0.0f, 8.0f);

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

            // THE NEW CHARACTER - MR SKELLY BONES
            glm::vec3 skellyPos = glm::vec3(-4.5f, 0.0f, 6.5f);
            drawObject(mrSkelly, skellyPos, glm::vec3(2.0f), shader, ViewMatrix, ProjectionMatrix, 180.0f);
            colliders.push_back(makeAABB(skellyPos, glm::vec3(0.5f, 3.0f, 0.5f)));

            drawObject(evilSkelly, -skellyPos, glm::vec3(2.0f), shader, ViewMatrix, ProjectionMatrix, 0.0f);

            // ======================
            // KEY PICKUP
            // ======================
            
            float distToKey = glm::length(activePos - keyPos);
            if (distToKey < 2.0f && !keyCollected)
            {
                if (window.isPressed(GLFW_KEY_E))
                {
                    if (currentTask == 2)
                    {
                        LoadDialogue(2);
                        currentTask = 3;
                    }
                    keyCollected = true;
                    hasKey = true;
                }
            }

            // ======================
            // DOOR INTERACTION
            // ======================
            float distToDoor = glm::length(activePos - doorPos);
            if (distToDoor < 2.0f && !activeIsWarlock)
            {
                if (window.isPressed(GLFW_KEY_E))
                {
                    if (hasKey && !doorUnlocked)
                    {
                        if (currentTask == 3)
                        {
                            LoadDialogue(3);
                            currentTask = 4;
                        }
                        doorUnlocked = true;
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
                drawObject(prisonDoor, doorPos, glm::vec3(2.0f, 2.0f, 0.1f), shader, ViewMatrix, ProjectionMatrix, 180.0f, 4.0f);
            }

            // ======================
            // DRAW EXIT
            // ======================

            drawObject(doorMesh, exitPos, glm::vec3(1.5f, 2.0f, 0.1f), shader, ViewMatrix, ProjectionMatrix, 0.0f, 4.0f);

            // ======================
            // EXIT INTERACTION
            // ======================
            float distToExit = glm::length(warlockPos - exitPos);
            if (distToExit < 1.0f)
            {

                firstLoad = 1;
                currentRoom = 2;
                // TriggerRoomChange(); timing/function to be adjusted to suit room 1 as well
            }

            // ======================
            // TORCH
            // ======================
            //  offset to be "inside" the flame
            glm::vec3 flameLightPos = wallTorch->position + glm::vec3(0.0f, 0.4f, 0.0f);
            glUniform3f(glGetUniformLocation(shader.getId(), "torchPos"), flameLightPos.x, flameLightPos.y, flameLightPos.z);
            glUniform3f(glGetUniformLocation(shader.getId(), "torchColor"), 1.0f, 0.5f, 0.0f);
            // torch state
            glUniform1i(glGetUniformLocation(shader.getId(), "torchOn"), wallTorch->isOn);

            wallTorch->draw(shader, ViewMatrix, ProjectionMatrix, currentFrame);
        }
        else 
        if (currentRoom == 2)
        {
			// turn room1 torch off (could find smarter workaround prob)
            glUniform1i(glGetUniformLocation(shader.getId(), "torchOn"), 0);
            room2shader.use();
            glUniform3f(glGetUniformLocation(room2shader.getId(), "viewPos"), camera.getCameraPosition().x, camera.getCameraPosition().y, camera.getCameraPosition().z);
            glUniform3f(glGetUniformLocation(room2shader.getId(), "lightColor"), lightColor.x, lightColor.y, lightColor.z);
            glUniform3f(glGetUniformLocation(room2shader.getId(), "lightPos"), lightPos.x, lightPos.y, lightPos.z);
            glUniform1i(glGetUniformLocation(room2shader.getId(), "activeTorchCount"), HALL_TORCH_COUNT);

            if (firstLoad == 1)
            {
                barrel = loadAssimpMesh("Resources/Models/barrel.obj", spruce, 0);
                book = loader.loadObj("Resources/Models/openBook.obj", paint_white_texture);

                warlockPos = glm::vec3(1.2f, 2.0f, 6.0f);
                knightPos = glm::vec3(-1.2f, 2.0f, 6.0f);
                warlockYaw = 180.0f;
                knightYaw = 180.0f;

                if (activeIsWarlock)
                    cameraCube.position = warlockPos;
                else
                    cameraCube.position = knightPos;

                camera.setCameraPosition(cameraCube.position);
                camera.setYaw(warlockYaw);

                if (currentTask == 4)
                {
                    currentTask = 5;
                    LoadDialogue(4);
                }

                firstLoad = 0;
            }
            exitPos = glm::vec3(6.8f, 2.0f, 0.0f);

            // room 1 colliders
            colliders.push_back(backWall.getAABB());
            colliders.push_back(frontWall.getAABB());
            colliders.push_back(leftWall.getAABB());
            colliders.push_back(rightWall.getAABB());

            // ======================
            // DRAW WALLS
            // ======================
            backWall.draw(room2shader, ViewMatrix, ProjectionMatrix, 32.0f);
            frontWall.draw(room2shader, ViewMatrix, ProjectionMatrix, 32.0f);
            leftWall.draw(room2shader, ViewMatrix, ProjectionMatrix, 32.0f);
            rightWall.draw(room2shader, ViewMatrix, ProjectionMatrix, 32.0f);

            // ======================
            // DRAW FLOOR
            // ======================

            drawObject(floorCube, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(7.0f, 0.1f, 7.0f), room2shader, ViewMatrix, ProjectionMatrix, 0.0f, 32.0f);
            drawObject(floorCube, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(7.0f, 7.1f, 7.0f), room2shader, ViewMatrix, ProjectionMatrix, 0.0f, 32.0f);

            //=======================
            // DRAW BOOK AND BARREL nad knife
            //=======================

            glm::vec3 barrelPos = glm::vec3(-6.0f, 0.1f, 0.0f);
            drawObject(barrel, barrelPos, glm::vec3(1.6f), room2shader, ViewMatrix, ProjectionMatrix, 0.0f, 4.0f);
            colliders.push_back(makeAABB(barrelPos, glm::vec3(0.8f, 3.0f, 0.8f)));

            drawObject(book, barrelPos, glm::vec3(1.6f), room2shader, ViewMatrix, ProjectionMatrix, 90.0f);

            static bool eKeyWasPressed = false;

            float distToBook = glm::length(activePos - barrelPos);
            if (distToBook < 3.0f)
            {
                if (window.isPressed(GLFW_KEY_E))
                {
                    // debounce
                    if (!eKeyWasPressed) 
                    {
                        LoadDialogue(5);
                        if (currentTask == 5)
                        {
                            currentTask = 6;
                        }
                        eKeyWasPressed = true;
                    }
                }
                else {
                    eKeyWasPressed = false;
                }
            }
            
            // ======================
            // DRAW TORCHES
            // ======================

            // light iters
            for (int i = 0; i < HALL_TORCH_COUNT; i++)
            {
                std::string posName = "torchPos[" + std::to_string(i) + "]";
                std::string colorName = "torchColor[" + std::to_string(i) + "]";
                std::string onName = "torchOn[" + std::to_string(i) + "]";

                glm::vec3 p = hallTorches[i]->position + glm::vec3(0.0f, 0.4f, 0.0f);

                glUniform3f(glGetUniformLocation(room2shader.getId(), posName.c_str()), p.x, p.y, p.z);
                glUniform3f(glGetUniformLocation(room2shader.getId(), colorName.c_str()), 1.0f, 0.5f, 0.2f);
                glUniform1i(glGetUniformLocation(room2shader.getId(), onName.c_str()), hallTorches[i]->isOn ? 1 : 0);
            }
            
            for (int i = 0; i < HALL_TORCH_COUNT; i++) 
            {
                hallTorches[i]->draw(room2shader, ViewMatrix, ProjectionMatrix, currentFrame);

                static bool eKeyWasPressed = false;

                if (hallTorches[i]->isPlayerClose(activeIsWarlock ? warlockPos : knightPos, 1.5f) && currentTask == 6)
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

            if (hallTorches[0]->isOn && !hallTorches[1]->isOn && !hallTorches[2]->isOn
                && !hallTorches[3]->isOn && !hallTorches[4]->isOn && hallTorches[5]->isOn
                && hallTorches[6]->isOn )
            {
                isSolved_torch = true;
            }

            // =====================
            // DRAW EXTRA DOORS
            // =====================

            drawObject(decoDoor, glm::vec3(6.8f, 2.0f, -3.5f), glm::vec3(1.5f, 2.0f, 0.1f), room2shader, ViewMatrix, ProjectionMatrix, -90.0f, 4.0f);
            drawObject(decoDoor, glm::vec3(6.8f, 2.0f, 3.5f), glm::vec3(1.5f, 2.0f, 0.1f), room2shader, ViewMatrix, ProjectionMatrix, -90.0f, 4.0f);
            drawObject(decoDoor, glm::vec3(-6.8f, 2.0f, -3.5f), glm::vec3(1.5f, 2.0f, 0.1f), room2shader, ViewMatrix, ProjectionMatrix, 90.0f, 4.0f);
            drawObject(decoDoor, glm::vec3(-6.8f, 2.0f, 3.5f), glm::vec3(1.5f, 2.0f, 0.1f), room2shader, ViewMatrix, ProjectionMatrix, 90.0f, 4.0f);

            drawObject(decoDoor, glm::vec3(0.0f, 2.0f, 6.8f), glm::vec3(1.5f, 2.0f, 0.1f), room2shader, ViewMatrix, ProjectionMatrix, 180.0f, 4.0f);

            if (isSolved_torch == true)
            {

                if (currentTask == 6)
                {
                    LoadDialogue(6);
                    currentTask = 7;
                }
                // ======================
                // DRAW EXIT
                // ======================

                drawObject(doorMesh, exitPos, glm::vec3(1.5f, 2.0f, 0.1f), room2shader, ViewMatrix, ProjectionMatrix, -90.0f, 4.0f);

                // ======================
                // EXIT INTERACTION
                // ======================
                float distToExit = glm::length(warlockPos - exitPos);
                if (distToExit < 1.0f)
                {

                    firstLoad = 1;
                    currentRoom = 3;
                    TriggerRoomChange();
                }
            } 
        }
        else
        if (currentRoom == 3)
        {
            room2shader.use();
            glUniform3f(glGetUniformLocation(room2shader.getId(), "viewPos"), camera.getCameraPosition().x, camera.getCameraPosition().y, camera.getCameraPosition().z);
            glUniform3f(glGetUniformLocation(room2shader.getId(), "lightColor"), lightColor.x, lightColor.y, lightColor.z);
            glUniform3f(glGetUniformLocation(room2shader.getId(), "lightPos"), lightPos.x, lightPos.y, lightPos.z);
            glUniform1i(glGetUniformLocation(room2shader.getId(), "activeTorchCount"), HALL_TORCH_COUNT3);
            if (firstLoad == 1)
            {
                bookcaseParts = loadAssimpMesh("Resources/Models/bookcaseWideFilled.obj", spruce, 0);

                warlockPos = glm::vec3(-2.7f, 2.0f, 6.0f);
                knightPos = glm::vec3(-4.7f, 2.0f, 6.0f);
                warlockYaw = 180.0f;
                knightYaw = 180.0f;

                if (activeIsWarlock)
                    cameraCube.position = warlockPos;
                else
                    cameraCube.position = knightPos;

                camera.setCameraPosition(cameraCube.position);
                camera.setYaw(warlockYaw);

                if (currentTask == 7)
                {
                    currentTask = 8;
                    LoadDialogue(7);
                }

                firstLoad = 0;
            }
            exitPos = glm::vec3(-3.5f, 2.0f, -6.8f);

            // ======================
            // DRAW WALLS
            // ======================
            backWall.draw(room2shader, ViewMatrix, ProjectionMatrix, 32.0f);
            frontWall.draw(room2shader, ViewMatrix, ProjectionMatrix, 32.0f);
            leftWall.draw(room2shader, ViewMatrix, ProjectionMatrix, 32.0f);
            rightWall.draw(room2shader, ViewMatrix, ProjectionMatrix, 32.0f);


            colliders.push_back(backWall.getAABB());
            colliders.push_back(frontWall.getAABB());
            colliders.push_back(leftWall.getAABB());
            colliders.push_back(rightWall.getAABB());

            // ======================
            // DRAW FLOOR
            // ======================

            drawObject(floorCube, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(7.0f, 0.1f, 7.0f), room2shader, ViewMatrix, ProjectionMatrix, 0.0f, 32.0f);
            drawObject(floorCube, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(7.0f, 7.1f, 7.0f), room2shader, ViewMatrix, ProjectionMatrix, 0.0f, 32.0f);

            drawObject(decoDoor, glm::vec3(-3.5f, 2.0f, 6.8f), glm::vec3(1.5f, 2.0f, 0.1f), room2shader, ViewMatrix, ProjectionMatrix, 180.0f, 4.0f);

            // =====================
            // DRAW BOOKSHELVES
            // =====================
            if (!isSolved_books)
            {
                // Draw the first shelf
                for (auto& part : bookcaseParts) {
                    drawObject(part, bookshelves[0].position, bookshelves[0].scale, room2shader, ViewMatrix, ProjectionMatrix, 0.0f);
                }
                colliders.push_back(makeAABB(bookshelves[0].position, glm::vec3(1.9f, 3.0f, 0.4f)));

            }

            // Draw the rest
            for (int i = 1; i < BOOKSHELF_COUNT; i++)
            {
                for (auto& part : bookcaseParts) {
                    drawObject(part, bookshelves[i].position, bookshelves[i].scale, room2shader, ViewMatrix, ProjectionMatrix, 0.0f);
                }
                colliders.push_back(makeAABB(bookshelves[i].position, glm::vec3(1.9f, 3.0f, 0.4f)));
            }

            // reading books:
            for (int i = 0; i < BOOKSHELF_COUNT; i++)
            {
                static bool eKeyWasPressed = false;

                float distToBookhelf = glm::length(activePos - bookshelves[i].position);
                if (distToBookhelf < 3.0f)
                {
                    if (window.isPressed(GLFW_KEY_E))
                    {
                        // debounce
                        if (!eKeyWasPressed)
                        {
                            if(!(isSolved_books && i == 0))
                                LoadDialogue(300 + i);
                            eKeyWasPressed = true;
                        }
                    }
                    else {
                        eKeyWasPressed = false;
                    }
                }
            }

            glm::vec3 endPos = glm::vec3(7.0f, 2.0f, -7.0f);

            float distToEnd = glm::length(warlockPos - endPos);

            if (distToEnd < 1.0f)
            {
                isSolved_books = true;
                if (currentTask == 8)
                {
                    LoadDialogue(8);
                    currentTask = 9;
                }
            }
                

            // ======================
            // TORCH
            // ======================
            // light iters
            for (int i = 0; i < HALL_TORCH_COUNT3; i++)
            {
                std::string posName = "torchPos[" + std::to_string(i) + "]";
                std::string colorName = "torchColor[" + std::to_string(i) + "]";
                std::string onName = "torchOn[" + std::to_string(i) + "]";

                glm::vec3 p = hallTorches3[i]->position + glm::vec3(0.0f, 0.4f, 0.0f);

                glUniform3f(glGetUniformLocation(room2shader.getId(), posName.c_str()), p.x, p.y, p.z);
                glUniform3f(glGetUniformLocation(room2shader.getId(), colorName.c_str()), 1.0f, 0.5f, 0.2f);
                glUniform1i(glGetUniformLocation(room2shader.getId(), onName.c_str()), hallTorches3[i]->isOn ? 1 : 0);
            }

            for (int i = 0; i < HALL_TORCH_COUNT3; i++)
            {
                hallTorches3[i]->draw(room2shader, ViewMatrix, ProjectionMatrix, currentFrame);
            }

            if (true)
            {
                // ======================
                // DRAW EXIT
                // ======================

                drawObject(doorMesh, exitPos, glm::vec3(1.5f, 2.0f, 0.1f), room2shader, ViewMatrix, ProjectionMatrix, 0.0f, 4.0f);

                // ======================
                // EXIT INTERACTION
                // ======================
                float distToExit = glm::length(warlockPos - exitPos);
                if (distToExit < 1.0f)
                {

                    firstLoad = 1;
                    currentRoom = 4;
                    TriggerRoomChange();
                }
            }
        }
        else
        if (currentRoom == 4)
        {
            shader.use();
            if (firstLoad == 1)
            {
                gardenFloorCube = loader.loadObj("Resources/Models/cube.obj", garden_floor_texture);

                tree_a = loadAssimpMesh("Resources/Models/Nature Pack/PineTree_1.obj");
                tree_b = loadAssimpMesh("Resources/Models/Nature Pack/PineTree_2.obj");
                tree_c = loadAssimpMesh("Resources/Models/Nature Pack/CommonTree_5.obj");
                tree_d = loadAssimpMesh("Resources/Models/Nature Pack/CommonTree_2.obj");
                tree_e = loadAssimpMesh("Resources/Models/Nature Pack/Willow_5.obj");
                tree_f = loadAssimpMesh("Resources/Models/Nature Pack/Willow_4.obj");

                frog = loader.loadObj("Resources/Models/frog.obj", paint_orange_texture);

                bush_a = loadAssimpMesh("Resources/Models/Nature Pack/Bush_1.obj");
                bush_b = loadAssimpMesh("Resources/Models/Nature Pack/BushBerries_2.obj");
                plant_a = loadAssimpMesh("Resources/Models/Nature Pack/Plant_4.obj");
                plant_b = loadAssimpMesh("Resources/Models/Nature Pack/Plant_5.obj");
                rock_a = loadAssimpMesh("Resources/Models/Nature Pack/Rock_6.obj");
                rock_b = loadAssimpMesh("Resources/Models/Nature Pack/Rock_Moss_7.obj");

                pond_frame = loader.loadObj("Resources/Models/Pond Pack/pond_frame.obj", paint_lightgray_texture);
                pond_water = loader.loadObj("Resources/Models/Pond Pack/water.obj", paint_blue_texture);

                warlockPos = glm::vec3(1.2f, 2.0f, 14.0f);
                knightPos = glm::vec3(-1.2f, 2.0f, 14.0f);
                warlockYaw = 180.0f;
                knightYaw = 180.0f;

                if (activeIsWarlock)
                    cameraCube.position = warlockPos;
                else
                    cameraCube.position = knightPos;

                camera.setCameraPosition(cameraCube.position);
                camera.setYaw(warlockYaw);

                if (currentTask == 9)
                {
                    currentTask = 10;
                    LoadDialogue(9);
                }

                firstLoad = 0;
            }
            exitPos = glm::vec3(0.0f, 2.0f, -14.8f);

            // ======================
            // DRAW FLOOR
            // ======================

            drawObject(gardenFloorCube, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(15.0f, 0.1f, 15.0f), shader, ViewMatrix, ProjectionMatrix, 0.0f, 32.0f);

            drawObject(decoDoor, glm::vec3(0.0f, 2.0f, 14.8f), glm::vec3(1.5f, 2.0f, 0.1f), room2shader, ViewMatrix, ProjectionMatrix, 180.0f, 4.0f);

            // ======================
            // DRAW WALLS
            // ======================
            backWall_4.draw(shader, ViewMatrix, ProjectionMatrix, 32.0f);
            frontWall_4.draw(shader, ViewMatrix, ProjectionMatrix, 32.0f);
            leftWall_4.draw(shader, ViewMatrix, ProjectionMatrix, 32.0f);
            rightWall_4.draw(shader, ViewMatrix, ProjectionMatrix, 32.0f);

            colliders.push_back(backWall_4.getAABB());
            colliders.push_back(frontWall_4.getAABB());
            colliders.push_back(leftWall_4.getAABB());
            colliders.push_back(rightWall_4.getAABB());

            // ======================
            // DRAW POND
            // ======================

            drawObject(pond_frame, glm::vec3(0.0f, 0.1f, 0.0f), glm::vec3(0.6f), shader, ViewMatrix, ProjectionMatrix, 0.0f);

            // THIS NEEDS TO BE WAVY JWAN
            drawObject(pond_water, glm::vec3(0.0f, 0.22f, 0.0f), glm::vec3(0.5f), shader, ViewMatrix, ProjectionMatrix, 0.0f);

            colliders.push_back(makeAABB(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(3.5f, 3.0f, 3.5f)));

            // ======================
            // EFFED UP GARDEN GENERATION (TRUST ME BRO)
            // ======================
            for (int i = 0; i < 25; i++)
            {
                glm::vec3 treePos = trees[i].position;
                glm::vec3 treeScale = glm::vec3(trees[i].scale);
                float treeRotation = trees[i].rotation;

                switch (i % 6)
                {
                case 0:
                {
                    drawObject(bush_a, -treePos + glm::vec3(1.0f, 0.0f, 1.0f), treeScale - glm::vec3(1.5f), shader, ViewMatrix, ProjectionMatrix, treeRotation);
                    drawObject(tree_a, treePos, treeScale, shader, ViewMatrix, ProjectionMatrix, treeRotation); break;

                }
                case 1:
                {
                    drawObject(bush_b, -treePos - glm::vec3(1.0f, 0.0f, 1.0f), treeScale - glm::vec3(1.5f), shader, ViewMatrix, ProjectionMatrix, treeRotation);
                    drawObject(tree_b, treePos, treeScale, shader, ViewMatrix, ProjectionMatrix, treeRotation); break;
                }
                case 2:
                {
                    drawObject(plant_a, -treePos + glm::vec3(1.0f, 0.0f, 1.0f), treeScale - glm::vec3(1.5f), shader, ViewMatrix, ProjectionMatrix, treeRotation);
                    drawObject(tree_c, treePos, treeScale, shader, ViewMatrix, ProjectionMatrix, treeRotation); break;
                }
                case 3:
                {
                    drawObject(plant_b, -treePos - glm::vec3(1.0f, 0.0f, 1.0f), treeScale - glm::vec3(1.5f), shader, ViewMatrix, ProjectionMatrix, treeRotation);
                    drawObject(tree_d, treePos, treeScale, shader, ViewMatrix, ProjectionMatrix, treeRotation); break;
                }
                case 4:
                {
                    drawObject(rock_a, -treePos + glm::vec3(1.0f, 0.0f, 1.0f), treeScale - glm::vec3(1.5f), shader, ViewMatrix, ProjectionMatrix, treeRotation);
                    drawObject(tree_e, treePos, treeScale, shader, ViewMatrix, ProjectionMatrix, treeRotation); break;
                }
                case 5:
                {
                    drawObject(rock_b, -treePos - glm::vec3(1.0f, 0.0f, 1.0f), treeScale - glm::vec3(1.5f), shader, ViewMatrix, ProjectionMatrix, treeRotation);
                    drawObject(tree_f, treePos, treeScale, shader, ViewMatrix, ProjectionMatrix, treeRotation); break;
                }
                }

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

            glm::vec3 frogPos(frog_x, frog_y + 0.1f, frog_z);
            glm::vec3 frogScale(0.15f);

            drawObject(frog, frogPos, frogScale, shader, ViewMatrix, ProjectionMatrix, frogRotation * -58 - glm::half_pi<float>());

            if (isSolved_frog)
            {
                // ======================
                // DRAW EXIT
                // ======================

                drawObject(doorMesh, exitPos, glm::vec3(1.5f, 2.0f, 0.1f), shader, ViewMatrix, ProjectionMatrix, 0.0f, 4.0f);

                // ======================
                // EXIT INTERACTION
                // ======================
                float distToExit = glm::length(warlockPos - exitPos);
                if (distToExit < 1.0f)
                {
                    firstLoad = 1;
                    currentRoom = 5;
                    TriggerRoomChange();
                }
            }

        }
        else
        if (currentRoom == 5)
        {
            room2shader.use();
            glUniform3f(glGetUniformLocation(room2shader.getId(), "viewPos"), camera.getCameraPosition().x, camera.getCameraPosition().y, camera.getCameraPosition().z);
            glUniform3f(glGetUniformLocation(room2shader.getId(), "lightColor"), lightColor.x, lightColor.y, lightColor.z);
            glUniform3f(glGetUniformLocation(room2shader.getId(), "lightPos"), lightPos.x, lightPos.y, lightPos.z);
            glUniform1i(glGetUniformLocation(room2shader.getId(), "activeTorchCount"), HALL_TORCH_COUNT5);
            if (firstLoad == 1)
            {
                frogButton = loader.loadObj("Resources/Models/frog_button.obj", paint_gold_texture);

                warlockPos = glm::vec3(1.2f, 2.0f, 9.0f);
                knightPos = glm::vec3(-1.2f, 2.0f, 9.0f);
                warlockYaw = 180.0f;
                knightYaw = 180.0f;

                if (activeIsWarlock)
                    cameraCube.position = warlockPos;
                else
                    cameraCube.position = knightPos;

                camera.setCameraPosition(cameraCube.position);
                camera.setYaw(warlockYaw);

                if (currentTask == 10)
                {
                    currentTask = 11;
                    LoadDialogue(10);
                }

                firstLoad = 0;
            }
            exitPos = glm::vec3(0.0f, 2.0f, -6.8f);

            // ======================
            // DRAW FLOOR
            // ======================

            drawObject(floorCube, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(12.0f, 0.1f, 12.0f), shader, ViewMatrix, ProjectionMatrix, 0.0f, 32.0f);
            drawObject(floorCube, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(12.0f, 8.1f, 12.0f), shader, ViewMatrix, ProjectionMatrix, 0.0f, 32.0f);

            drawObject(decoDoor, glm::vec3(0.0f, 2.0f, 11.8f), glm::vec3(1.5f, 2.0f, 0.1f), room2shader, ViewMatrix, ProjectionMatrix, 180.0f, 4.0f);

            // ======================
            // DRAW WALLS
            // ======================
            backWall_5.draw(room2shader, ViewMatrix, ProjectionMatrix, 32.0f);
            frontWall_5.draw(room2shader, ViewMatrix, ProjectionMatrix, 32.0f);
            leftWall_5.draw(room2shader, ViewMatrix, ProjectionMatrix, 32.0f);
            rightWall_5.draw(room2shader, ViewMatrix, ProjectionMatrix, 32.0f);
            wall_5_a.draw(room2shader, ViewMatrix, ProjectionMatrix, 4.0f);
            wall_5_b.draw(room2shader, ViewMatrix, ProjectionMatrix, 4.0f);
            wall_5_c.draw(room2shader, ViewMatrix, ProjectionMatrix, 4.0f);

            colliders.push_back(backWall_5.getAABB());
            colliders.push_back(frontWall_5.getAABB());
            colliders.push_back(leftWall_5.getAABB());
            colliders.push_back(rightWall_5.getAABB());
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
                    drawObject(prisonDoor, doors[i].position, doors[i].scale, room2shader, ViewMatrix, ProjectionMatrix, doors[i].rotation);
                    if (i == 1 || i == 2 || i == 4 || i == 5 || i == 7)
                        colliders.push_back(makeAABB(doors[i].position, glm::vec3(0.1f, 2.0f, 2.0f)));
                    else
                        colliders.push_back(makeAABB(doors[i].position, doors[i].scale));
                }
                else
                    drawObject(prisonDoor, doors[i].position + glm::vec3(0.0f, 4.0f, 0.0f), doors[i].scale, room2shader, ViewMatrix, ProjectionMatrix, doors[i].rotation);
                doors[i].isUnlocked = false;
            }

            // =======================
            // FROG BUTTONS
            // =======================

            isSolved_wardrobe = false;

            for (int i = 0; i < BUTTON_COUNT; i++)
            {
                if( i == 0 )
                    drawObject(frogButton, buttons[i].position, buttons[i].scale, room2shader, ViewMatrix, ProjectionMatrix, 0.0f);
                else
                    drawObject(frogButton, buttons[i].position, buttons[i].scale, room2shader, ViewMatrix, ProjectionMatrix, 180.0f);

                float distToButton_W = glm::length(warlockPos - buttons[i].position);
                float distToButton_K = glm::length(knightPos - buttons[i].position);
                if (distToButton_W < 1.0f || distToButton_K < 1.0f)
                {
                    if (i == 0)
                    {
                        if (currentTask == 11)
                        {
                            LoadDialogue(11);
                            currentTask = 12;
                        }
                        isSolved_wardrobe = true;
                    }

                    for (int link : buttons[i].links)
                    {
                        doors[link].isUnlocked = true;
                    }
                }
            }

            // ======================
            // TORCH
            // ======================
            // light iters
            for (int i = 0; i < HALL_TORCH_COUNT5; i++)
            {
                std::string posName = "torchPos[" + std::to_string(i) + "]";
                std::string colorName = "torchColor[" + std::to_string(i) + "]";
                std::string onName = "torchOn[" + std::to_string(i) + "]";

                glm::vec3 p = hallTorches5[i]->position + glm::vec3(0.0f, 0.4f, 0.0f);

                glUniform3f(glGetUniformLocation(room2shader.getId(), posName.c_str()), p.x, p.y, p.z);
                glUniform3f(glGetUniformLocation(room2shader.getId(), colorName.c_str()), 1.0f, 0.5f, 0.2f);
                glUniform1i(glGetUniformLocation(room2shader.getId(), onName.c_str()), hallTorches5[i]->isOn ? 1 : 0);
            }

            for (int i = 0; i < HALL_TORCH_COUNT5; i++)
            {
                hallTorches5[i]->draw(room2shader, ViewMatrix, ProjectionMatrix, currentFrame);
            }

            if (isSolved_wardrobe)
            {
                // ======================
                // DRAW EXIT
                // ======================

                drawObject(doorMesh, exitPos, glm::vec3(1.5f, 2.0f, 0.1f), room2shader, ViewMatrix, ProjectionMatrix, 0.0f, 4.0f);

                // ======================
                // EXIT INTERACTION
                // ======================
                float distToExit = glm::length(warlockPos - exitPos);
                if (distToExit < 1.0f)
                {
                    firstLoad = 1;
                    currentRoom = 6;
                    TriggerRoomChange();
                }
            }
        }
        else
        if (currentRoom == 6)
        {
            shader.use();
            activeIsWarlock = true;
            activeIsWarlock = true;

            if (firstLoad == 1)
            {
                bedroomFloorCube = loader.loadObj("Resources/Models/cube.obj", bedroom_floor_texture);
                bedroomFloorCarpet = loader.loadObj("Resources/Models/cube.obj", bedroom_carpet_texture);

                bedroomBed = loader.loadObj("Resources/Models/bed.obj", paint_pink_texture);
                bedroomPiano = loader.loadObj("Resources/Models/piano.obj", paint_black_texture);
                bedroomDresser = loader.loadObj("Resources/Models/dresser.obj", paint_darkbrown_texture);
                bedroomTeddy = loader.loadObj("Resources/Models/teddy.obj", paint_purple_texture);
                
                princess = loader.loadObj("Resources/Models/princess.obj", paint_yellow_texture);

                warlockPos = glm::vec3(1.2f, 2.0f, 6.0f);
                knightPos = glm::vec3(-1.2f, 2.0f, 6.0f);
                warlockYaw = 180.0f;
                knightYaw = 0.0f;

                if (activeIsWarlock)
                    cameraCube.position = warlockPos;
                else
                    cameraCube.position = knightPos;

                camera.setCameraPosition(cameraCube.position);
                camera.setYaw(warlockYaw);

                if (currentTask == 12)
                {
                    currentTask = 13;
                    LoadDialogue(12);
                }

                firstLoad = 0;
            }

            // ======================
            // DRAW WALLS
            // ======================
            backWall_6.draw(shader, ViewMatrix, ProjectionMatrix, 32.0f);
            frontWall_6.draw(shader, ViewMatrix, ProjectionMatrix, 32.0f);
            leftWall_6.draw(shader, ViewMatrix, ProjectionMatrix, 32.0f);
            rightWall_6.draw(shader, ViewMatrix, ProjectionMatrix, 32.0f);

            colliders.push_back(backWall_6.getAABB());
            colliders.push_back(frontWall_6.getAABB());
            colliders.push_back(leftWall_6.getAABB());
            colliders.push_back(rightWall_6.getAABB());

            // ======================
            // DRAW FLOOR
            // ======================

            drawObject(bedroomFloorCube, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(9.0f, 0.1f, 9.0f), shader, ViewMatrix, ProjectionMatrix, 0.0f, 32.0f);
            drawObject(bedroomFloorCube, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(9.0f, 8.1f, 9.0f), shader, ViewMatrix, ProjectionMatrix, 0.0f, 32.0f);
            drawObject(bedroomFloorCarpet, glm::vec3(0.0f, 0.1f, 0.0f), glm::vec3(5.0f, 0.1f, 5.0f), shader, ViewMatrix, ProjectionMatrix, 0.0f, 32.0f);

            drawObject(decoDoor, glm::vec3(0.0f, 2.0f, 8.8f), glm::vec3(1.5f, 2.0f, 0.1f), room2shader, ViewMatrix, ProjectionMatrix, 180.0f, 4.0f);

            // ======================
            // DRAW DECORATIONS
            // ======================

            glm::vec3 bedPos(0.0f, 0.0f, -5.5f);
            glm::vec3 dresserPos(-6.0f, 0.0f, 5.5f);
            glm::vec3 pianoPos(6.0f, 0.0f, 5.5f);
            glm::vec3 teddyPos(-6.0, 0.0f, -5.5f);

            glm::vec3 windowPos(6.0f, 3.0f, -8.9f);
            glm::vec3 princessPos(-3.9f, 0.1f, 3.0f);
            glm::vec3 armorPos(-3.0f, 0.3f, 0.0f);
            glm::vec3 knifePos(2.0f, 1.1f, -1.0f);

            drawObject(bedroomBed, bedPos, glm::vec3(0.1f), shader, ViewMatrix, ProjectionMatrix, 0.0f);
            drawObject(bedroomDresser, dresserPos, glm::vec3(0.09f), shader, ViewMatrix, ProjectionMatrix, 120.0f);
            drawObject(bedroomPiano, pianoPos, glm::vec3(0.12f), shader, ViewMatrix, ProjectionMatrix, 210.0f);
            drawObject(bedroomTeddy, teddyPos, glm::vec3(3.0f), shader, ViewMatrix, ProjectionMatrix, 45.0f);

            // window
            drawObject(windowCube, windowPos, glm::vec3(1.8f, 1.8f, 0.1f), shader, ViewMatrix, ProjectionMatrix, 0.0f, 8.0f);
            // princess
            if (currentTask == 16)
                drawObjectSideways(princess, princessPos, glm::vec3(1.6f), shader, ViewMatrix, ProjectionMatrix, -90.0f);
            else
                drawObject(princess, princessPos, glm::vec3(1.6f), shader, ViewMatrix, ProjectionMatrix, 45.0f);
            // armor
            drawObjectSideways(knight, armorPos, glm::vec3(1.8f), shader, ViewMatrix, ProjectionMatrix, 90.0f);
            // knife
            if(currentTask == 14)
                drawObjectSideways(knife, knifePos, glm::vec3(2.0f), shader, ViewMatrix, ProjectionMatrix, 180.0f);

            static bool eKeyWasPressed = false;

            float distToPrincess = glm::length(warlockPos - princessPos);
            float distToKnife = glm::length(warlockPos - knifePos);

                if (window.isPressed(GLFW_KEY_E))
                {
                    if (!eKeyWasPressed)
                    {
                        if (currentTask == 13 && distToPrincess < 3.0f)
                        {
                            LoadDialogue(13);
                            currentTask = 14;
                        }
                        if (currentTask == 14 && distToKnife < 3.0f)
                        {
                            LoadDialogue(14);
                            currentTask = 15;
                        }
                        if (currentTask == 15 && distToPrincess < 3.0f)
                        {
                            LoadDialogue(15);
                            currentTask = 16;
                        }
                        eKeyWasPressed = true;
                    }
                }
                else {
                    eKeyWasPressed = false;
                }

            colliders.push_back(makeAABB(bedPos, glm::vec3(1.9f, 3.0f, 2.8f)));
            colliders.push_back(makeAABB(dresserPos, glm::vec3(2.0f, 3.0f, 2.0f)));
            colliders.push_back(makeAABB(pianoPos, glm::vec3(1.8f, 3.0f, 2.3f)));
            colliders.push_back(makeAABB(teddyPos, glm::vec3(1.5f, 3.0f, 1.5f)));
            colliders.push_back(makeAABB(princessPos, glm::vec3(0.7f, 3.0f, 0.7f)));
            colliders.push_back(makeAABB(armorPos - glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(1.5f, 3.0f, 0.6f)));

            float distToWindow = glm::length(warlockPos - windowPos);


        }
        // ======================
        // CHARACTER SWAP (SPACE)
        // ======================
        static bool spacePressedLastFrame = false;
        if (window.isPressed(GLFW_KEY_SPACE) && currentRoom != 6 && !(!currentDialogue.empty() && currentLineIndex < currentDialogue.size()))
        {
            if (currentTask == 1)
            {
                LoadDialogue(1);
                currentTask++;
            }
            if (!spacePressedLastFrame)
            {
                // SAVE current character state
                if (activeIsWarlock)
                {
                    warlockPos = cameraCube.position;
                    warlockYaw = camera.getYaw();
                }
                else
                {
                    knightPos = cameraCube.position;
                    knightYaw = camera.getYaw();
                }

                // SWITCH character
                activeIsWarlock = !activeIsWarlock;

                // LOAD new character state
                if (activeIsWarlock)
                {
                    cameraCube.position = warlockPos;
                    camera.setYaw(warlockYaw);
                }
                else
                {
                    cameraCube.position = knightPos;
                    camera.setYaw(knightYaw);
                }

                camera.setCameraPosition(cameraCube.position);

                spacePressedLastFrame = true;
            }
        }
        else
        {
            spacePressedLastFrame = false;
        }

        // ======================
        // ROTATION
        // ======================
        float rotationSpeed = 180.0f * deltaTime;
        if (!(!currentDialogue.empty() && currentLineIndex < currentDialogue.size()))
        {
            if (window.isPressed(GLFW_KEY_A)) camera.rotateOy(rotationSpeed);
            if (window.isPressed(GLFW_KEY_D)) camera.rotateOy(-rotationSpeed);
        }

        // ======================
        // MOVEMENT
        // ======================
        float speed = 5.0f * deltaTime;

        glm::vec3 forward = camera.getCameraViewDirection();
        forward.y = 0.0f;
        forward = glm::normalize(forward);

        glm::vec3 delta(0.0f);
        if (!(!currentDialogue.empty() && currentLineIndex < currentDialogue.size()))
        {
            if (window.isPressed(GLFW_KEY_W)) delta += forward * speed;
            if (window.isPressed(GLFW_KEY_S)) delta -= forward * speed;
        }

        // Move camera cube with collision
        movement(cameraCube.position, delta, cameraCube.halfSize, colliders);

        // ======================
        // UPDATE ACTIVE CHARACTER POSITION
        // ======================
        if (activeIsWarlock)
            warlockPos = cameraCube.position;
        else
            knightPos = cameraCube.position;

        // ======================
        // UPDATE CAMERA POSITION
        // ======================
        camera.setCameraPosition(cameraCube.position);


        // ==================================
        // DRAW UI / DIALOGUE BOX
        // ==================================

        glDisable(GL_DEPTH_TEST);

        // Only render if we have lines left
        if (!currentDialogue.empty() && currentLineIndex < currentDialogue.size())
        {
            // Get current line data
            DialogueLine& line = currentDialogue[currentLineIndex];

            // Draw black background for narrator when the case
            if (line.CharacterName == "black")
            {
                glUniform3f(glGetUniformLocation(diagShader.getId(), "color"), 0.0f, 0.0f, 0.0f); // Set Color to Black
                drawObject(dialogueBoxMesh, glm::vec3(window.getWidth() / 2.0f, window.getHeight() / 2.0f, 0.0f), glm::vec3(window.getWidth(), window.getHeight(), 1.0f), diagShader, glm::mat4(1.0f), textProjection, 0.0f);
            }

            // --------------------------
            // 1. Draw Background Box
            // --------------------------
            diagShader.use();
            glUniform1f(glGetUniformLocation(diagShader.getId(), "alpha"), 1.0f);
            // Pass Color
            glUniform3f(glGetUniformLocation(diagShader.getId(), "color"), 0.2f, 0.212f, 0.239f);


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
                    drawObject(dialogueBoxMesh, glm::vec3(200.0f, 150.0f, 0.0f), glm::vec3(150.0f, 150.0f, 1.0f), portraitShader, glm::mat4(1.0f), textProjection, 0.0f);
                else
                    drawObject(dialogueBoxMesh, glm::vec3(1400.0f, 150.0f, 0.0f), glm::vec3(150.0f, 150.0f, 1.0f), portraitShader, glm::mat4(1.0f), textProjection, 0.0f);
            }

            // --------------------------
            // 3. Render Text
            // --------------------------
            if((line.CharacterName!="none") && (line.CharacterName !="black"))
                // Character Name (Yellow)
                textRenderer.RenderText(textShader, line.CharacterName, 400.0f, 200.0f, 1.0f, glm::vec3(1.0f, 1.0f, 0.0f));

            // Dialogue Line (White)
            textRenderer.RenderWrappedText(textShader, line.Text, 400.0f, 130.0f, 0.7f, glm::vec3(1.0f, 1.0f, 1.0f), 50);
        }

        float hx = 25.0f;
        float hy = 850.0f;
        float hz = 0.8f;
        glm::vec3 white(1.0f, 1.0f, 1.0f);
        std::string hint;

        // --- Render Hints (Always visible) ---
        // Hints (Top Left)

        switch (currentTask)
        {
        case 1: { hint = "Swap to the Knight! (Press Space)"; break; }
        case 2: { hint = "Pick up the key. (Press E)"; break; }
        case 3: { hint = "Open the door."; break; }
        case 4: { hint = "Exit the Dungeon."; break; }
        case 5: { hint = "Read the book."; break; }
        case 6: { hint = "Solve the riddle. (Press E near a torch to toggle it)"; break; }
        case 7: { hint = "Exit the Hallway."; break; }
        case 8: { hint = "Find a clue to escape."; break; }
        case 9: { hint = "Exit the Library."; break; }
        case 10: { hint = "Exit the Garden."; break; }
        case 11: { hint = "Find a way to the exit."; break; }
        case 12: { hint = "Enter the Bedroom."; break; }
        case 13: { hint = "Talk to the Princess."; break; }
        case 14: { hint = "Grab the knife..."; break; }
        case 15: { hint = "Kill the Princess..."; break; }
        case 16: { hint = "Escape out the window."; break; }
        }
        if (!(!currentDialogue.empty() && currentLineIndex < currentDialogue.size()))
            textRenderer.RenderText(textShader, hint, hx, hy, hz, white);

        // ==========================================
        // TRANSITION LOGIC
        // ==========================================

        if (isFadingOut)
        {
            fadeAlpha += fadeSpeed * deltaTime;
            if (fadeAlpha >= 1.0f)
            {
                fadeAlpha = 1.0f;
                isFadingOut = false;
                isFadingIn = true;
            }
        }
        else if (isFadingIn)
        {
            fadeAlpha -= fadeInSpeed * deltaTime;
            if (fadeAlpha <= 0.0f)
            {
                fadeAlpha = 0.0f;
                isFadingIn = false;
            }
        }

        // ==========================================
        // DRAW FADE OVERLAY
        // ==========================================
        if (fadeAlpha > 0.0f)
        {
            glEnable(GL_BLEND);
            glDisable(GL_DEPTH_TEST);

            diagShader.use();

            glUniform3f(glGetUniformLocation(diagShader.getId(), "color"), 0.0f, 0.0f, 0.0f);

            glUniform1f(glGetUniformLocation(diagShader.getId(), "alpha"), fadeAlpha);

            glm::vec3 fadeScale = glm::vec3(window.getWidth(), window.getHeight(), 1.0f);
            glm::vec3 fadePos = glm::vec3(window.getWidth() / 2.0f, window.getHeight() / 2.0f, 0.0f);
            drawObject(dialogueBoxMesh, fadePos, fadeScale, diagShader, glm::mat4(1.0f), textProjection, 0.0f);

            glUniform1f(glGetUniformLocation(diagShader.getId(), "alpha"), 1.0f);

            glEnable(GL_DEPTH_TEST);
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
    //float cameraSpeed = 5.0 * deltaTime;
    //if (window.isPressed(GLFW_KEY_W)) camera.keyboardMoveFront(cameraSpeed);
    //if (window.isPressed(GLFW_KEY_S)) camera.keyboardMoveBack(cameraSpeed);
    //if (window.isPressed(GLFW_KEY_A)) camera.rotateOy(cameraSpeed * 15);
    //if (window.isPressed(GLFW_KEY_D)) camera.rotateOy(-cameraSpeed * 15);
}