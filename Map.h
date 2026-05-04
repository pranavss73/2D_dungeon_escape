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

    // Lever 1 (right side)
    bool isLeverPulled() const;
    void triggerLever();
    sf::FloatRect getLeverBounds() const;
    void reset();

    // Lever 2 (left side)
    bool isLever2Pulled() const;
    void triggerLever2();
    sf::FloatRect getLever2Bounds() const;

    // Key pickup
    bool isKeyCollected() const;
    sf::FloatRect getKeyBounds() const;
    void collectKey();

    // Door / map swap
    void openDoor();  // swap to map2, remove door obstacle
    bool isDoorOpen() const;
    sf::FloatRect getDoorAreaBounds() const; // win zone (1871,752,124,96)

    // Chest interaction
    bool isChestOpened() const;
    void openChest();
    sf::FloatRect getChestBounds() const;

private:
    sf::Texture mapTexture;
    sf::Sprite mapSprite;
    
    sf::Vector2f spawnPoint;
    sf::FloatRect escapeDoorBounds;
    std::vector<sf::FloatRect> obstacles;

    sf::Texture spikeTexture;
    sf::Texture chestTexture;
    std::vector<sf::Sprite> props;

    // Lever 1 vars
    bool leverPulled;
    sf::Texture leverTexture;
    sf::Texture blockTexture;
    sf::Sprite leverSprite;
    sf::Sprite blockSprite;
    sf::FloatRect invisibleBlockRect;

    // Lever 2 vars
    bool lever2Pulled;
    sf::Texture lever2Texture;
    sf::Texture block2Texture;
    sf::Texture blockHzTexture;
    sf::Sprite lever2Sprite;
    sf::Sprite block2Sprite;
    sf::Sprite blockHzSprite;
    sf::FloatRect invisibleBlock2Rect;

    // Key vars
    bool keyCollected;
    sf::Texture keyTexture;
    sf::Sprite keySprite;
    sf::FloatRect keyRect;

    // Door vars
    bool doorOpen;
    sf::Texture map2Texture;
    sf::FloatRect doorObstacleRect; // the blocked-off exit area
    sf::FloatRect doorAreaBounds;   // the win zone

    // Chest vars
    bool chestOpened;
};

#endif // MAP_H
