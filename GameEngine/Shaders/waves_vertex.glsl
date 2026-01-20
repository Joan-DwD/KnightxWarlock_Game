#version 330 core
layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec2 vertexUV;

uniform mat4 MVP;
uniform mat4 model;
uniform float time;
uniform vec2 uvScale;

out vec3 FragPos;
out vec3 Normal;
out vec2 UV;

vec3 gerstnerWaveNormal(vec3 pos, float time) {
    vec3 tangent = vec3(1.0, 0.0, 0.0);
    vec3 binormal = vec3(0.0, 0.0, 1.0);
    
    float wavelength1 = 0.6;
    float amplitude1 = 0.04;  // Matched to gerstnerWave
    vec2 direction1 = vec2(1.0, 0.0);
    float k1 = 2.0 * 3.14159 / wavelength1;
    float c1 = sqrt(9.8 / k1);
    vec2 d1 = normalize(direction1);
    float f1 = k1 * (dot(d1, pos.xz) - c1 * time);
    float wa1 = k1 * amplitude1;
    
    tangent.x -= d1.x * d1.x * wa1 * sin(f1);
    tangent.y += d1.x * wa1 * cos(f1);
    tangent.z -= d1.x * d1.y * wa1 * sin(f1);
    
    binormal.x -= d1.x * d1.y * wa1 * sin(f1);
    binormal.y += d1.y * wa1 * cos(f1);
    binormal.z -= d1.y * d1.y * wa1 * sin(f1);
    
    float wavelength2 = 0.3;
    float amplitude2 = 0.02;  // Matched to gerstnerWave
    vec2 direction2 = vec2(0.6, 0.8);
    float k2 = 2.0 * 3.14159 / wavelength2;
    float c2 = sqrt(9.8 / k2);
    vec2 d2 = normalize(direction2);
    float f2 = k2 * (dot(d2, pos.xz) - c2 * time);
    float wa2 = k2 * amplitude2;
    
    tangent.x -= d2.x * d2.x * wa2 * sin(f2);
    tangent.y += d2.x * wa2 * cos(f2);
    tangent.z -= d2.x * d2.y * wa2 * sin(f2);
    
    binormal.x -= d2.x * d2.y * wa2 * sin(f2);
    binormal.y += d2.y * wa2 * cos(f2);
    binormal.z -= d2.y * d2.y * wa2 * sin(f2);
    
    float wavelength3 = 0.15;
    float amplitude3 = 0.01;  // Matched to gerstnerWave
    vec2 direction3 = vec2(-0.7, 0.7);
    float k3 = 2.0 * 3.14159 / wavelength3;
    float c3 = sqrt(9.8 / k3);
    vec2 d3 = normalize(direction3);
    float f3 = k3 * (dot(d3, pos.xz) - c3 * time);
    float wa3 = k3 * amplitude3;
    
    tangent.x -= d3.x * d3.x * wa3 * sin(f3);
    tangent.y += d3.x * wa3 * cos(f3);
    tangent.z -= d3.x * d3.y * wa3 * sin(f3);
    
    binormal.x -= d3.x * d3.y * wa3 * sin(f3);
    binormal.y += d3.y * wa3 * cos(f3);
    binormal.z -= d3.y * d3.y * wa3 * sin(f3);
    
    return normalize(cross(binormal, tangent));
}
vec3 gerstnerWave(vec3 pos, float time) {
    vec3 offset = vec3(0.0);
    
    // Wave 1 - Longer, smoother
    float wavelength1 = 1.2;  // Was 0.6 - doubled for smoother
    float amplitude1 = 0.02;  // Was 0.04 - reduced
    vec2 direction1 = vec2(1.0, 0.0);
    float k1 = 2.0 * 3.14159 / wavelength1;
    float c1 = sqrt(9.8 / k1);
    vec2 d1 = normalize(direction1);
    float f1 = k1 * (dot(d1, pos.xz) - c1 * time);
    offset.x += d1.x * (amplitude1 * cos(f1));
    offset.y += amplitude1 * sin(f1);
    offset.z += d1.y * (amplitude1 * cos(f1));
    
    // Wave 2 - Longer, smoother
    float wavelength2 = 0.8;  // Was 0.3
    float amplitude2 = 0.015;  // Was 0.02
    vec2 direction2 = vec2(0.6, 0.8);
    float k2 = 2.0 * 3.14159 / wavelength2;
    float c2 = sqrt(9.8 / k2);
    vec2 d2 = normalize(direction2);
    float f2 = k2 * (dot(d2, pos.xz) - c2 * time);
    offset.x += d2.x * (amplitude2 * cos(f2));
    offset.y += amplitude2 * sin(f2);
    offset.z += d2.y * (amplitude2 * cos(f2));
    
    // Wave 3 - Longer, smoother
    float wavelength3 = 0.4;  // Was 0.15
    float amplitude3 = 0.008;  // Was 0.01
    vec2 direction3 = vec2(-0.7, 0.7);
    float k3 = 2.0 * 3.14159 / wavelength3;
    float c3 = sqrt(9.8 / k3);
    vec2 d3 = normalize(direction3);
    float f3 = k3 * (dot(d3, pos.xz) - c3 * time);
    offset.x += d3.x * (amplitude3 * cos(f3));
    offset.y += amplitude3 * sin(f3);
    offset.z += d3.y * (amplitude3 * cos(f3));
    
    return pos + offset;
}
void main() {
    vec3 pos = gerstnerWave(vertexPosition, time * 0.5);  // Slow down by half
    vec3 normal = gerstnerWaveNormal(vertexPosition, time * 0.5);
    
    gl_Position = MVP * vec4(pos, 1.0);
    FragPos = vec3(model * vec4(pos, 1.0));
    Normal = mat3(transpose(inverse(model))) * normal;
    UV = vertexUV * uvScale;
}