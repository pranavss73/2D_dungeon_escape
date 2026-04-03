#ifndef PLAYER_H
#define PLAYER_H

#include <SFML/Graphics.hpp>

#include "Map.h"

class Player {
public:
    Player();
    ~Player();

    bool loadTextures();
    void update(float deltaTime, const sf::Vector2f& mousePos, const Map& gameMap);
    void draw(sf::RenderWindow& window);

    bool attack();
    sf::FloatRect getAttackBounds() const;

    float getHealth() const;
    float getStamina() const;
    void takeDamage(float amount);
    bool drainStamina(float amount);

    void setPosition(float x, float y);
    sf::Vector2f getPosition() const;
    sf::FloatRect getGlobalBounds() const;

private:
    sf::Texture idleTextures[4];
    sf::Texture runTextures[4];
    sf::Texture attackTextures[4];
    sf::Sprite sprite;

    sf::Vector2f velocity;
    float speed;

    // Animation specific
    int columns;
    int currentDir; // 0: up, 1: down, 2: right, 3: left
    int currentFrameX;
    float animationTime;
    float animationSpeed;
    
    bool isAttacking;
    float attackTimer;
    float attackDuration;
    
    sf::Vector2i frameSize;
    
    float maxHealth;
    float health;
    float maxStamina;
    float stamina;
    
    float damageCooldown;

    void updateAnimation(float deltaTime);
};

#endif // PLAYER_H
