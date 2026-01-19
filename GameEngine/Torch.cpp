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

void Torch::draw(Shader& defaultShader, glm::mat4 viewMatrix, glm::mat4 projectionMatrix, float time) {

    glm::mat4 model = glm::mat4(1.0f);

    // move to wall position
    model = glm::translate(model, position);
    glm::mat4 orientationMatrix = model;
    defaultShader.use();

    // scale stick
    glm::mat4 bodyModel = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f)); //default scaling

    glm::mat4 mvp = projectionMatrix * viewMatrix * bodyModel;
    glUniformMatrix4fv(glGetUniformLocation(defaultShader.getId(), "MVP"), 1, GL_FALSE, &mvp[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(defaultShader.getId(), "model"), 1, GL_FALSE, &bodyModel[0][0]);
    bodyMesh->draw(defaultShader);

    // only draw flame if the torch is on
    if (isOn) {
        defaultShader.use();

        // lower opacity stuff
        glEnable(GL_BLEND);
        // GL_ONE for additive blending (glow effect)
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glDepthMask(GL_FALSE);
        glm::mat4 flameModel = orientationMatrix;

        // dynamic flame: 1st arg - speed, 2nd arg - height (intensity)
        float xmove = cos(time * 15.0f) * 0.015f; // (left/right)
        float ymove = sin(time * 10.0f) * 0.02f; // (up/down)
        float zmove = sin(time * 12.0f) * 0.015f; // (forward/back)

        // offset to move flame up
        flameModel = glm::translate(flameModel, glm::vec3(xmove, 0.60f + ymove, zmove));

        // pulsing
        float baseScale = 0.2f;
        float scaleFlicker = baseScale + (sin(time * 20.0f) * 0.03f);

        // scale the flame
        flameModel = glm::scale(flameModel, glm::vec3(scaleFlicker, scaleFlicker * 1.3f, scaleFlicker));

        mvp = projectionMatrix * viewMatrix * flameModel;
        glUniformMatrix4fv(glGetUniformLocation(defaultShader.getId(), "MVP"), 1, GL_FALSE, &mvp[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(defaultShader.getId(), "model"), 1, GL_FALSE, &flameModel[0][0]);

        // drawn flame
        flameMesh->draw(defaultShader);

        // core of flame
        glm::mat4 coreModel = glm::scale(flameModel, glm::vec3(0.7f));
        glUniformMatrix4fv(glGetUniformLocation(defaultShader.getId(), "model"), 1, GL_FALSE, &coreModel[0][0]);
        flameMesh->draw(defaultShader);

        // particles
        for (int i = 0; i < 3; i++)
        {
            glm::mat4 emberModel = orientationMatrix;

            // each i - different particle cube
            float speed = 0.8f;
            float heightLimit = 1.5f; // height before vanishing
            float timeOffset = i * 5.0f; // time delay

            // fmod - float modulus, 2nd arg is cap before starting back from bottom y
            float currentY = fmod((time * speed) + timeOffset, heightLimit);

            // spiraling - rotation based on current height
            float radius = 0.15f;
            float wiggleX = sin(currentY * 5.0f + i) * radius;
            float wiggleZ = cos(currentY * 5.0f + i) * radius;

            // fade - shrink based on height
            float lifePercentage = currentY / heightLimit; // 0 at heightLimit
            float emberScale = 0.08f * (1.0f - lifePercentage);

            // apply transformations (same height offset)
            emberModel = glm::translate(emberModel, glm::vec3(wiggleX, 0.6f + currentY, wiggleZ));
            emberModel = glm::scale(emberModel, glm::vec3(emberScale));

            // draw current particle
            mvp = projectionMatrix * viewMatrix * emberModel;
            glUniformMatrix4fv(glGetUniformLocation(defaultShader.getId(), "MVP"), 1, GL_FALSE, &mvp[0][0]);
            glUniformMatrix4fv(glGetUniformLocation(defaultShader.getId(), "model"), 1, GL_FALSE, &emberModel[0][0]);
            flameMesh->draw(defaultShader);
        }

        // cleanup (neccesary!! breaks walls otherwise)
        glDepthMask(GL_TRUE);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        defaultShader.use();
    }
}