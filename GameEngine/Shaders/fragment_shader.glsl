#version 400

out vec4 fragColor;

in vec3 FragPos; // from vertex
in vec3 Normal;
in vec2 TexCoords;

uniform sampler2D texture1; 

uniform vec3 lightPos;
uniform vec3 lightColor;

// torch stuff v
uniform vec3 torchPos;
uniform vec3 torchColor;
uniform bool torchOn;
uniform vec3 viewPos;
// torch stuff ^

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

    // torch stuff v
    vec3 torchResult = vec3(0.0); // start w no light

    if (torchOn) {
        vec3 torchDir = normalize(torchPos - FragPos);
        float distance = length(torchPos - FragPos);

        // attenuation
        float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * (distance * distance));

        // diffuse
        float diff2 = max(dot(norm, torchDir), 0.0);
        vec3 torchDiffuse = diff2 * torchColor;

        // specular
        float specularStrength = 0.5; // How shiny? (0.0 to 1.0)
        vec3 viewDir = normalize(viewPos - FragPos);
        vec3 reflectDir = reflect(-torchDir, norm);
        // 32 gives how shiny
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0); 
        vec3 torchSpecular = specularStrength * spec * torchColor;

        torchResult = (torchDiffuse + torchSpecular) * attenuation;
    }
    // torch stuff ^
    
    // old
    // vec3 result = (ambient + diffuse) * objectColor.rgb;

    // after torch
    vec3 result = (ambient + diffuse + torchResult) * objectColor.rgb;
    
    fragColor = vec4(result, objectColor.a);
}