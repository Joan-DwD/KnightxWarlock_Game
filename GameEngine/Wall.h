#pragma once
#include "Shaders/shader.h"
#include "Model Loading/mesh.h"

#include <glm.hpp>
#include <gtx\transform.hpp>
#include <gtc\matrix_transform.hpp>
#include <gtc\type_ptr.hpp>

#include "Collision.h"

class Wall {
public:
    glm::vec3 position;
    glm::vec3 scale;
    Mesh* mesh; // Pointer to the shared mesh so we dont load it every time

    AABB getAABB() const
    {
        return makeAABB(position, scale);
    }

    Wall(Mesh* meshRef, glm::vec3 pos, glm::vec3 sc);

    void draw(Shader& shader, glm::mat4 viewMatrix, glm::mat4 projectionMatrix);
};