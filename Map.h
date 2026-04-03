#ifndef MAP_H
#define MAP_H

#include <SFML/Graphics.hpp>
#include <vector>

class Map {
public:
    Map();
    ~Map();

    bool load(const std::string& texturePath);
    void draw(sf::RenderWindow& window);

    // Getters for game logic
    sf::Vector2f getSpawnPoint() const;
    sf::FloatRect getEscapeDoorBounds() const;
    sf::FloatRect getBounds() const;

    const std::vector<sf::FloatRect>& getObstacles() const;
    const std::vector<sf::Sprite>& getProps() const;

private:
    sf::Texture mapTexture;
    sf::Sprite mapSprite;
    
    sf::Vector2f spawnPoint;
    sf::FloatRect escapeDoorBounds;
    std::vector<sf::FloatRect> obstacles;

    sf::Texture spikeTexture;
    sf::Texture chestTexture;
    std::vector<sf::Sprite> props;
};

#endif // MAP_H
