#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <string>
#include <fstream>
#include <sstream>

#include "Map.h"
#include "Player.h"
#include "Enemy.h"
#include "Bullet.h"
#include "QuadTree.h"

const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;
const int INITIAL_ENEMY_COUNT = 10;
const int MAX_ENEMIES_ON_SCREEN = 20;

// Data persistence structures
struct GameRecord {
    std::string levelName;
    std::string status;
    int score;
    int enemiesKilled;
    int coins;
};

void loadGameRecords(std::vector<GameRecord>& history, int& totalCoins, int& highScore) {
    history.clear();
    totalCoins = 0;
    highScore = 0;
    std::ifstream file("save_data.txt");
    if (!file.is_open()) return;
    
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string token;
        GameRecord rec;
        
        std::getline(ss, rec.levelName, ',');
        std::getline(ss, rec.status, ',');
        
        std::getline(ss, token, ',');
        rec.score = token.empty() ? 0 : std::stoi(token);
        
        std::getline(ss, token, ',');
        rec.enemiesKilled = token.empty() ? 0 : std::stoi(token);
        
        std::getline(ss, token, ',');
        rec.coins = token.empty() ? 0 : std::stoi(token);
        
        history.push_back(rec);
        totalCoins += rec.coins;
        if (rec.score > highScore) highScore = rec.score;
    }
}

void saveGameRecord(const GameRecord& rec, std::vector<GameRecord>& history, int& totalCoins, int& highScore) {
    history.push_back(rec);
    totalCoins += rec.coins;
    if (rec.score > highScore) highScore = rec.score;
    
    std::ofstream file("save_data.txt", std::ios::app);
    if (file.is_open()) {
        file << rec.levelName << "," << rec.status << "," << rec.score << "," 
             << rec.enemiesKilled << "," << rec.coins << "\n";
    }
}

// Skill Tree Data Structure
struct SkillNode {
    std::string id;
    std::string displayName;
    int cost;
    bool unlocked;
    std::vector<SkillNode*> children;
    
    SkillNode(std::string i, std::string n, int c, bool u) 
        : id(i), displayName(n), cost(c), unlocked(u) {}
    ~SkillNode() {
        for (auto child : children) delete child;
    }
};

SkillNode* skillTreeRoot = nullptr;
int availableCoins = 0;

void initSkillTree() {
    if (skillTreeRoot) delete skillTreeRoot;
    skillTreeRoot = new SkillNode("root", "Root", 0, true);
    
    // Speed Branch
    SkillNode* s1 = new SkillNode("speed1", "Speed Lvl. 1", 0, true);
    SkillNode* s2 = new SkillNode("speed2", "Speed Lvl. 2", 1000, false);
    SkillNode* s3 = new SkillNode("speed3", "Speed Lvl. 3", 2000, false);
    s1->children.push_back(s2);
    s2->children.push_back(s3);
    skillTreeRoot->children.push_back(s1);
    
    // Power Branch
    SkillNode* p1 = new SkillNode("power1", "Power Lvl. 1", 0, true);
    SkillNode* p2 = new SkillNode("power2", "Power Lvl. 2", 1000, false);
    SkillNode* p3 = new SkillNode("power3", "Power Lvl. 3", 2000, false);
    p1->children.push_back(p2);
    p2->children.push_back(p3);
    skillTreeRoot->children.push_back(p1);
}

void saveProfile() {
    std::ofstream out("profile.txt");
    if(out.is_open()) {
        out << availableCoins << "\n";
        out << skillTreeRoot->children[0]->children[0]->unlocked << "\n"; // speed2
        out << skillTreeRoot->children[0]->children[0]->children[0]->unlocked << "\n"; // speed3
        out << skillTreeRoot->children[1]->children[0]->unlocked << "\n"; // power2
        out << skillTreeRoot->children[1]->children[0]->children[0]->unlocked << "\n"; // power3
    }
}

void loadProfile(int fallbackCoins) {
    std::ifstream in("profile.txt");
    if(in.is_open()) {
        std::string line;
        std::getline(in, line); availableCoins = std::stoi(line);
        std::getline(in, line); skillTreeRoot->children[0]->children[0]->unlocked = (line == "1");
        std::getline(in, line); skillTreeRoot->children[0]->children[0]->children[0]->unlocked = (line == "1");
        std::getline(in, line); skillTreeRoot->children[1]->children[0]->unlocked = (line == "1");
        std::getline(in, line); skillTreeRoot->children[1]->children[0]->children[0]->unlocked = (line == "1");
    } else {
        availableCoins = fallbackCoins; 
        saveProfile();
    }
}

// Applies upgrades to player instance
void applyPlayerStatsFromTree(Player& player) {
    if(!skillTreeRoot) return;
    
    SkillNode* speedNode = skillTreeRoot->children[0];
    int speedTier = 1;
    while(speedNode->children.size() > 0 && speedNode->children[0]->unlocked) {
        speedTier++;
        speedNode = speedNode->children[0];
    }
    if (speedTier == 1) player.setSpeedMultiplier(1.0f);
    else if (speedTier == 2) player.setSpeedMultiplier(1.1f);
    else player.setSpeedMultiplier(1.2f);
    
    SkillNode* powerNode = skillTreeRoot->children[1];
    int powerTier = 1;
    while(powerNode->children.size() > 0 && powerNode->children[0]->unlocked) {
        powerTier++;
        powerNode = powerNode->children[0];
    }
    if (powerTier == 1) player.setBaseDamage(10);
    else if (powerTier == 2) player.setBaseDamage(15);
    else player.setBaseDamage(20);
}

// Helper to buy next node
void buyNextNode(SkillNode* branchHead) {
    SkillNode* curr = branchHead;
    while(curr->children.size() > 0 && curr->children[0]->unlocked) {
        curr = curr->children[0];
    }
    
    if (curr->children.size() > 0 && !curr->children[0]->unlocked) {
        SkillNode* nextUpgrade = curr->children[0];
        if (availableCoins >= nextUpgrade->cost) {
            availableCoins -= nextUpgrade->cost;
            nextUpgrade->unlocked = true;
            saveProfile();
        }
    }
}

// Safe spawning helper function that ensures an enemy doesn't spawn inside a wall or too close to the player
// level: 1=skeletons only, 2=enemy2+enemy3, 3=enemy3+enemy4, 4=infinite slash (all)
void spawnEnemy(std::vector<Enemy*>& enemies, const Map& gameMap, const sf::Vector2f& playerPos, int level) {
    Enemy* enemy = new Enemy();
    
    if (level == 1) {
        // Level 1: Skeleton Horde — only skeletons
        enemy->load("assets/enemy.png", 9, 4);
    } else if (level == 2) {
        // Level 2: Dark Warriors — enemy2 and enemy3
        int rng = std::rand() % 2;
        enemy->loadAdvancedEnemy(rng == 0 ? 1 : 2);
    } else if (level == 3) {
        // Level 3: Elite Beasts — enemy3 and enemy4
        int rng = std::rand() % 2;
        enemy->loadAdvancedEnemy(rng == 0 ? 2 : 3);
    } else {
        // Level 4: Infinite Slash — all enemy types
        int rng = std::rand() % 4;
        if (rng == 0) {
            enemy->load("assets/enemy.png", 9, 4);
        } else {
            enemy->loadAdvancedEnemy(rng);
        }
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
                const sf::Vector2f& mousePos, bool isRedTheme = false) {
    sf::RectangleShape btn(sf::Vector2f(w, h));
    btn.setPosition(x, y);

    bool hovered = btn.getGlobalBounds().contains(mousePos);

    if (hovered) {
        btn.setFillColor(isRedTheme ? sf::Color(130, 40, 40, 235) : sf::Color(80, 70, 55, 235));
        btn.setOutlineColor(isRedTheme ? sf::Color(255, 120, 120) : sf::Color(255, 215, 0));
    } else {
        btn.setFillColor(isRedTheme ? sf::Color(80, 25, 25, 220) : sf::Color(45, 40, 35, 220));
        btn.setOutlineColor(isRedTheme ? sf::Color(200, 60, 60) : sf::Color(160, 130, 40));
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

    // 3. Initialize Enemies (populated when a level is selected)
    std::vector<Enemy*> enemies;

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

    sf::Texture coinTex;
    sf::Sprite coinSpr;
    if (coinTex.loadFromFile("assets/objects/coin.png")) {
        coinSpr.setTexture(coinTex);
        coinSpr.setScale(30.f / coinTex.getSize().x, 30.f / coinTex.getSize().y);
    }
    
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
    enum GameState { MAIN_MENU, LEVEL_SELECT, STORE_MENU, SCORES_MENU, PLAYING, PAUSED, GAME_OVER_STATE, ESCAPED_STATE };
    GameState gameState = MAIN_MENU;
    int currentLevel = 0; // 1=Skeleton Horde, 2=Dark Warriors, 3=Elite Beasts, 4=Infinite Slash
    float spawnTimer = 0.0f;
    float spawnInterval = 3.0f;
    bool showScoresMsg = false; // placeholder for Scores button

    int coins = 0;
    int enemiesKilled = 0;

    // Load history
    std::vector<GameRecord> matchHistory;
    int totalLifetimeCoins = 0;
    int absoluteHighScore = 0;
    loadGameRecords(matchHistory, totalLifetimeCoins, absoluteHighScore);
    
    initSkillTree();
    loadProfile(totalLifetimeCoins);

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
        applyPlayerStatsFromTree(player);
        gameMap.reset();

        score = 0;
        coins = 0;
        enemiesKilled = 0;
        spawnTimer = 0.0f;
        spawnInterval = (currentLevel == 1) ? 2.0f : 3.0f;
        hasPlayedGameOver = false;

        for (int i = 0; i < INITIAL_ENEMY_COUNT; i++) {
            spawnEnemy(enemies, gameMap, player.getPosition(), currentLevel);
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

                if (gameState == MAIN_MENU) {
                    showScoresMsg = false;
                    if (mx >= BTN_X && mx <= BTN_X + BTN_W && my >= 280.f && my <= 335.f) gameState = LEVEL_SELECT;  // Play
                    if (mx >= BTN_X && mx <= BTN_X + BTN_W && my >= 350.f && my <= 405.f) gameState = STORE_MENU;    // Store
                    if (mx >= BTN_X && mx <= BTN_X + BTN_W && my >= 420.f && my <= 475.f) gameState = SCORES_MENU;   // Scores
                    if (mx >= BTN_X && mx <= BTN_X + BTN_W && my >= 490.f && my <= 545.f) window.close();            // Quit
                }
                else if (gameState == STORE_MENU) {
                    if (mx >= BTN_X && mx <= BTN_X + BTN_W && my >= 620.f && my <= 665.f) gameState = MAIN_MENU;     // Back
                    if (mx >= 300.f && mx <= 500.f && my >= 350.f && my <= 400.f) {
                        buyNextNode(skillTreeRoot->children[0]);
                        applyPlayerStatsFromTree(player);
                    }
                    if (mx >= 780.f && mx <= 980.f && my >= 350.f && my <= 400.f) {
                        buyNextNode(skillTreeRoot->children[1]);
                        applyPlayerStatsFromTree(player);
                    }
                }
                else if (gameState == SCORES_MENU) {
                    if (mx >= BTN_X && mx <= BTN_X + BTN_W && my >= 530.f && my <= 575.f) gameState = MAIN_MENU;     // Back
                }
                else if (gameState == LEVEL_SELECT) {
                    // Level buttons
                    if (mx >= BTN_X && mx <= BTN_X + BTN_W && my >= 230.f && my <= 285.f) { currentLevel = 1; resetGame(); } // Level 1
                    if (mx >= BTN_X && mx <= BTN_X + BTN_W && my >= 300.f && my <= 355.f) { currentLevel = 2; resetGame(); } // Level 2
                    if (mx >= BTN_X && mx <= BTN_X + BTN_W && my >= 370.f && my <= 425.f) { currentLevel = 3; resetGame(); } // Level 3
                    if (mx >= BTN_X && mx <= BTN_X + BTN_W && my >= 440.f && my <= 495.f) { currentLevel = 4; resetGame(); } // Infinite Slash
                    // Back button (narrower, centered, below)
                    if (mx >= BTN_X && mx <= BTN_X + BTN_W && my >= 530.f && my <= 575.f) gameState = MAIN_MENU;              // Back
                }
                else if (gameState == PAUSED) {
                    if (mx >= BTN_X && mx <= BTN_X + BTN_W && my >= 280.f && my <= 335.f) gameState = PLAYING;   // Resume
                    if (mx >= BTN_X && mx <= BTN_X + BTN_W && my >= 350.f && my <= 405.f) resetGame();           // Retry
                    if (mx >= BTN_X && mx <= BTN_X + BTN_W && my >= 420.f && my <= 475.f) { bgMusic.stop(); gameState = MAIN_MENU; } // Home
                    if (mx >= BTN_X && mx <= BTN_X + BTN_W && my >= 490.f && my <= 545.f) window.close();        // Quit
                }
                else if (gameState == GAME_OVER_STATE) {
                    if (mx >= BTN_X && mx <= BTN_X + BTN_W && my >= 370.f && my <= 425.f) resetGame();           // Retry
                    if (mx >= BTN_X && mx <= BTN_X + BTN_W && my >= 440.f && my <= 495.f) { gameOverMusic.stop(); gameState = MAIN_MENU; } // Home
                    if (mx >= BTN_X && mx <= BTN_X + BTN_W && my >= 510.f && my <= 565.f) window.close();        // Quit
                }
                else if (gameState == ESCAPED_STATE) {
                    if (mx >= BTN_X && mx <= BTN_X + BTN_W && my >= 380.f && my <= 435.f) gameState = MAIN_MENU; // Home
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
                            int t = e->getType();
                            if (e->takeDamage(player.getDamage(), player.getPosition())) {
                                score += 10;
                                enemiesKilled++;
                                if (t == 0) coins += 10;
                                else if (t == 1) coins += 20;
                                else coins += 25;
                            }
                        }
                    }
                }
            }
        }

        // =================== MAIN MENU SCREEN ===================
        if (gameState == MAIN_MENU) {
            window.clear(sf::Color(10, 8, 6));
            window.setView(window.getDefaultView());

            // Dark dungeon overlay
            sf::RectangleShape overlay(sf::Vector2f(WINDOW_WIDTH, WINDOW_HEIGHT));
            overlay.setFillColor(sf::Color(15, 12, 8, 240));
            window.draw(overlay);

            // Title: DUNGEON ESCAPE
            sf::Text title;
            title.setFont(font);
            title.setString("DUNGEON ESCAPE");
            title.setCharacterSize(64);
            title.setFillColor(sf::Color(220, 190, 60));
            title.setStyle(sf::Text::Bold);
            sf::FloatRect ttb = title.getLocalBounds();
            title.setOrigin(ttb.left + ttb.width / 2.f, ttb.top + ttb.height / 2.f);
            title.setPosition(WINDOW_WIDTH / 2.f, 140.f);
            window.draw(title);

            // Subtitle
            sf::Text sub;
            sub.setFont(font);
            sub.setString("Survive the dungeon. Slay or be slain.");
            sub.setCharacterSize(16);
            sub.setFillColor(sf::Color(140, 120, 80));
            sf::FloatRect stb = sub.getLocalBounds();
            sub.setOrigin(stb.left + stb.width / 2.f, stb.top + stb.height / 2.f);
            sub.setPosition(WINDOW_WIDTH / 2.f, 195.f);
            window.draw(sub);

            // Decorative line
            sf::RectangleShape dLine(sf::Vector2f(350.f, 2.f));
            dLine.setFillColor(sf::Color(180, 150, 50, 160));
            dLine.setPosition(WINDOW_WIDTH / 2.f - 175.f, 225.f);
            window.draw(dLine);

            // Buttons
            drawButton(window, font, "Play",   BTN_X, 280.f, BTN_W, BTN_H, mouseScreen);
            drawButton(window, font, "Store",  BTN_X, 350.f, BTN_W, BTN_H, mouseScreen);
            drawButton(window, font, "Scores", BTN_X, 420.f, BTN_W, BTN_H, mouseScreen);
            drawButton(window, font, "Quit",   BTN_X, 490.f, BTN_W, BTN_H, mouseScreen);

            // Show stats
            sf::Text statsTxt;
            statsTxt.setFont(font);
            statsTxt.setString("Total Coins: " + std::to_string(totalLifetimeCoins) + "   |   High Score: " + std::to_string(absoluteHighScore));
            statsTxt.setCharacterSize(18);
            statsTxt.setFillColor(sf::Color(200, 180, 100));
            sf::FloatRect sttb = statsTxt.getLocalBounds();
            statsTxt.setOrigin(sttb.left + sttb.width / 2.f, sttb.top + sttb.height / 2.f);
            statsTxt.setPosition(WINDOW_WIDTH / 2.f, 260.f);
            window.draw(statsTxt);

            // Footer
            sf::Text footer;
            footer.setFont(font);
            footer.setString("DS II Course Project  |  SFML + QuadTree");
            footer.setCharacterSize(12);
            footer.setFillColor(sf::Color(80, 70, 50));
            sf::FloatRect ftb = footer.getLocalBounds();
            footer.setOrigin(ftb.left + ftb.width / 2.f, ftb.top + ftb.height / 2.f);
            footer.setPosition(WINDOW_WIDTH / 2.f, WINDOW_HEIGHT - 30.f);
            window.draw(footer);

            window.display();
            continue;
        }

        // =================== STORE MENU SCREEN ===================
        if (gameState == STORE_MENU) {
            window.clear(sf::Color(10, 8, 6));
            window.setView(window.getDefaultView());

            sf::RectangleShape overlay(sf::Vector2f(WINDOW_WIDTH, WINDOW_HEIGHT));
            overlay.setFillColor(sf::Color(15, 12, 8, 240));
            window.draw(overlay);

            sf::Text title;
            title.setFont(font);
            title.setString("UPGRADE STORE");
            title.setCharacterSize(52);
            title.setFillColor(sf::Color(220, 190, 60));
            title.setStyle(sf::Text::Bold);
            sf::FloatRect ltb = title.getLocalBounds();
            title.setOrigin(ltb.left + ltb.width / 2.f, ltb.top + ltb.height / 2.f);
            title.setPosition(WINDOW_WIDTH / 2.f, 80.f);
            window.draw(title);

            sf::RectangleShape dLine(sf::Vector2f(350.f, 2.f));
            dLine.setFillColor(sf::Color(180, 150, 50, 160));
            dLine.setPosition(WINDOW_WIDTH / 2.f - 175.f, 120.f);
            window.draw(dLine);

            sf::Text avCoins;
            avCoins.setFont(font);
            avCoins.setString("Available Coins: " + std::to_string(availableCoins));
            avCoins.setCharacterSize(28);
            avCoins.setFillColor(sf::Color(255, 215, 0));
            sf::FloatRect acb = avCoins.getLocalBounds();
            avCoins.setOrigin(acb.left + acb.width / 2.f, acb.top + acb.height / 2.f);
            avCoins.setPosition(WINDOW_WIDTH / 2.f, 180.f);
            window.draw(avCoins);

            // Fetch Next Nodes
            SkillNode* sNode = skillTreeRoot->children[0];
            while (sNode->children.size() > 0 && sNode->children[0]->unlocked) sNode = sNode->children[0];
            bool sMax = (sNode->children.size() == 0 || sNode->children[0]->unlocked);
            
            SkillNode* pNode = skillTreeRoot->children[1];
            while (pNode->children.size() > 0 && pNode->children[0]->unlocked) pNode = pNode->children[0];
            bool pMax = (pNode->children.size() == 0 || pNode->children[0]->unlocked);

            // Left Node (Speed) Display
            sf::Text sTxt;
            sTxt.setFont(font);
            sTxt.setString(sMax ? "Speed MAX" : "Next: " + sNode->children[0]->displayName);
            sTxt.setCharacterSize(24);
            sTxt.setFillColor(sf::Color::White);
            sf::FloatRect sTxB = sTxt.getLocalBounds();
            sTxt.setOrigin(sTxB.left + sTxB.width / 2.f, sTxB.top + sTxB.height / 2.f);
            sTxt.setPosition(400.f, 280.f);
            window.draw(sTxt);

            if (!sMax) {
                drawButton(window, font, "+ (Cost: " + std::to_string(sNode->children[0]->cost) + ")", 300.f, 350.f, 200.f, 50.f, mouseScreen, false);
            }

            // Right Node (Power) Display
            sf::Text pTxt;
            pTxt.setFont(font);
            pTxt.setString(pMax ? "Power MAX" : "Next: " + pNode->children[0]->displayName);
            pTxt.setCharacterSize(24);
            pTxt.setFillColor(sf::Color::White);
            sf::FloatRect pTxB = pTxt.getLocalBounds();
            pTxt.setOrigin(pTxB.left + pTxB.width / 2.f, pTxB.top + pTxB.height / 2.f);
            pTxt.setPosition(880.f, 280.f);
            window.draw(pTxt);

            if (!pMax) {
                drawButton(window, font, "+ (Cost: " + std::to_string(pNode->children[0]->cost) + ")", 780.f, 350.f, 200.f, 50.f, mouseScreen, false);
            }

            drawButton(window, font, "Back", BTN_X, 620.f, BTN_W, 45.f, mouseScreen, false);

            window.display();
            continue;
        }

        // =================== SCORES MENU SCREEN ===================
        if (gameState == SCORES_MENU) {
            window.clear(sf::Color(10, 8, 6));
            window.setView(window.getDefaultView());

            // Dark overlay
            sf::RectangleShape overlay(sf::Vector2f(WINDOW_WIDTH, WINDOW_HEIGHT));
            overlay.setFillColor(sf::Color(15, 12, 8, 240));
            window.draw(overlay);

            // Title
            sf::Text title;
            title.setFont(font);
            title.setString("RECENT MATCHES");
            title.setCharacterSize(52);
            title.setFillColor(sf::Color(220, 190, 60));
            title.setStyle(sf::Text::Bold);
            sf::FloatRect ltb = title.getLocalBounds();
            title.setOrigin(ltb.left + ltb.width / 2.f, ltb.top + ltb.height / 2.f);
            title.setPosition(WINDOW_WIDTH / 2.f, 80.f);
            window.draw(title);

            // Decorative line
            sf::RectangleShape dLine(sf::Vector2f(350.f, 2.f));
            dLine.setFillColor(sf::Color(180, 150, 50, 160));
            dLine.setPosition(WINDOW_WIDTH / 2.f - 175.f, 120.f);
            window.draw(dLine);

            // Render up to 10 most recent games
            float yPos = 160.f;
            int count = 0;
            for (auto it = matchHistory.rbegin(); it != matchHistory.rend() && count < 10; ++it, ++count) {
                sf::Text entry;
                entry.setFont(font);
                entry.setString(it->levelName + " (" + it->status + ") - Score: " + std::to_string(it->score) + 
                                " - Kills: " + std::to_string(it->enemiesKilled) + " - Coins: " + std::to_string(it->coins));
                entry.setCharacterSize(16);
                entry.setFillColor(it->status == "Win" ? sf::Color(100, 255, 100) : sf::Color(255, 100, 100));
                sf::FloatRect eb = entry.getLocalBounds();
                entry.setOrigin(eb.left + eb.width / 2.f, eb.top + eb.height / 2.f);
                entry.setPosition(WINDOW_WIDTH / 2.f, yPos);
                window.draw(entry);
                yPos += 30.f;
            }

            if (matchHistory.empty()) {
                sf::Text emptyMsg;
                emptyMsg.setFont(font);
                emptyMsg.setString("No recent matches found.");
                emptyMsg.setCharacterSize(20);
                emptyMsg.setFillColor(sf::Color(180, 160, 100));
                sf::FloatRect emb = emptyMsg.getLocalBounds();
                emptyMsg.setOrigin(emb.left + emb.width / 2.f, emb.top + emb.height / 2.f);
                emptyMsg.setPosition(WINDOW_WIDTH / 2.f, 250.f);
                window.draw(emptyMsg);
            }

            // Back button
            drawButton(window, font, "Back", BTN_X, 530.f, BTN_W, 40.f, mouseScreen, false);

            window.display();
            continue;
        }

        // =================== LEVEL SELECT SCREEN ===================
        if (gameState == LEVEL_SELECT) {
            window.clear(sf::Color(10, 8, 6));
            window.setView(window.getDefaultView());

            // Dark overlay
            sf::RectangleShape overlay(sf::Vector2f(WINDOW_WIDTH, WINDOW_HEIGHT));
            overlay.setFillColor(sf::Color(15, 12, 8, 240));
            window.draw(overlay);

            // Title
            sf::Text title;
            title.setFont(font);
            title.setString("SELECT LEVEL");
            title.setCharacterSize(52);
            title.setFillColor(sf::Color(220, 190, 60));
            title.setStyle(sf::Text::Bold);
            sf::FloatRect ltb = title.getLocalBounds();
            title.setOrigin(ltb.left + ltb.width / 2.f, ltb.top + ltb.height / 2.f);
            title.setPosition(WINDOW_WIDTH / 2.f, 120.f);
            window.draw(title);

            // Decorative line
            sf::RectangleShape dLine(sf::Vector2f(300.f, 2.f));
            dLine.setFillColor(sf::Color(180, 150, 50, 160));
            dLine.setPosition(WINDOW_WIDTH / 2.f - 150.f, 160.f);
            window.draw(dLine);

            // Subtitle
            sf::Text sub;
            sub.setFont(font);
            sub.setString("Choose your challenge");
            sub.setCharacterSize(16);
            sub.setFillColor(sf::Color(140, 120, 80));
            sf::FloatRect slb = sub.getLocalBounds();
            sub.setOrigin(slb.left + slb.width / 2.f, slb.top + slb.height / 2.f);
            sub.setPosition(WINDOW_WIDTH / 2.f, 185.f);
            window.draw(sub);

            // Level buttons
            drawButton(window, font, "Easy (Skeleton Horde)",    BTN_X, 230.f, BTN_W, BTN_H, mouseScreen, true);
            drawButton(window, font, "Medium (Dark Warriors)",   BTN_X, 300.f, BTN_W, BTN_H, mouseScreen, true);
            drawButton(window, font, "Hard (Elite Beasts)",      BTN_X, 370.f, BTN_W, BTN_H, mouseScreen, true);
            drawButton(window, font, "Hell (Infinite Slash)",    BTN_X, 440.f, BTN_W, BTN_H, mouseScreen, true);

            // Back button
            drawButton(window, font, "Back", BTN_X, 530.f, BTN_W, 40.f, mouseScreen, false);

            window.display();
            continue;
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
            drawButton(window, font, "Home", BTN_X, 380.f, BTN_W, BTN_H, mouseScreen);
            drawButton(window, font, "Quit", BTN_X, 450.f, BTN_W, BTN_H, mouseScreen);

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
            drawButton(window, font, "Retry", BTN_X, 370.f, BTN_W, BTN_H, mouseScreen);
            drawButton(window, font, "Home",  BTN_X, 440.f, BTN_W, BTN_H, mouseScreen);
            drawButton(window, font, "Quit",  BTN_X, 510.f, BTN_W, BTN_H, mouseScreen);

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
                spawnEnemy(enemies, gameMap, player.getPosition(), currentLevel);
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
                    int t = enemies[enemyIndex]->getType();
                    if (enemies[enemyIndex]->takeDamage(player.getDamage(), player.getPosition())) {
                        score += 10;
                        enemiesKilled++;
                        if (t == 0) coins += 10;
                        else if (t == 1) coins += 20;
                        else coins += 25;
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

        // CHEST PICKUP
        if (!gameMap.isChestOpened() && player.getGlobalBounds().intersects(gameMap.getChestBounds())) {
            gameMap.openChest();
            coins += 300;
        }

        // WIN ZONE: entering the door area after door is opened
        if (gameMap.isDoorOpen() &&
            player.getGlobalBounds().intersects(gameMap.getDoorAreaBounds())) {
            
            if (gameState != ESCAPED_STATE) {
                std::string lvlStr = (currentLevel == 1) ? "Easy" : (currentLevel == 2) ? "Medium" : (currentLevel == 3) ? "Hard" : "Hell";
                saveGameRecord({lvlStr, "Win", score, enemiesKilled, coins}, matchHistory, totalLifetimeCoins, absoluteHighScore);
                availableCoins += coins;
                saveProfile();
            }
            gameState = ESCAPED_STATE;
            bgMusic.stop();
        }

        // --- TAKING DAMAGE --- //
        // From Enemies
        for (auto& enemy : enemies) {
            if ((enemy->getState() == ALIVE || enemy->getState() == ATTACKING) && enemy->getDamageBounds().intersects(player.getGlobalBounds())) {
                float dmg = (enemy->getType() >= 1) ? 20.0f : 10.0f;
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
            if (gameState != GAME_OVER_STATE) {
                std::string lvlStr = (currentLevel == 1) ? "Easy" : (currentLevel == 2) ? "Medium" : (currentLevel == 3) ? "Hard" : "Hell";
                saveGameRecord({lvlStr, "Lose", score, enemiesKilled, coins}, matchHistory, totalLifetimeCoins, absoluteHighScore);
                availableCoins += coins;
                saveProfile();
            }
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

        // Draw Coins
        coinSpr.setPosition(WINDOW_WIDTH - 150.f, 10.f);
        window.draw(coinSpr);
        sf::Text coinsText;
        coinsText.setFont(font);
        coinsText.setString(std::to_string(coins));
        coinsText.setCharacterSize(24);
        coinsText.setFillColor(sf::Color(255, 215, 0)); // Gold
        coinsText.setPosition(WINDOW_WIDTH - 110.f, 8.f);
        window.draw(coinsText);

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
            drawButton(window, font, "Resume", BTN_X, 280.f, BTN_W, BTN_H, mouseScreen);
            drawButton(window, font, "Retry",  BTN_X, 350.f, BTN_W, BTN_H, mouseScreen);
            drawButton(window, font, "Home",   BTN_X, 420.f, BTN_W, BTN_H, mouseScreen);
            drawButton(window, font, "Quit",   BTN_X, 490.f, BTN_W, BTN_H, mouseScreen);
        }

        window.display();
    }

    // Cleanup
    for (auto enemy : enemies) delete enemy;
    for (auto bullet : bullets) delete bullet;

    return 0;
}
