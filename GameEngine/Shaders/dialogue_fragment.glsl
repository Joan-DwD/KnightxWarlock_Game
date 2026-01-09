#version 330 core
out vec4 FragColor;

uniform vec3 color; // Simple RGB color

void main()
{
    // Output the solid color with 1.0 alpha (Opaque)
    FragColor = vec4(color, 1.0);
}