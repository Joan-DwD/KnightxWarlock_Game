#version 330 core

in vec2 UV;
in vec3 FragPos;
in vec3 Normal;

out vec4 color;

uniform sampler2D textureSampler;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 waterColor;

void main() {
    // Ambient
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * waterColor;
    
    // Diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * waterColor;
    
    // Specular
    float specularStrength = 0.8;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * vec3(1.0);
    
    vec3 result = ambient + diffuse + specular;
    
    // Add some transparency
    color = vec4(result, 0.7);
}