#ifndef QUADTREE_H
#define QUADTREE_H

#include <SFML/Graphics.hpp>
#include <vector>

// Forward declaration if needed, or we just store sf::FloatRect and an ID/pointer
// For simplicity, we'll store sf::FloatRect and a pointer to an entity abstract class or just a generic void* / int id.
// We'll just store the sf::FloatRect bounds and an integer ID representing the enemy/object index.

struct QuadTreeData {
    sf::FloatRect bounds;
    int id;
};

class QuadTree {
public:
    QuadTree(int level, const sf::FloatRect& bounds);
    ~QuadTree();

    void clear();
    void split();
    int getIndex(const sf::FloatRect& pRect);
    void insert(const QuadTreeData& data);
    std::vector<QuadTreeData> retrieve(std::vector<QuadTreeData>& returnObjects, const sf::FloatRect& pRect);

private:
    int MAX_OBJECTS = 5;
    int MAX_LEVELS = 5;

    int level;
    std::vector<QuadTreeData> objects;
    sf::FloatRect bounds;
    QuadTree* nodes[4];
};

#endif // QUADTREE_H
