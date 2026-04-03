#include "Map.h"
#include <iostream>

Map::Map() {
    // Default values
    spawnPoint = sf::Vector2f(400.f, 300.f); 
    escapeDoorBounds = sf::FloatRect(2700.f, 1400.f, 100.f, 100.f); // Assuming escape is bottom right
}

Map::~Map() {}

bool Map::load(const std::string& texturePath) {
    if (!mapTexture.loadFromFile(texturePath)) {
        std::cout << "Failed to load map texture: " << texturePath << std::endl;
        return false;
    }
    mapSprite.setTexture(mapTexture);

    // Exact bounds generated from map visual analysis
    obstacles.clear();
    obstacles.push_back(sf::FloatRect(0, 0, 2239, 215)); //up boundry
    obstacles.push_back(sf::FloatRect(0, 0, 187, 1220));  //left boundry
    obstacles.push_back(sf::FloatRect(2127, 0, 120, 1220)); //right boundry
    obstacles.push_back(sf::FloatRect(0, 1079, 2239, 138)); //down boundry
    obstacles.push_back(sf::FloatRect(0, 529, 366, 229));
    obstacles.push_back(sf::FloatRect(465, 535, 671, 235));
    obstacles.push_back(sf::FloatRect(616, 613, 365, 255));
    obstacles.push_back(sf::FloatRect(647, 382, 337, 209));
    obstacles.push_back(sf::FloatRect(1214, 535, 287, 224));
    obstacles.push_back(sf::FloatRect(1371, 377, 253, 268));
    obstacles.push_back(sf::FloatRect(1369, 590, 373, 284));
    obstacles.push_back(sf::FloatRect(1580, 457, 620, 382));
    obstacles.push_back(sf::FloatRect(647, 56, 336, 273));
    obstacles.push_back(sf::FloatRect(1372, 56, 257, 273));
    obstacles.push_back(sf::FloatRect(613, 924, 368, 228));
    obstacles.push_back(sf::FloatRect(1370, 923, 372, 223));
    
    // Load and construct props
    if (spikeTexture.loadFromFile("assets/objects/spikes.png") && 
        chestTexture.loadFromFile("assets/objects/chest.png")) {
        
        props.clear();
        // Spike 1 (1847, 236, width 50)
        sf::Sprite s1(spikeTexture);
        s1.setPosition(1847, 226);
        s1.setScale(50.f / spikeTexture.getSize().x, 70.f / spikeTexture.getSize().x);
        props.push_back(s1);

        // Spike 2 (1847, 320, width 50)
        sf::Sprite s2(spikeTexture);
        s2.setPosition(1847, 310);
        s2.setScale(50.f / spikeTexture.getSize().x, 70.f / spikeTexture.getSize().x);
        props.push_back(s2);

        // Chest (1920, 243, width 75)
        sf::Sprite c(chestTexture);
        c.setPosition(1920, 243);
        c.setScale(65.f / chestTexture.getSize().x, 65.f / chestTexture.getSize().x);
        props.push_back(c);
    }
    
    // obstacles.push_back(sf::FloatRect(1728, 1216, 384, 64));
    // obstacles.push_back(sf::FloatRect(128, 1280, 128, 256));
    // obstacles.push_back(sf::FloatRect(768, 1280, 64, 192));
    // obstacles.push_back(sf::FloatRect(1152, 1280, 64, 192));
    // obstacles.push_back(sf::FloatRect(1728, 1280, 64, 192));
    // obstacles.push_back(sf::FloatRect(2048, 1280, 128, 192));
    // obstacles.push_back(sf::FloatRect(2496, 1280, 128, 256));
    // obstacles.push_back(sf::FloatRect(320, 1344, 64, 192));
    // obstacles.push_back(sf::FloatRect(2432, 1344, 64, 192));
    // obstacles.push_back(sf::FloatRect(256, 1408, 64, 128));
    // obstacles.push_back(sf::FloatRect(384, 1408, 384, 64));
    // obstacles.push_back(sf::FloatRect(1216, 1408, 512, 64));
    // obstacles.push_back(sf::FloatRect(2176, 1408, 256, 128));
    // obstacles.push_back(sf::FloatRect(2624, 1408, 64, 128));
    // obstacles.push_back(sf::FloatRect(2752, 1408, 64, 128));
    // obstacles.push_back(sf::FloatRect(384, 1472, 256, 64));
    // obstacles.push_back(sf::FloatRect(2112, 1472, 64, 64));
    // obstacles.push_back(sf::FloatRect(2688, 1472, 64, 64));

    return true;
}

void Map::draw(sf::RenderWindow& window) {
    window.draw(mapSprite);

    // Draw objects
    for (size_t i = 0; i < props.size(); ++i) {
        // Draw shadow beneath the chest (which is the 3rd prop inserted)
        if (i == 2) {
            sf::FloatRect bounds = props[i].getGlobalBounds();
            sf::CircleShape shadow(bounds.width * 0.45f); 
            shadow.setOrigin(shadow.getRadius(), shadow.getRadius());
            shadow.setScale(1.f, 0.4f);
            shadow.setFillColor(sf::Color(0, 0, 0, 140));
            shadow.setPosition(bounds.left + bounds.width * 0.50f, bounds.top + bounds.height * 0.85f);
            window.draw(shadow);
        }
        
        window.draw(props[i]);
    }
    
    // For debugging the escape door, you could draw it
    // sf::RectangleShape doorShape(sf::Vector2f(escapeDoorBounds.width, escapeDoorBounds.height));
    // doorShape.setPosition(escapeDoorBounds.left, escapeDoorBounds.top);
    // doorShape.setFillColor(sf::Color(0, 255, 0, 100)); // Semi-transparent green
    // window.draw(doorShape);
    
    // Optional: Draw obstacles for testing
    for (const auto& obs : obstacles) {
        sf::RectangleShape box(sf::Vector2f(obs.width, obs.height));
        box.setPosition(obs.left, obs.top);
        box.setFillColor(sf::Color(255, 0, 0, 0 )); // Make it strongly visible
        window.draw(box);
    }
}

sf::Vector2f Map::getSpawnPoint() const {
    return spawnPoint;
}

sf::FloatRect Map::getEscapeDoorBounds() const {
    return escapeDoorBounds;
}

sf::FloatRect Map::getBounds() const {
    return mapSprite.getGlobalBounds();
}

const std::vector<sf::FloatRect>& Map::getObstacles() const {
    return obstacles;
}

const std::vector<sf::Sprite>& Map::getProps() const {
    return props;
}
