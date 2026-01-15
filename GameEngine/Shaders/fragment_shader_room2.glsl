#version 400

out vec4 fragColor;

in vec3 FragPos; // from vertex
in vec3 Normal;
in vec2 TexCoords;

uniform sampler2D texture_diffuse1; 

uniform vec3 lightPos;
uniform vec3 lightColor;

uniform vec3 viewPos;

// torch stuff v
#define MAX_TORCHES 10
uniform vec3 torchPos[MAX_TORCHES];
uniform vec3 torchColor[MAX_TORCHES];
uniform int torchOn[MAX_TORCHES];
uniform int activeTorchCount;
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
    vec3 difuse = diff * lightColor;

    // torch stuff v
    vec3 totalTorchResult = vec3(0.0); // start w no light

    for(int i = 0; i < activeTorchCount; i++) 
    {
        if (torchOn[i] == 1) {
            vec3 tPos = torchPos[i];
            vec3 tCol = torchColor[i];

            vec3 torchDir = normalize(tPos - FragPos);
            float distance = length(tPos - FragPos);

            // attenuation
            float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * (distance * distance));

            // diffuse
            float diff2 = max(dot(norm, torchDir), 0.0);
            vec3 torchDiffuse = diff2 * tCol;

            // specular
            float specularStrength = 0.5;
            vec3 viewDir = normalize(viewPos - FragPos);
            vec3 reflectDir = reflect(-torchDir, norm);
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0); 
            vec3 torchSpecular = specularStrength * spec * tCol;

            totalTorchResult += (torchDiffuse + torchSpecular) * attenuation;
        }
    }
    // torch stuff ^
    
    vec4 objectColor = texture(texture_diffuse1, TexCoords);
    vec3 result = (ambient + difuse + totalTorchResult) * objectColor.rgb;
    
    fragColor = vec4(result, 1.0);
}