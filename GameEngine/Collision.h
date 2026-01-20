#pragma once

#include <vector>
#include <glm.hpp>
#include <algorithm>

// Axis-Aligned Bounding Box 
struct AABB {
    glm::vec3 min;
    glm::vec3 max;

    // Check if this box overlaps with other box
    bool intersects(const AABB& other) const {
        return (min.x <= other.max.x && max.x >= other.min.x) &&
            (min.y <= other.max.y && max.y >= other.min.y) &&
            (min.z <= other.max.z && max.z >= other.min.z);
    }

    bool fitsInside(const AABB& region) const {
        return (min.x >= region.min.x && max.x <= region.max.x) &&
            (min.y >= region.min.y && max.y <= region.max.y) &&
            (min.z >= region.min.z && max.z <= region.max.z);
    }
};

// create AABB from position and half-size
inline AABB makeAABB(const glm::vec3& pos, const glm::vec3& halfSize) {
    return { pos - halfSize, pos + halfSize };
}

// The actual game object stored in the tree
struct Collider {
    AABB box;
    int typeID;         // 0 = Wall, 1 = Key, 2 = Door, etc
    bool active = true; // Use this to disable picked-up keys or unlocked doors
    void* userData = nullptr; // not used yet
};

// Type IDs for different collider types
enum ColliderType {
    COLLIDER_WALL = 0,
    COLLIDER_KEY = 1,
    COLLIDER_DOOR = 2,
    COLLIDER_PLAYER = 3
};


class OctreeNode {
private:
    AABB region;
    std::vector<Collider*> objects;
    OctreeNode* children[8];
    bool isLeaf;
    int depth;

    // Config: Max objects before split, Max depth
    const int MAX_OBJECTS = 4;
    const int MAX_DEPTH = 6;

public:
    // Constructor
    OctreeNode(AABB bounds, int currentDepth = 0) {
        region = bounds;
        depth = currentDepth;
        isLeaf = true;
        for (int i = 0; i < 8; i++) children[i] = nullptr;
    }

    // Destructor
    ~OctreeNode() {
        for (int i = 0; i < 8; i++) {
            if (children[i]) delete children[i];
        }
    }

    // Get the region bounds
    const AABB& getRegion() const { return region; }

    // Insert an object into the tree
    void insert(Collider* obj) {
        // If we have children, try to push object down
        if (!isLeaf) {
            bool foundChild = false;
            for (int i = 0; i < 8; i++) {
                // Only push down if it fits strictly inside the child
                if (obj->box.fitsInside(children[i]->region)) {
                    children[i]->insert(obj);
                    foundChild = true;
                    break;
                }
            }
            // If it overlaps boundaries, keep it here
            if (!foundChild) objects.push_back(obj);
        }
        else {
            // We are a leaf, just add it
            objects.push_back(obj);

            // Split if we exceed capacity
            if (objects.size() > MAX_OBJECTS && depth < MAX_DEPTH) {
                split();

                // Re-distribute existing objects to new children
                std::vector<Collider*> remainingObjects;
                for (Collider* existing : objects) {
                    bool moved = false;
                    for (int i = 0; i < 8; i++) {
                        if (existing->box.fitsInside(children[i]->region)) {
                            children[i]->insert(existing);
                            moved = true;
                            break;
                        }
                    }
                    if (!moved) remainingObjects.push_back(existing);
                }
                objects = remainingObjects; // Only keep the ones that couldn't move down, in our case we moved key and door to children
            }
        }
    }

    // Retrieve only relevant colliders
    void getPotentialCollisions(const AABB& queryBox, std::vector<Collider*>& results) const {
        // Check objects in this node
        for (Collider* obj : objects) {
            if (obj->active && queryBox.intersects(obj->box)) {
                results.push_back(obj);
            }
        }

        // Recurse into children ONLY if the box touches their area
        if (!isLeaf) {
            for (int i = 0; i < 8; i++) {
                if (queryBox.intersects(children[i]->region)) {
                    children[i]->getPotentialCollisions(queryBox, results);
                }
            }
        }
    }

private:
    // divide the node into 8 octants
    void split() {
        isLeaf = false;
        glm::vec3 mn = region.min;
        glm::vec3 mx = region.max;
        glm::vec3 center = (mn + mx) / 2.0f;

        // Create AABB bounds for each octant
        AABB bounds[8];

        // Bottom Half (Y = min to center)
        bounds[0].min = mn;
        bounds[0].max = center;

        bounds[1].min = glm::vec3(center.x, mn.y, mn.z);
        bounds[1].max = glm::vec3(mx.x, center.y, center.z);

        bounds[2].min = glm::vec3(mn.x, mn.y, center.z);
        bounds[2].max = glm::vec3(center.x, center.y, mx.z);

        bounds[3].min = glm::vec3(center.x, mn.y, center.z);
        bounds[3].max = glm::vec3(mx.x, center.y, mx.z);

        // Top Half (Y = center to max)
        bounds[4].min = glm::vec3(mn.x, center.y, mn.z);
        bounds[4].max = glm::vec3(center.x, mx.y, center.z);

        bounds[5].min = glm::vec3(center.x, center.y, mn.z);
        bounds[5].max = glm::vec3(mx.x, mx.y, center.z);

        bounds[6].min = glm::vec3(mn.x, center.y, center.z);
        bounds[6].max = glm::vec3(center.x, mx.y, mx.z);

        bounds[7].min = center;
        bounds[7].max = mx;

        // Create children nodes
        for (int i = 0; i < 8; i++) {
            children[i] = new OctreeNode(bounds[i], depth + 1);
        }
    }
};

// Movement function that uses octree for collision detection
inline void movementOctree(glm::vec3& pos, const glm::vec3& delta, const glm::vec3& halfSize, OctreeNode* octree) {
    glm::vec3 newPos = pos;

    // X axis movement
    newPos.x += delta.x;
    AABB playerBox = makeAABB(newPos, halfSize);

    std::vector<Collider*> potentialHits;
    octree->getPotentialCollisions(playerBox, potentialHits);

    for (const auto& collider : potentialHits) {
        if (collider->typeID == COLLIDER_WALL || collider->typeID == COLLIDER_DOOR) {
            if (playerBox.intersects(collider->box)) {
                newPos.x = pos.x; // revert X only
                break;
            }
        }
    }

    // Z axis movement
    newPos.z += delta.z;
    playerBox = makeAABB(newPos, halfSize);

    potentialHits.clear();
    octree->getPotentialCollisions(playerBox, potentialHits);

    for (const auto& collider : potentialHits) {
        if (collider->typeID == COLLIDER_WALL || collider->typeID == COLLIDER_DOOR) {
            if (playerBox.intersects(collider->box)) {
                newPos.z = pos.z; // revert Z only
                break;
            }
        }
    }

    pos = newPos;
}
