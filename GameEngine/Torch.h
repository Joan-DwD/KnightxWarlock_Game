#pragma once
#include "Shaders/shader.h"
#include "Model Loading/mesh.h"

#include <glm.hpp>
#include <gtx/transform.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

class Torch {
public:
    glm::vec3 position;
    bool isOn;

    Mesh* bodyMesh;  // stick
    Mesh* flameMesh; // cube

    Torch(Mesh* body, Mesh* flame, glm::vec3 pos, float rotY);

    void draw(Shader& shader, glm::mat4 viewMatrix, glm::mat4 projectionMatrix);

    void toggle();

    bool isPlayerClose(glm::vec3 playerPos, float radius) const;
};