#include "Map.h"
#include <iostream>

Map::Map() {
    // Default values
    spawnPoint = sf::Vector2f(400.f, 300.f); 
    escapeDoorBounds = sf::FloatRect(2700.f, 1400.f, 100.f, 100.f);
    leverPulled = false;
    lever2Pulled = false;
    keyCollected = false;
    doorOpen = false;
    doorObstacleRect = sf::FloatRect(1871.f, 752.f, 124.f, 96.f);
    doorAreaBounds   = sf::FloatRect(1871.f, 752.f, 124.f, 96.f);
    chestOpened = false;
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
    obstacles.push_back(sf::FloatRect(647, 387, 337, 209));
    obstacles.push_back(sf::FloatRect(1214, 535, 287, 224));
    obstacles.push_back(sf::FloatRect(1371, 388, 253, 268));
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
    
    // Load Lever & Block
    leverPulled = false;
    if (leverTexture.loadFromFile("assets/objects/lever.png")) {
        leverSprite.setTexture(leverTexture);
        leverSprite.setOrigin(leverTexture.getSize().x / 2.f, leverTexture.getSize().y / 2.f);
        leverSprite.setScale(25.f / leverTexture.getSize().x, 106.f / leverTexture.getSize().y);
        leverSprite.setPosition(1746.f + 12.5f, 254.f + 53.f); // Centered relative to explicit position
    }

    if (blockTexture.loadFromFile("assets/objects/block.png")) {
        blockSprite.setTexture(blockTexture);
        blockSprite.setPosition(1370.f, 832.f);
        blockSprite.setScale(24.f / blockTexture.getSize().x, 90.f / blockTexture.getSize().y);
    }

    // Add explicit invisible blockade mapped by user bounds
    invisibleBlockRect = sf::FloatRect(1372.f, 590.f, 867.f, 565.f);
    obstacles.push_back(invisibleBlockRect);

    // Load Lever 2 & its blocks
    lever2Pulled = false;
    if (lever2Texture.loadFromFile("assets/objects/lever.png")) {
        lever2Sprite.setTexture(lever2Texture);
        lever2Sprite.setOrigin(lever2Texture.getSize().x / 2.f, lever2Texture.getSize().y / 2.f);
        lever2Sprite.setScale(25.f / lever2Texture.getSize().x, 106.f / lever2Texture.getSize().y);
        lever2Sprite.setPosition(2076.f + 12.5f, 940.f + 53.f);
    }

    if (block2Texture.loadFromFile("assets/objects/block.png")) {
        block2Sprite.setTexture(block2Texture);
        block2Sprite.setPosition(957.f, 831.f);
        block2Sprite.setScale(24.f / block2Texture.getSize().x, 90.f / block2Texture.getSize().y);
    }

    if (blockHzTexture.loadFromFile("assets/objects/block_hz.png")) {
        blockHzSprite.setTexture(blockHzTexture);
        blockHzSprite.setPosition(379.f, 535.f);
        blockHzSprite.setScale(84.f / blockHzTexture.getSize().x, 18.f / blockHzTexture.getSize().y);
    }

    // Invisible blockade for lever 2
    invisibleBlock2Rect = sf::FloatRect(162.f, 535.f, 819.f, 592.f);
    obstacles.push_back(invisibleBlock2Rect);
    
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

    // Door area is blocked until door is opened
    obstacles.push_back(doorObstacleRect);

    // Load key sprite
    keyCollected = false;
    keyRect = sf::FloatRect(391.f, 996.f, 24.f, 11.f);
    if (keyTexture.loadFromFile("assets/objects/key.png")) {
        keySprite.setTexture(keyTexture);
        keySprite.setPosition(keyRect.left, keyRect.top);
        keySprite.setScale(keyRect.width / keyTexture.getSize().x,
                           keyRect.height / keyTexture.getSize().y);
    }

    // Preload map2 texture (used when door opens)
    doorOpen = false;
    map2Texture.loadFromFile("assets/map2.jpg");
    
    chestOpened = false;

    return true;
}

void Map::draw(sf::RenderWindow& window) {
    window.draw(mapSprite);

    // Draw objects
    for (size_t i = 0; i < props.size(); ++i) {
        // Skip drawing the chest if it has been opened
        if (i == 2 && chestOpened) continue;

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

    window.draw(leverSprite);
    if (!leverPulled) {
        window.draw(blockSprite);
    }

    window.draw(lever2Sprite);
    if (!lever2Pulled) {
        window.draw(block2Sprite);
        window.draw(blockHzSprite);
    }

    // Draw key if not yet collected
    if (!keyCollected) {
        window.draw(keySprite);
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
        box.setFillColor(sf::Color(255, 0, 0, 0)); // Make it strongly visible
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

bool Map::isLeverPulled() const {
    return leverPulled;
}

void Map::triggerLever() {
    if (leverPulled) return;
    leverPulled = true;
    
    // Flip Lever visually
    leverSprite.setScale(leverSprite.getScale().x, -std::abs(leverSprite.getScale().y));

    // Dissolve the invisible blockade
    auto it = obstacles.begin();
    while (it != obstacles.end()) {
        if (it->left == invisibleBlockRect.left && it->top == invisibleBlockRect.top) {
            it = obstacles.erase(it);
        } else {
            ++it;
        }
    }
}

sf::FloatRect Map::getLeverBounds() const {
    return leverSprite.getGlobalBounds();
}

bool Map::isLever2Pulled() const {
    return lever2Pulled;
}

void Map::triggerLever2() {
    if (lever2Pulled) return;
    lever2Pulled = true;

    // Flip lever 2 visually
    lever2Sprite.setScale(lever2Sprite.getScale().x, -std::abs(lever2Sprite.getScale().y));

    // Remove the invisible blockade for lever 2
    auto it = obstacles.begin();
    while (it != obstacles.end()) {
        if (it->left == invisibleBlock2Rect.left && it->top == invisibleBlock2Rect.top) {
            it = obstacles.erase(it);
        } else {
            ++it;
        }
    }
}

sf::FloatRect Map::getLever2Bounds() const {
    return lever2Sprite.getGlobalBounds();
}

// --- Key ---
bool Map::isKeyCollected() const {
    return keyCollected;
}

sf::FloatRect Map::getKeyBounds() const {
    return keySprite.getGlobalBounds();
}

void Map::collectKey() {
    keyCollected = true;
}

// --- Door ---
void Map::openDoor() {
    if (doorOpen) return;
    doorOpen = true;

    // Swap background to map2 (door open version)
    mapSprite.setTexture(map2Texture);

    // Remove the door obstacle so player can walk through
    auto it = obstacles.begin();
    while (it != obstacles.end()) {
        if (it->left == doorObstacleRect.left && it->top == doorObstacleRect.top) {
            it = obstacles.erase(it);
        } else {
            ++it;
        }
    }
}

bool Map::isDoorOpen() const {
    return doorOpen;
}

sf::FloatRect Map::getDoorAreaBounds() const {
    return doorAreaBounds;
}

// --- Chest ---
bool Map::isChestOpened() const {
    return chestOpened;
}

void Map::openChest() {
    chestOpened = true;
}

sf::FloatRect Map::getChestBounds() const {
    if (props.size() > 2) {
        // Expand the bounds slightly to make it easier to intersect with
        sf::FloatRect b = props[2].getGlobalBounds();
        return sf::FloatRect(b.left - 20.f, b.top - 20.f, b.width + 40.f, b.height + 40.f);
    }
    return sf::FloatRect(1920, 235, 76, 66); // fallback region provided by user
}

void Map::reset() {
    // Reset lever 1
    if (leverPulled) {
        leverPulled = false;
        leverSprite.setScale(25.f / leverTexture.getSize().x, 106.f / leverTexture.getSize().y);
        obstacles.push_back(invisibleBlockRect);
    }

    // Reset lever 2
    if (lever2Pulled) {
        lever2Pulled = false;
        lever2Sprite.setScale(25.f / lever2Texture.getSize().x, 106.f / lever2Texture.getSize().y);
        obstacles.push_back(invisibleBlock2Rect);
    }

    // Reset key
    keyCollected = false;

    // Reset door
    if (doorOpen) {
        doorOpen = false;
        mapSprite.setTexture(mapTexture); // swap back to original map
        obstacles.push_back(doorObstacleRect);
    }

    // Reset chest
    chestOpened = false;
}
