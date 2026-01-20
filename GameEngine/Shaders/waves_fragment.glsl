#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 UV;

uniform vec3 viewPos;
uniform vec3 lightPos;
uniform vec3 lightColor;

void main() {
    vec3 waterColor = vec3(0.1, 0.30, 1.0);
    
    float ambientStrength = 0.65;
    vec3 ambient = ambientStrength * waterColor;
    
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0) * 0.35;
    vec3 diffuse = diff * waterColor;
    
    // Reduced specular for less white
    float specularStrength = 0.15;  // Was 0.3
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 20); 
    vec3 specular = specularStrength * spec * vec3(0.7, 0.85, 1.0);  
    
    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 0.7);
}