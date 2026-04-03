#include "QuadTree.h"

QuadTree::QuadTree(int level, const sf::FloatRect& bounds) : level(level), bounds(bounds) {
    for (int i = 0; i < 4; i++) {
        nodes[i] = nullptr;
    }
}

QuadTree::~QuadTree() {
    clear();
}

void QuadTree::clear() {
    objects.clear();
    for (int i = 0; i < 4; i++) {
        if (nodes[i] != nullptr) {
            nodes[i]->clear();
            delete nodes[i];
            nodes[i] = nullptr;
        }
    }
}

void QuadTree::split() {
    float subWidth = bounds.width / 2.0f;
    float subHeight = bounds.height / 2.0f;
    float x = bounds.left;
    float y = bounds.top;

    nodes[0] = new QuadTree(level + 1, sf::FloatRect(x + subWidth, y, subWidth, subHeight));
    nodes[1] = new QuadTree(level + 1, sf::FloatRect(x, y, subWidth, subHeight));
    nodes[2] = new QuadTree(level + 1, sf::FloatRect(x, y + subHeight, subWidth, subHeight));
    nodes[3] = new QuadTree(level + 1, sf::FloatRect(x + subWidth, y + subHeight, subWidth, subHeight));
}

int QuadTree::getIndex(const sf::FloatRect& pRect) {
    int index = -1;
    double verticalMidpoint = bounds.left + (bounds.width / 2.0);
    double horizontalMidpoint = bounds.top + (bounds.height / 2.0);

    // Object can completely fit within the top quadrants
    bool topQuadrant = (pRect.top < horizontalMidpoint && pRect.top + pRect.height < horizontalMidpoint);
    // Object can completely fit within the bottom quadrants
    bool bottomQuadrant = (pRect.top > horizontalMidpoint);

    // Object can completely fit within the left quadrants
    if (pRect.left < verticalMidpoint && pRect.left + pRect.width < verticalMidpoint) {
        if (topQuadrant) {
            index = 1;
        } else if (bottomQuadrant) {
            index = 2;
        }
    }
    // Object can completely fit within the right quadrants
    else if (pRect.left > verticalMidpoint) {
        if (topQuadrant) {
            index = 0;
        } else if (bottomQuadrant) {
            index = 3;
        }
    }

    return index;
}

void QuadTree::insert(const QuadTreeData& data) {
    if (nodes[0] != nullptr) {
        int index = getIndex(data.bounds);
        if (index != -1) {
            nodes[index]->insert(data);
            return;
        }
    }

    objects.push_back(data);

    if (objects.size() > MAX_OBJECTS && level < MAX_LEVELS) {
        if (nodes[0] == nullptr) {
            split();
        }

        int i = 0;
        while (i < objects.size()) {
            int index = getIndex(objects[i].bounds);
            if (index != -1) {
                QuadTreeData movingData = objects[i];
                objects.erase(objects.begin() + i);
                nodes[index]->insert(movingData);
            } else {
                i++;
            }
        }
    }
}

std::vector<QuadTreeData> QuadTree::retrieve(std::vector<QuadTreeData>& returnObjects, const sf::FloatRect& pRect) {
    int index = getIndex(pRect);
    if (index != -1 && nodes[0] != nullptr) {
        nodes[index]->retrieve(returnObjects, pRect);
    } else if (nodes[0] != nullptr) {
        // If it doesn't fit in a specific quadrant perfectly, it spans multiple, 
        // so we retrieve from all children it intersects.
        for (int i = 0; i < 4; i++) {
            if (pRect.intersects(nodes[i]->bounds)) {
               nodes[i]->retrieve(returnObjects, pRect);
            }
        }
    }

    returnObjects.insert(returnObjects.end(), objects.begin(), objects.end());
    return returnObjects;
}
