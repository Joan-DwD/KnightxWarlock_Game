#pragma once

#include <glm.hpp>
#include <vector>

struct AABB
{
	glm::vec3 min;
	glm::vec3 max;
};

inline AABB makeAABB(const glm::vec3& pos, const glm::vec3& halfSize)
{
	return { pos - halfSize, pos + halfSize };
}

inline bool AABBintersect(const AABB& a, const AABB& b)
{
	return (a.min.x <= b.max.x && a.max.x >= b.min.x) &&
			(a.min.y <= b.max.y && a.max.y >= b.min.y) &&
			(a.min.z <= b.max.z && a.max.z >= b.min.z);
}

inline void movement(glm::vec3& pos, const glm::vec3& delta, const glm::vec3& halfSize, const std::vector<AABB>& colliders)
{
    glm::vec3 newPos = pos;

    // X axis
    newPos.x += delta.x;
    AABB box = makeAABB(newPos, halfSize);
    for (const auto& c : colliders)
    {
        if (AABBintersect(box, c))
        {
            newPos.x = pos.x; // revert X only
            break;
        }
    }

    // Z axis
    newPos.z += delta.z;
    box = makeAABB(newPos, halfSize);
    for (const auto& c : colliders)
    {
        if (AABBintersect(box, c))
        {
            newPos.z = pos.z; // revert Z only
            break;
        }
    }

    pos = newPos;
}
