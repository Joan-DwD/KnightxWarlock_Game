#pragma once

#include <gtc\matrix_transform.hpp>
#include <gtc\type_ptr.hpp>
#include <map>
#include <string>
#include <iostream>

#include "Shaders/shader.h"

// --- FreeType Includes ---
#include <ft2build.h>
#include FT_FREETYPE_H

struct Character {
    unsigned int TextureID; // ID of the glyph texture
    glm::ivec2   Size;      // Size of glyph
    glm::ivec2   Bearing;   // Offset from baseline to left/top of glyph
    unsigned int Advance;   // Offset to advance to next glyph
};

class TextRenderer {
public:
    // Constructor loads the font and sets up the VAO/VBO
    TextRenderer(const char* fontPath, int fontSize);

    // Main draw function
    void RenderText(Shader& shader, std::string text, float x, float y, float scale, glm::vec3 color);

private:
    std::map<GLchar, Character> Characters;
    unsigned int VAO = 0;
    unsigned int VBO = 0;
    // Success flag for initilizing
    bool isInitialized = false;
};