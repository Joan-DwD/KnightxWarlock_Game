#include "Wall.h"

Wall::Wall(Mesh* meshRef, glm::vec3 pos, glm::vec3 sc)
    : mesh(meshRef), position(pos), scale(sc)
{
}

void Wall::draw(Shader& shader, glm::mat4 viewMatrix, glm::mat4 projectionMatrix, float tiling) {
    // Calculate Model Matrix
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::scale(model, scale);

    glm::mat4 mvp = projectionMatrix * viewMatrix * model;

    GLuint matrixID = glGetUniformLocation(shader.getId(), "MVP");
    GLuint modelID = glGetUniformLocation(shader.getId(), "model");

    // Send to shader
    glUniformMatrix4fv(matrixID, 1, GL_FALSE, &mvp[0][0]);
    glUniformMatrix4fv(modelID, 1, GL_FALSE, &model[0][0]);

    // tiling
    glUniform2f(glGetUniformLocation(shader.getId(), "uvScale"), tiling, tiling);

    // Draw the actual mesh
    mesh->draw(shader);
}