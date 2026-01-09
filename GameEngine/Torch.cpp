#include "Torch.h"

Torch::Torch(Mesh* body, Mesh* flame, glm::vec3 pos, float rotY)
    : bodyMesh(body), flameMesh(flame), position(pos), isOn(true)
{
}

void Torch::toggle() {
    isOn = !isOn;
}

bool Torch::isPlayerClose(glm::vec3 playerPos, float radius) const {
    return glm::distance(position, playerPos) < radius;
}

void Torch::draw(Shader& shader, glm::mat4 viewMatrix, glm::mat4 projectionMatrix) {

    glm::mat4 model = glm::mat4(1.0f);

    // move to wall position
    model = glm::translate(model, position);
    glm::mat4 orientationMatrix = model;

    // scale stick
    glm::mat4 bodyModel = glm::scale(model, glm::vec3(0.1f, 0.6f, 0.1f));

    glm::mat4 mvp = projectionMatrix * viewMatrix * bodyModel;
    glUniformMatrix4fv(glGetUniformLocation(shader.getId(), "MVP"), 1, GL_FALSE, &mvp[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shader.getId(), "model"), 1, GL_FALSE, &bodyModel[0][0]);
    bodyMesh->draw(shader);

    // only draw flame if the torch is on
    if (isOn) {
        glm::mat4 flameModel = orientationMatrix;

        // move flame up
        flameModel = glm::translate(flameModel, glm::vec3(0.0f, 0.35f, 0.0f));

        // scale the flame
        flameModel = glm::scale(flameModel, glm::vec3(0.2f, 0.2f, 0.2f));

        mvp = projectionMatrix * viewMatrix * flameModel;
        glUniformMatrix4fv(glGetUniformLocation(shader.getId(), "MVP"), 1, GL_FALSE, &mvp[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(shader.getId(), "model"), 1, GL_FALSE, &flameModel[0][0]);

        flameMesh->draw(shader);
    }
}