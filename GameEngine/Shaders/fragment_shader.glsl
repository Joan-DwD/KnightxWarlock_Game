#version 400

out vec4 fragColor;

in vec3 FragPos; // from vertex
in vec3 Normal;
in vec2 TexCoords;

uniform sampler2D texture1; 

uniform vec3 lightPos;
uniform vec3 lightColor;

void main()
{
    // small amount of ambient light
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor;

    // diffuse light
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    
        // dot product
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    vec4 objectColor = texture(texture1, TexCoords);
    
    vec3 result = (ambient + diffuse) * objectColor.rgb;
    
    fragColor = vec4(result, objectColor.a);
}