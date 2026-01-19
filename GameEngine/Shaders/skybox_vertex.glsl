#version 400

layout (location = 0) in vec3 pos;

out vec3 fragPos;

uniform mat4 VP;

void main()
{
    fragPos = pos;
    vec4 position = VP * vec4(pos, 1.0);
    // Set z to w so depth is always 1.0 (far plane) after perspective divide
    gl_Position = position.xyww;
}