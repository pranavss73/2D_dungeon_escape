#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <string>

#include "Map.h"
#include "Player.h"
#include "Enemy.h"
#include "Bullet.h"
#include "QuadTree.h"

const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;
const int INITIAL_ENEMY_COUNT = 10;
const int MAX_ENEMIES_ON_SCREEN = 20;

// Safe spawning helper function that ensures an enemy doesn't spawn inside a wall or too close to the player
void spawnEnemy(std::vector<Enemy*>& enemies, const Map& gameMap, const sf::Vector2f& playerPos) {
    Enemy* enemy = new Enemy();
    
    int rng = std::rand() % 4;
    if (rng == 0) {
        enemy->load("assets/enemy.png", 9, 4);
    } else {
        enemy->loadAdvancedEnemy(rng);
    }

    bool validSpawn = false;
    sf::Vector2f spawnPos;
    
    // Attempt to safely spawn 50 times maximum per frame to avoid freezing
    int attempts = 0;
    while (!validSpawn && attempts < 50) {
        // Strict boundary generation to guarantee enemies never spawn inside the thick exterior walls
        float x = (std::rand() % (2127 - 187)) + 187.f;
        float y = (std::rand() % (1079 - 215)) + 215.f;
        
        enemy->setPosition(x, y);
        sf::FloatRect bounds = enemy->getGlobalBounds();

        bool hitWall = false;
        for (const auto& obs : gameMap.getObstacles()) {
            if (bounds.intersects(obs)) {
                hitWall = true;
                break;
            }
        }
        
        // Ensure they don't spawn right on top of the player
        float dx = x - playerPos.x;
        float dy = y - playerPos.y;
        float distToPlayer = std::sqrt(dx*dx + dy*dy);
        
        if (!hitWall && distToPlayer > 300.f) {
            validSpawn = true;
        }
        
        attempts++;
    }

    if (validSpawn) {
        enemies.push_back(enemy);
    } else {
        delete enemy; // failed to find safe spot, abort
    }
}

int main()
{
    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "SFML RPG with DSA QuadTree");
    window.setFramerateLimit(60);

    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    // 1. Initialize Map
    Map gameMap;
    // You may need to provide the full relative access path or handle exceptions
    if (!gameMap.load("assets/map.jpg")) {
        // Handle error gracefully if not found, but we proceed assuming it works
    }

    // View for scrolling
    sf::View view(sf::FloatRect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT));
    view.setCenter(gameMap.getSpawnPoint());

    // 2. Initialize Player
    Player player;
    player.loadTextures();
    player.setPosition(gameMap.getSpawnPoint().x, gameMap.getSpawnPoint().y);

    // 3. Initialize Enemies
    std::vector<Enemy*> enemies;
    for (int i = 0; i < INITIAL_ENEMY_COUNT; i++) {
        spawnEnemy(enemies, gameMap, player.getPosition());
    }

    // 4. Initialize Bullets
    std::vector<Bullet*> bullets;

    // Font for score
    sf::Font font;
    if (!font.loadFromFile("assets/arial.ttf")) {
        std::cout << "Font failed to load\n";
    }
    sf::Text scoreText;
    scoreText.setFont(font);
    scoreText.setCharacterSize(24);
    scoreText.setFillColor(sf::Color::White);
    
    // 5. Initialize Audio
    sf::Music bgMusic;
    if (bgMusic.openFromFile("assets/sound/music.wav")) {
        bgMusic.setLoop(true);
        bgMusic.setVolume(15.f); // Low volume
        bgMusic.play();
    } else {
        std::cerr << "ERROR: Failed to load assets/sound/music.wav! (Check if SFML version supports MP3 or if openal32.dll is missing)\n";
    }

    sf::SoundBuffer slashBuffer;
    sf::Sound slashSound;
    if (slashBuffer.loadFromFile("assets/sound/slash.wav")) {
        slashSound.setBuffer(slashBuffer);
        slashSound.setVolume(70.f); // slightly louder for impact
    } else {
        std::cerr << "ERROR: Failed to load assets/sound/slash.wav!\n";
    }

    sf::Music gameOverMusic;
    bool hasPlayedGameOver = false;
    if (gameOverMusic.openFromFile("assets/sound/game_over.wav")) {
        gameOverMusic.setVolume(25.f);
        gameOverMusic.setLoop(false);
    } else {
        std::cerr << "ERROR: Failed to load assets/sound/game_over.wav!\n";
    }

    int score = 0;

    // Clock for deltaTime
    sf::Clock dtClock;

    // Game state variables
    bool escaped = false;
    bool gameOver = false;
    float spawnTimer = 0.0f;
    float spawnInterval = 3.0f; // Seconds per spawn

    // QuadTree bounds - let's make it cover the map, approx 2816x1536
    sf::FloatRect mapBounds(0, 0, 3000, 2000);

    // Game Loop
    while (window.isOpen())
    {
        float dt = dtClock.restart().asSeconds();

        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::MouseButtonPressed) {
                if (event.mouseButton.button == sf::Mouse::Left && !escaped) {
                    // Shoot bullet - Disable per user request, kept for later
                    /*
                    sf::Vector2i pixelPos = sf::Mouse::getPosition(window);
                    sf::Vector2f worldPos = window.mapPixelToCoords(pixelPos);
                    
                    sf::Vector2f playerPos = player.getPosition();
                    sf::Vector2f direction = worldPos - playerPos;
                    
                    Bullet* b = new Bullet(playerPos, direction, 0.f);
                    bullets.push_back(b);
                    */
                }
            }

            // Handle input for attacks
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) {
                if (player.attack()) {
                    // Play slash sound effect
                    slashSound.play();
                    
                    sf::FloatRect attackArea = player.getAttackBounds();
                    for (Enemy* e : enemies) {
                        if (!e->isDead() && attackArea.intersects(e->getGlobalBounds())) {
                            if (e->takeDamage(1, player.getPosition())) {
                                score += 10;
                            }
                        }
                    }
                }
            }
        }

        if (escaped) {
            // Stop updating
            window.clear(sf::Color::Black);
            scoreText.setPosition(window.getView().getCenter().x - 100, window.getView().getCenter().y);
            scoreText.setString("You Escaped! Final Score: " + std::to_string(score));
            window.draw(scoreText);
            window.display();
            continue;
        }

        if (gameOver) {
            // Stop updating
            window.clear(sf::Color(50, 0, 0)); // Dark red tone
            scoreText.setPosition(window.getView().getCenter().x - 100, window.getView().getCenter().y);
            scoreText.setString("YOU DIED! Game Over.");
            window.draw(scoreText);
            window.display();
            continue;
        }

        // Endless Spawner Logic
        spawnTimer += dt;
        // Count active enemies
        int activeEnemies = 0;
        for(auto& e : enemies) if (!e->isDead()) activeEnemies++;
        
        if (spawnTimer >= spawnInterval) {
            spawnTimer = 0.0f;
            if (activeEnemies < MAX_ENEMIES_ON_SCREEN) {
                spawnEnemy(enemies, gameMap, player.getPosition());
            }
        }

        // Update Player Aim tracking Mouse and Move
        sf::Vector2i pixelPos = sf::Mouse::getPosition(window);
        sf::Vector2f worldPos = window.mapPixelToCoords(pixelPos);
        player.update(dt, worldPos, gameMap);

        // Update Camera View to follow player
        view.setCenter(player.getPosition());
        window.setView(view);

        // 1. Clear QuadTree
        QuadTree quadTree(0, mapBounds);

        // Update Enemies & Insert into QuadTree
        for (int i = 0; i < enemies.size(); ++i) {
            if (!enemies[i]->isDead()) {
                enemies[i]->update(dt, player.getPosition(), gameMap);

                // Insert into QuadTree (DSA component)
                QuadTreeData data;
                data.bounds = enemies[i]->getGlobalBounds();
                data.id = i;
                quadTree.insert(data);
            }
        }

        // Update Bullets & Check Collisions via QuadTree
        for (auto it = bullets.begin(); it != bullets.end(); ) {
            Bullet* b = *it;
            b->update(dt);

            // Destroy bullets if they hit a wall
            bool hitWall = false;
            for (const auto& obs : gameMap.getObstacles()) {
                if (b->getGlobalBounds().intersects(obs)) {
                    hitWall = true;
                    break;
                }
            }

            if (!b->isActive() || hitWall) {
                delete b;
                it = bullets.erase(it);
                continue;
            }

            // Retrieve potential enemies this bullet could collide with
            std::vector<QuadTreeData> returnObjects;
            quadTree.retrieve(returnObjects, b->getGlobalBounds());

            for (const auto& data : returnObjects) {
                int enemyIndex = data.id;
                if (!enemies[enemyIndex]->isDead() && b->getGlobalBounds().intersects(enemies[enemyIndex]->getGlobalBounds())) {
                    if (enemies[enemyIndex]->takeDamage(1, player.getPosition())) {
                        score += 10;
                    }
                    b->destroy();
                    break;
                }
            }

            ++it;
        }

        // Check Escape Condition
        if (player.getGlobalBounds().intersects(gameMap.getEscapeDoorBounds())) {
            escaped = true;
        }

        // --- TAKING DAMAGE --- //
        // From Enemies
        for (auto& enemy : enemies) {
            if ((enemy->getState() == ALIVE || enemy->getState() == ATTACKING) && enemy->getDamageBounds().intersects(player.getGlobalBounds())) {
                float dmg = (enemy->getType() == 1) ? 20.0f : 10.0f;
                player.takeDamage(dmg); // dynamic damage (respects iFrames)
            }
        }
        
        // From Spikes (indices 0 and 1 natively assigned to spikes in Map)
        const auto& props = gameMap.getProps();
        if (props.size() >= 2) {
            if (player.getGlobalBounds().intersects(props[0].getGlobalBounds())) player.takeDamage(10.0f);
            if (player.getGlobalBounds().intersects(props[1].getGlobalBounds())) player.takeDamage(10.0f);
        }

        if (player.getHealth() <= 0) {
            gameOver = true;
            // Transition music
            bgMusic.stop();
            if (!hasPlayedGameOver) {
                gameOverMusic.play();
                hasPlayedGameOver = true;
            }
        }

        // Render
        window.clear();
        gameMap.draw(window);

        // Sort enemies and player by Y axis so objects lower appear in front? Optional.
        for (auto& enemy : enemies) {
            enemy->draw(window);
        }

        player.draw(window);

        for (auto& bullet : bullets) {
            bullet->draw(window);
        }

        // Switch purely to UI Overlay Context
        window.setView(window.getDefaultView());
        
        // Draw Score
        scoreText.setPosition(10.f, 10.f);
        scoreText.setString("Score: " + std::to_string(score));
        window.draw(scoreText);

        // Draw Health Bar (10 segments)
        float currentHealth = player.getHealth();
        for (int i = 0; i < 10; ++i) {
            sf::RectangleShape chunk(sf::Vector2f(28.f, 15.f)); // 28px wide with 2px gaps visually
            chunk.setPosition(WINDOW_WIDTH / 2.0f - 150.f + (i * 30.f), WINDOW_HEIGHT - 50.f);
            
            // Draw background chunk with border
            chunk.setFillColor(sf::Color(40, 40, 40, 200));
            chunk.setOutlineThickness(1.f);
            chunk.setOutlineColor(sf::Color::Black);
            window.draw(chunk);

            // Draw red fill if applicable
            if (currentHealth > i * 10.f) {
                float fillAmt = std::min(currentHealth - (i * 10.f), 10.f);
                sf::RectangleShape fillPiece(sf::Vector2f((fillAmt / 10.f) * 28.f, 15.f));
                fillPiece.setPosition(WINDOW_WIDTH / 2.0f - 150.f + (i * 30.f), WINDOW_HEIGHT - 50.f);
                fillPiece.setFillColor(sf::Color(220, 20, 60)); // Crimson
                window.draw(fillPiece);
            }
        }

        // Draw Stamina Bar (5 segments)
        float currentStamina = player.getStamina();
        for (int i = 0; i < 5; ++i) {
            sf::RectangleShape chunk(sf::Vector2f(28.f, 10.f));
            chunk.setPosition(WINDOW_WIDTH / 2.0f - 150.f + (i * 30.f), WINDOW_HEIGHT - 30.f);
            
            // Draw background chunk with border
            chunk.setFillColor(sf::Color(40, 40, 40, 200));
            chunk.setOutlineThickness(1.f);
            chunk.setOutlineColor(sf::Color::Black);
            window.draw(chunk);

            // Draw blue fill if applicable
            if (currentStamina > i * 10.f) {
                float fillAmt = std::min(currentStamina - (i * 10.f), 10.f);
                sf::RectangleShape fillPiece(sf::Vector2f((fillAmt / 10.f) * 28.f, 10.f));
                fillPiece.setPosition(WINDOW_WIDTH / 2.0f - 150.f + (i * 30.f), WINDOW_HEIGHT - 30.f);
                fillPiece.setFillColor(sf::Color(30, 144, 255)); // Dodger Blue
                window.draw(fillPiece);
            }
        }

        window.display();
    }

    // Cleanup
    for (auto enemy : enemies) delete enemy;
    for (auto bullet : bullets) delete bullet;

    return 0;
}
