#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal; // We ignore this for UI
layout (location = 2) in vec2 aTexCoords;

out vec2 TexCoords;

uniform mat4 model;
uniform mat4 projection; // Orthographic projection

void main()
{
    // We ignore View matrix usually for UI, or assume it's Identity
    gl_Position = projection * model * vec4(aPos, 1.0);
    TexCoords = aTexCoords;
}