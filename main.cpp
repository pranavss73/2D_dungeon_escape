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

// Draw a themed dungeon-style button with hover highlight
void drawButton(sf::RenderWindow& window, const sf::Font& font,
                const std::string& label, float x, float y, float w, float h,
                const sf::Vector2f& mousePos) {
    sf::RectangleShape btn(sf::Vector2f(w, h));
    btn.setPosition(x, y);

    bool hovered = btn.getGlobalBounds().contains(mousePos);

    if (hovered) {
        btn.setFillColor(sf::Color(80, 70, 55, 235));
        btn.setOutlineColor(sf::Color(255, 215, 0));
    } else {
        btn.setFillColor(sf::Color(45, 40, 35, 220));
        btn.setOutlineColor(sf::Color(160, 130, 40));
    }
    btn.setOutlineThickness(2.f);

    sf::Text text;
    text.setFont(font);
    text.setString(label);
    text.setCharacterSize(24);
    text.setFillColor(hovered ? sf::Color(255, 225, 80) : sf::Color(210, 180, 60));

    sf::FloatRect tb = text.getLocalBounds();
    text.setOrigin(tb.left + tb.width / 2.f, tb.top + tb.height / 2.f);
    text.setPosition(x + w / 2.f, y + h / 2.f);

    window.draw(btn);
    window.draw(text);
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
        bgMusic.setVolume(25.f); // Low volume
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

    // Game state
    enum GameState { PLAYING, PAUSED, GAME_OVER_STATE, ESCAPED_STATE };
    GameState gameState = PLAYING;
    float spawnTimer = 0.0f;
    float spawnInterval = 3.0f;

    // QuadTree bounds - let's make it cover the map, approx 2816x1536
    sf::FloatRect mapBounds(0, 0, 3000, 2000);

    // Button layout constants
    const float BTN_W = 240.f;
    const float BTN_H = 55.f;
    const float BTN_X = WINDOW_WIDTH / 2.f - BTN_W / 2.f;

    // Lambda to fully reset game state for retry / play-again
    auto resetGame = [&]() {
        for (auto enemy : enemies) delete enemy;
        enemies.clear();
        for (auto bullet : bullets) delete bullet;
        bullets.clear();

        player.reset(gameMap.getSpawnPoint().x, gameMap.getSpawnPoint().y);
        gameMap.reset();

        score = 0;
        spawnTimer = 0.0f;
        spawnInterval = 3.0f;
        hasPlayedGameOver = false;

        for (int i = 0; i < INITIAL_ENEMY_COUNT; i++) {
            spawnEnemy(enemies, gameMap, player.getPosition());
        }

        bgMusic.play();
        gameOverMusic.stop();

        gameState = PLAYING;
    };

    // Game Loop
    while (window.isOpen())
    {
        float dt = dtClock.restart().asSeconds();

        // Mouse position for button hover detection (screen-space)
        sf::Vector2i mousePixel = sf::Mouse::getPosition(window);
        sf::Vector2f mouseScreen(static_cast<float>(mousePixel.x), static_cast<float>(mousePixel.y));

        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();

            // Toggle pause with Escape key
            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
                if (gameState == PLAYING)  gameState = PAUSED;
                else if (gameState == PAUSED) gameState = PLAYING;
            }

            // Mouse click — handle menu/overlay buttons
            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                float mx = static_cast<float>(event.mouseButton.x);
                float my = static_cast<float>(event.mouseButton.y);

                if (gameState == PAUSED) {
                    if (mx >= BTN_X && mx <= BTN_X + BTN_W && my >= 300.f && my <= 355.f) gameState = PLAYING;   // Resume
                    if (mx >= BTN_X && mx <= BTN_X + BTN_W && my >= 370.f && my <= 425.f) resetGame();           // Retry
                    if (mx >= BTN_X && mx <= BTN_X + BTN_W && my >= 440.f && my <= 495.f) window.close();        // Quit
                }
                else if (gameState == GAME_OVER_STATE) {
                    if (mx >= BTN_X && mx <= BTN_X + BTN_W && my >= 380.f && my <= 435.f) resetGame();           // Retry
                    if (mx >= BTN_X && mx <= BTN_X + BTN_W && my >= 450.f && my <= 505.f) window.close();        // Quit
                }
                else if (gameState == ESCAPED_STATE) {
                    if (mx >= BTN_X && mx <= BTN_X + BTN_W && my >= 380.f && my <= 435.f) resetGame();           // Play Again
                    if (mx >= BTN_X && mx <= BTN_X + BTN_W && my >= 450.f && my <= 505.f) window.close();        // Quit
                }
            }

            // Melee attack — only while actively playing
            if (gameState == PLAYING && sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) {
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

        // =================== VICTORY SCREEN ===================
        if (gameState == ESCAPED_STATE) {
            window.clear(sf::Color(10, 10, 15));
            window.setView(window.getDefaultView());

            // Subtle dark-green overlay
            sf::RectangleShape overlay(sf::Vector2f(WINDOW_WIDTH, WINDOW_HEIGHT));
            overlay.setFillColor(sf::Color(5, 20, 5, 180));
            window.draw(overlay);

            // Title
            sf::Text title;
            title.setFont(font);
            title.setString("YOU ESCAPED!");
            title.setCharacterSize(56);
            title.setFillColor(sf::Color(220, 190, 60));
            title.setStyle(sf::Text::Bold);
            sf::FloatRect tb1 = title.getLocalBounds();
            title.setOrigin(tb1.left + tb1.width / 2.f, tb1.top + tb1.height / 2.f);
            title.setPosition(WINDOW_WIDTH / 2.f, 180.f);
            window.draw(title);

            // Decorative line
            sf::RectangleShape dLine(sf::Vector2f(300.f, 2.f));
            dLine.setFillColor(sf::Color(180, 150, 50, 180));
            dLine.setPosition(WINDOW_WIDTH / 2.f - 150.f, 225.f);
            window.draw(dLine);

            // Subtitle
            sf::Text sub;
            sub.setFont(font);
            sub.setString("You made it out of the dungeon alive!");
            sub.setCharacterSize(18);
            sub.setFillColor(sf::Color(160, 180, 140));
            sf::FloatRect sb1 = sub.getLocalBounds();
            sub.setOrigin(sb1.left + sb1.width / 2.f, sb1.top + sb1.height / 2.f);
            sub.setPosition(WINDOW_WIDTH / 2.f, 260.f);
            window.draw(sub);

            // Score
            sf::Text sc;
            sc.setFont(font);
            sc.setString("Final Score: " + std::to_string(score));
            sc.setCharacterSize(28);
            sc.setFillColor(sf::Color(200, 200, 200));
            sf::FloatRect scb1 = sc.getLocalBounds();
            sc.setOrigin(scb1.left + scb1.width / 2.f, scb1.top + scb1.height / 2.f);
            sc.setPosition(WINDOW_WIDTH / 2.f, 320.f);
            window.draw(sc);

            // Buttons
            drawButton(window, font, "Play Again", BTN_X, 380.f, BTN_W, BTN_H, mouseScreen);
            drawButton(window, font, "Quit",       BTN_X, 450.f, BTN_W, BTN_H, mouseScreen);

            window.display();
            continue;
        }

        // =================== GAME OVER SCREEN ===================
        if (gameState == GAME_OVER_STATE) {
            window.clear(sf::Color(20, 5, 5));
            window.setView(window.getDefaultView());

            // Dark-red overlay
            sf::RectangleShape overlay(sf::Vector2f(WINDOW_WIDTH, WINDOW_HEIGHT));
            overlay.setFillColor(sf::Color(40, 0, 0, 180));
            window.draw(overlay);

            // Title
            sf::Text title;
            title.setFont(font);
            title.setString("YOU DIED");
            title.setCharacterSize(64);
            title.setFillColor(sf::Color(200, 30, 30));
            title.setStyle(sf::Text::Bold);
            sf::FloatRect tb2 = title.getLocalBounds();
            title.setOrigin(tb2.left + tb2.width / 2.f, tb2.top + tb2.height / 2.f);
            title.setPosition(WINDOW_WIDTH / 2.f, 180.f);
            window.draw(title);

            // Subtitle
            sf::Text sub;
            sub.setFont(font);
            sub.setString("The dungeon claims another soul...");
            sub.setCharacterSize(18);
            sub.setFillColor(sf::Color(150, 100, 100));
            sf::FloatRect sb2 = sub.getLocalBounds();
            sub.setOrigin(sb2.left + sb2.width / 2.f, sb2.top + sb2.height / 2.f);
            sub.setPosition(WINDOW_WIDTH / 2.f, 245.f);
            window.draw(sub);

            // Decorative line
            sf::RectangleShape dLine(sf::Vector2f(300.f, 2.f));
            dLine.setFillColor(sf::Color(150, 30, 30, 180));
            dLine.setPosition(WINDOW_WIDTH / 2.f - 150.f, 275.f);
            window.draw(dLine);

            // Score
            sf::Text sc;
            sc.setFont(font);
            sc.setString("Score: " + std::to_string(score));
            sc.setCharacterSize(24);
            sc.setFillColor(sf::Color(180, 150, 150));
            sf::FloatRect scb2 = sc.getLocalBounds();
            sc.setOrigin(scb2.left + scb2.width / 2.f, scb2.top + scb2.height / 2.f);
            sc.setPosition(WINDOW_WIDTH / 2.f, 320.f);
            window.draw(sc);

            // Buttons
            drawButton(window, font, "Retry", BTN_X, 380.f, BTN_W, BTN_H, mouseScreen);
            drawButton(window, font, "Quit",  BTN_X, 450.f, BTN_W, BTN_H, mouseScreen);

            window.display();
            continue;
        }

        // =================== GAMEPLAY UPDATE (only while PLAYING) ===================
        if (gameState == PLAYING) {

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

        // KEY PICKUP: walk over key to auto-collect
        if (!gameMap.isKeyCollected() &&
            player.getGlobalBounds().intersects(gameMap.getKeyBounds())) {
            gameMap.collectKey();
        }

        // WIN ZONE: entering the door area after door is opened
        if (gameMap.isDoorOpen() &&
            player.getGlobalBounds().intersects(gameMap.getDoorAreaBounds())) {
            gameState = ESCAPED_STATE;
            bgMusic.stop();
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
            gameState = GAME_OVER_STATE;
            // Transition music
            bgMusic.stop();
            if (!hasPlayedGameOver) {
                gameOverMusic.play();
                hasPlayedGameOver = true;
            }
        }

        } // end if (gameState == PLAYING)

        // Always update camera for rendering (needed for PAUSED state too)
        view.setCenter(player.getPosition());
        window.setView(view);

        // =================== RENDER GAME WORLD ===================
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

        // Draw Interaction UI natively attached to World Space
        sf::Vector2f playerPosWorld = player.getPosition();
        sf::FloatRect leverBounds = gameMap.getLeverBounds();
        sf::Vector2f leverCenter(leverBounds.left + leverBounds.width/2, leverBounds.top + leverBounds.height/2);
        
        float distToLever = std::sqrt(std::pow(playerPosWorld.x - leverCenter.x, 2) + std::pow(playerPosWorld.y - leverCenter.y, 2));

        if (distToLever < 40.f && !gameMap.isLeverPulled()) {
            sf::Text interactText;
            interactText.setFont(font);
            interactText.setCharacterSize(18);
            interactText.setFillColor(sf::Color::Yellow);
            interactText.setString("Press 'E' to open");
            interactText.setPosition(playerPosWorld.x - 60.f, playerPosWorld.y - 80.f);
            
            // Generate tiny background tag for readability
            sf::FloatRect textBounds = interactText.getGlobalBounds();
            sf::RectangleShape theTag(sf::Vector2f(textBounds.width + 10.f, textBounds.height + 10.f));
            theTag.setPosition(interactText.getPosition().x - 5.f, interactText.getPosition().y - 5.f);
            theTag.setFillColor(sf::Color(0, 0, 0, 0));
            
            window.draw(theTag);
            window.draw(interactText);
            
            if (gameState == PLAYING && sf::Keyboard::isKeyPressed(sf::Keyboard::E)) {
                gameMap.triggerLever();
            }
        }

        // Lever 2 proximity interaction
        sf::FloatRect lever2Bounds = gameMap.getLever2Bounds();
        sf::Vector2f lever2Center(lever2Bounds.left + lever2Bounds.width/2, lever2Bounds.top + lever2Bounds.height/2);
        float distToLever2 = std::sqrt(std::pow(playerPosWorld.x - lever2Center.x, 2) + std::pow(playerPosWorld.y - lever2Center.y, 2));

        if (distToLever2 < 40.f && !gameMap.isLever2Pulled()) {
            sf::Text interactText2;
            interactText2.setFont(font);
            interactText2.setCharacterSize(18);
            interactText2.setFillColor(sf::Color::Yellow);
            interactText2.setString("Press 'E' to open");
            interactText2.setPosition(playerPosWorld.x - 60.f, playerPosWorld.y - 80.f);

            sf::FloatRect textBounds2 = interactText2.getGlobalBounds();
            sf::RectangleShape theTag2(sf::Vector2f(textBounds2.width + 10.f, textBounds2.height + 10.f));
            theTag2.setPosition(interactText2.getPosition().x - 5.f, interactText2.getPosition().y - 5.f);
            theTag2.setFillColor(sf::Color(0, 0, 0, 0));

            window.draw(theTag2);
            window.draw(interactText2);

            if (gameState == PLAYING && sf::Keyboard::isKeyPressed(sf::Keyboard::E)) {
                gameMap.triggerLever2();
            }
        }

        // --- DOOR CIRCLE INTERACTION (center 1931,749 r=150) ---
        {
            sf::Vector2f doorCenter(1931.f, 749.f);
            float distToDoor = std::sqrt(
                std::pow(playerPosWorld.x - doorCenter.x, 2) +
                std::pow(playerPosWorld.y - doorCenter.y, 2));

            if (distToDoor < 150.f && !gameMap.isDoorOpen()) {
                std::string doorMsg = gameMap.isKeyCollected()
                    ? "Press 'E' to Escape"
                    : "Find Key to Open";
                sf::Color msgColor = gameMap.isKeyCollected()
                    ? sf::Color(50, 255, 100)   // green when ready
                    : sf::Color(255, 200, 50);   // amber when locked

                sf::Text doorText;
                doorText.setFont(font);
                doorText.setCharacterSize(18);
                doorText.setFillColor(msgColor);
                doorText.setString(doorMsg);
                doorText.setPosition(playerPosWorld.x - 70.f, playerPosWorld.y - 80.f);

                sf::FloatRect dtBounds = doorText.getGlobalBounds();
                sf::RectangleShape dtBg(sf::Vector2f(dtBounds.width + 12.f, dtBounds.height + 10.f));
                dtBg.setPosition(doorText.getPosition().x - 6.f, doorText.getPosition().y - 4.f);
                dtBg.setFillColor(sf::Color(0, 0, 0, 160));
                window.draw(dtBg);
                window.draw(doorText);

                // Only open if key in hand and actively playing
                if (gameState == PLAYING && gameMap.isKeyCollected() && sf::Keyboard::isKeyPressed(sf::Keyboard::E)) {
                    gameMap.openDoor();
                }
            }
        }

        // =================== HUD OVERLAY ===================
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

        // =================== PAUSE OVERLAY ===================
        if (gameState == PAUSED) {
            // Semi-transparent dark overlay over the frozen game world
            sf::RectangleShape pauseOverlay(sf::Vector2f(WINDOW_WIDTH, WINDOW_HEIGHT));
            pauseOverlay.setFillColor(sf::Color(0, 0, 0, 170));
            window.draw(pauseOverlay);

            // Title
            sf::Text pauseTitle;
            pauseTitle.setFont(font);
            pauseTitle.setString("PAUSED");
            pauseTitle.setCharacterSize(52);
            pauseTitle.setFillColor(sf::Color(220, 190, 60));
            pauseTitle.setStyle(sf::Text::Bold);
            sf::FloatRect ptb = pauseTitle.getLocalBounds();
            pauseTitle.setOrigin(ptb.left + ptb.width / 2.f, ptb.top + ptb.height / 2.f);
            pauseTitle.setPosition(WINDOW_WIDTH / 2.f, 200.f);
            window.draw(pauseTitle);

            // Decorative line
            sf::RectangleShape pauseLine(sf::Vector2f(250.f, 2.f));
            pauseLine.setFillColor(sf::Color(180, 150, 50, 160));
            pauseLine.setPosition(WINDOW_WIDTH / 2.f - 125.f, 240.f);
            window.draw(pauseLine);

            // Buttons
            drawButton(window, font, "Resume", BTN_X, 300.f, BTN_W, BTN_H, mouseScreen);
            drawButton(window, font, "Retry",  BTN_X, 370.f, BTN_W, BTN_H, mouseScreen);
            drawButton(window, font, "Quit",   BTN_X, 440.f, BTN_W, BTN_H, mouseScreen);
        }

        window.display();
    }

    // Cleanup
    for (auto enemy : enemies) delete enemy;
    for (auto bullet : bullets) delete bullet;

    return 0;
}
