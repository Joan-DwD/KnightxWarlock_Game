#version 330 core

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec2 vertexUV;
layout(location = 2) in vec3 vertexNormal;

out vec2 UV;
out vec3 FragPos;
out vec3 Normal;

uniform mat4 MVP;
uniform mat4 M;
uniform float time;

// Wave A, B, C: (dirX, dirY, steepness, wavelength)
uniform vec4 waveA;
uniform vec4 waveB;
uniform vec4 waveC;

const float PI = 3.14159265359;
const float g = 9.8; // gravity

vec3 GerstnerWave(vec4 wave, vec3 p, inout vec3 tangent, inout vec3 binormal) {
    float steepness = wave.z;
    float wavelength = wave.w;
    float k = 2.0 * PI / wavelength;
    float c = sqrt(g / k);
    vec2 d = normalize(wave.xy);
    float f = k * (dot(d, p.xz) - c * time);
    float a = steepness / k;
    
    // Accumulate tangent and binormal
    tangent += vec3(
        -d.x * d.x * (steepness * sin(f)),
        d.x * (steepness * cos(f)),
        -d.x * d.y * (steepness * sin(f))
    );
    binormal += vec3(
        -d.x * d.y * (steepness * sin(f)),
        d.y * (steepness * cos(f)),
        -d.y * d.y * (steepness * sin(f))
    );
    
    // Return wave offset
    return vec3(
        d.x * (a * cos(f)),
        a * sin(f),
        d.y * (a * cos(f))
    );
}

void main() {
    vec3 gridPoint = vertexPosition;
    vec3 tangent = vec3(1.0, 0.0, 0.0);
    vec3 binormal = vec3(0.0, 0.0, 1.0);
    vec3 p = gridPoint;
    
    // Accumulate waves
    p += GerstnerWave(waveA, gridPoint, tangent, binormal);
    p += GerstnerWave(waveB, gridPoint, tangent, binormal);
    p += GerstnerWave(waveC, gridPoint, tangent, binormal);
    
    // Calculate normal from tangent and binormal
    vec3 normal = normalize(cross(binormal, tangent));
    
    // Output
    gl_Position = MVP * vec4(p, 1.0);
    FragPos = vec3(M * vec4(p, 1.0));
    Normal = mat3(transpose(inverse(M))) * normal;
    UV = vertexUV;
}