#pragma once

#include <glm/glm.hpp>

struct tParticle
{
    glm::vec4 Position; // xyz = position, w = mass
    glm::vec4 Velocity; // xyz = velocity
};