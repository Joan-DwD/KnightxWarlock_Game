#include "Camera\camera.h"
#include "Shaders\shader.h"
#include "Model Loading\mesh.h"
#include "Model Loading\texture.h"
#include "Model Loading\meshLoaderObj.h"
#include <iostream>
#include <cmath>
#include <map>
#include <string>

// --- FreeType Includes (for text rendering) ---
#include <ft2build.h>
#include FT_FREETYPE_H

// --- Custom Class Includes ---
#include "Wall.h"

// ======================
// TEXT RENDERING STRUCTS
// ======================
struct Character {
    unsigned int TextureID; // ID of the glyph texture
    glm::ivec2   Size;
    glm::ivec2   Bearing;
    unsigned int Advance;
};

std::map<GLchar, Character> Characters;
unsigned int textVAO, textVBO;

// ======================
// FUNCTION DECLARATIONS
// ======================
void processKeyboardInput();
void RenderText(Shader& shader, std::string text, float x, float y, float scale, glm::vec3 color);

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
glm::vec3 lightColor = glm::vec3(0.8f, 0.6f, 0.4f);
glm::vec3 lightPos = glm::vec3(6.5f, 4.0f, 0.0f);

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


    // ======================
    // --- FREETYPE SETUP ---
    // ======================
    FT_Library ft;
    if (FT_Init_FreeType(&ft)) {
        std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
        return -1;
    }

    FT_Face face;
    // Make sure this path exists on your PC!
    if (FT_New_Face(ft, "C:/Windows/Fonts/arial.ttf", 0, &face)) {
        std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
        return -1;
    }

    FT_Set_Pixel_Sizes(face, 0, 48);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    for (unsigned char c = 0; c < 128; c++)
    {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
            continue;
        }
        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, face->glyph->bitmap.width, face->glyph->bitmap.rows, 0, GL_RED, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        Character character = {
            texture,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            static_cast<unsigned int>(face->glyph->advance.x)
        };
        Characters.insert(std::pair<char, Character>(c, character));
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    glGenVertexArrays(1, &textVAO);
    glGenBuffers(1, &textVBO);
    glBindVertexArray(textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Setup Text Projection (Orthographic)
    glm::mat4 textProjection = glm::ortho(0.0f, static_cast<float>(window.getWidth()), 0.0f, static_cast<float>(window.getHeight()));
    textShader.use();
    glUniformMatrix4fv(glGetUniformLocation(textShader.getId(), "projection"), 1, GL_FALSE, glm::value_ptr(textProjection));


    // ======================
    // OBJECT POSITIONS
    // ======================
    glm::vec3 warlockPos = glm::vec3(3.0f, 2.0f, 3.0f);
    glm::vec3 knightPos = glm::vec3(0.0f, 2.0f, 0.0f);
    bool activeIsWarlock = true; // start controlling Warlock

    glm::vec3 keyPos = glm::vec3(-3.0f, 0.2f, -3.0f);   // on floor
    bool keyCollected = false;
    bool hasKey = false;

    glm::vec3 doorPos = glm::vec3(0.0f, 2.0f, -6.9f);
    bool doorUnlocked = false;

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
        // WALLS (now using class !)
        // ======================
        backWall.draw(shader, ViewMatrix, ProjectionMatrix);
        frontWall.draw(shader, ViewMatrix, ProjectionMatrix);
        leftWall.draw(shader, ViewMatrix, ProjectionMatrix);
        rightWall.draw(shader, ViewMatrix, ProjectionMatrix);

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
        // PAWN MOVEMENT
        // ======================
        float speed = 5.0f * deltaTime;
        if (activeIsWarlock)
        {
            if (window.isPressed(GLFW_KEY_W)) warlockPos.z -= speed;
            if (window.isPressed(GLFW_KEY_S)) warlockPos.z += speed;
            if (window.isPressed(GLFW_KEY_A)) warlockPos.x -= speed;
            if (window.isPressed(GLFW_KEY_D)) warlockPos.x += speed;
        }
        else
        {
            if (window.isPressed(GLFW_KEY_W)) knightPos.z -= speed;
            if (window.isPressed(GLFW_KEY_S)) knightPos.z += speed;
            if (window.isPressed(GLFW_KEY_A)) knightPos.x -= speed;
            if (window.isPressed(GLFW_KEY_D)) knightPos.x += speed;
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
        ModelMatrix = glm::mat4(1.0f);
        ModelMatrix = glm::translate(ModelMatrix, doorPos);
        ModelMatrix = glm::scale(ModelMatrix, glm::vec3(2.0f, 2.0f, 0.1f));
        MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
        glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
        glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
        doorMesh.draw(shader);

        // ======================
        // --- RENDER TEXT ---
        // ======================
        RenderText(textShader, "This is sample text", 25.0f, 25.0f, 1.0f, glm::vec3(0.5f, 0.8f, 0.2f));

        window.update();
    }

    return 0;
}

// ======================
// --- RENDER TEXT FUNCTION IMPLEMENTATION ---
// ======================
void RenderText(Shader& shader, std::string text, float x, float y, float scale, glm::vec3 color)
{
    shader.use();
    glUniform3f(glGetUniformLocation(shader.getId(), "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(textVAO);

    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++)
    {
        Character ch = Characters[*c];

        float xpos = x + ch.Bearing.x * scale;
        float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;

        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },

            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }
        };

        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        glBindBuffer(GL_ARRAY_BUFFER, textVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        x += (ch.Advance >> 6) * scale;
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
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