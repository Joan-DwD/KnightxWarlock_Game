#version 400

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoords;

out vec3 FragPos;   // outputs to fragment shader
out vec3 Normal;
out vec2 TexCoords;

uniform mat4 MVP;
uniform mat4 model; // needed to calculate World Space position

void main()
{
    // calculate the position of the pixel in the world
    FragPos = vec3(model * vec4(pos, 1.0f));

    // calculating the normal in world space
    Normal = mat3(transpose(inverse(model))) * normal;

    // texture coordinates
    TexCoords = texCoords;

    gl_Position = MVP * vec4(pos, 1.0f);
}