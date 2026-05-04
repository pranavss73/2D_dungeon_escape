#include "Player.h"
#include <iostream>
#include <cmath>

Player::Player() {
    speed = 200.0f; // Pixels per second
    columns = 1;
    currentFrameX = 0;
    animationTime = 0.0f;
    animationSpeed = 0.1f; // Faster animation
    velocity = sf::Vector2f(0.f, 0.f);
    currentDir = 1; // Default to down
    isAttacking = false;
    attackTimer = 0.0f;
    attackDuration = 0.3f; // 0.3 seconds per slash
    
    maxHealth = 100.0f;
    health = maxHealth;
    maxStamina = 50.0f;
    stamina = maxStamina;
    
    damageCooldown = 0.0f;
}

Player::~Player() {}

bool Player::loadTextures() {
    bool success = true;
    success &= idleTextures[0].loadFromFile("assets/Hero1/IDLE/idle_up.png");
    success &= idleTextures[1].loadFromFile("assets/Hero1/IDLE/idle_down.png");
    success &= idleTextures[2].loadFromFile("assets/Hero1/IDLE/idle_right.png");
    success &= idleTextures[3].loadFromFile("assets/Hero1/IDLE/idle_left.png");

    success &= runTextures[0].loadFromFile("assets/Hero1/RUN/run_up.png");
    success &= runTextures[1].loadFromFile("assets/Hero1/RUN/run_down.png");
    success &= runTextures[2].loadFromFile("assets/Hero1/RUN/run_right.png");
    success &= runTextures[3].loadFromFile("assets/Hero1/RUN/run_left.png");

    success &= attackTextures[0].loadFromFile("assets/Hero1/ATTACK_1_IDLE/attack1_up.png");
    success &= attackTextures[1].loadFromFile("assets/Hero1/ATTACK_1_IDLE/attack1_down.png");
    success &= attackTextures[2].loadFromFile("assets/Hero1/ATTACK_1_IDLE/attack1_right.png");
    success &= attackTextures[3].loadFromFile("assets/Hero1/ATTACK_1_IDLE/attack1_left.png");

    if (!success) {
        std::cout << "Failed to load player textures" << std::endl;
        return false;
    }

    columns = 8;
    frameSize.x = 96;
    frameSize.y = 80;

    sprite.setTexture(idleTextures[currentDir]);
    sprite.setTextureRect(sf::IntRect(0, 0, frameSize.x, frameSize.y));
    sprite.setOrigin(frameSize.x / 2.0f, frameSize.y / 2.0f);
    sprite.setScale(1.5f, 1.5f);

    return true;
}

void Player::updateAnimation(float deltaTime) {
    if (isAttacking) {
        attackTimer += deltaTime;
        if (attackTimer >= attackDuration) {
            isAttacking = false;
        } else {
            animationTime += deltaTime;
            if (animationTime >= (attackDuration / columns)) {
                animationTime = 0.0f;
                currentFrameX = (currentFrameX + 1) % columns;
            }
            sprite.setTextureRect(sf::IntRect(currentFrameX * frameSize.x, 0, frameSize.x, frameSize.y));
            return;
        }
    }

    bool isMoving = (velocity.x != 0 || velocity.y != 0);

    if (velocity.y > 0) currentDir = 1; // Down
    else if (velocity.x < 0) currentDir = 3; // Left
    else if (velocity.x > 0) currentDir = 2; // Right
    else if (velocity.y < 0) currentDir = 0; // Up

    if (isMoving) {
        sprite.setTexture(runTextures[currentDir]);
    } else {
        sprite.setTexture(idleTextures[currentDir]);
    }

    animationTime += deltaTime;
    if (animationTime >= animationSpeed) {
        animationTime = 0.0f;
        currentFrameX = (currentFrameX + 1) % columns;
        sprite.setTextureRect(sf::IntRect(currentFrameX * frameSize.x, 0, frameSize.x, frameSize.y));
    }
}

void Player::update(float deltaTime, const sf::Vector2f& mousePos, const Map& gameMap) {
    velocity = sf::Vector2f(0.f, 0.f);

    // Apply stamina regeneration while idle/running
    if (velocity.x == 0 && velocity.y == 0) {
        stamina += 5.0f * deltaTime; // Lowered idle gain
    } else {
        stamina += 1.5f * deltaTime; // Lowered running gain
    }
    if (stamina > maxStamina) stamina = maxStamina;

    // Handle damage iFrames
    if (damageCooldown > 0.0f) {
        damageCooldown -= deltaTime;
    }

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) velocity.y -= speed;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) velocity.y += speed;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) velocity.x -= speed;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) velocity.x += speed;

    // Apply X movement and check collision
    sprite.move(velocity.x * deltaTime, 0);
    sf::FloatRect boundsAfterX = getGlobalBounds();
    for (const auto& obs : gameMap.getObstacles()) {
        if (boundsAfterX.intersects(obs)) {
            // Revert X movement if collision
            sprite.move(-velocity.x * deltaTime, 0);
            velocity.x = 0; // stop animation on this axis
            break;
        }
    }

    // Apply Y movement and check collision
    sprite.move(0, velocity.y * deltaTime);
    sf::FloatRect boundsAfterY = getGlobalBounds();
    for (const auto& obs : gameMap.getObstacles()) {
        if (boundsAfterY.intersects(obs)) {
            // Revert Y movement if collision
            sprite.move(0, -velocity.y * deltaTime);
            velocity.y = 0; // stop animation on this axis
            break;
        }
    }

    updateAnimation(deltaTime);

    // Rotation towards mouse
    sf::Vector2f playerPos = sprite.getPosition();
    float dx = mousePos.x - playerPos.x;
    float dy = mousePos.y - playerPos.y;
    float rotation = (atan2(dy, dx)) * 180.0f / 3.14159f;
    
    // SFML rotation is 0 degrees facing right (X axis). 
    // You can apply rotation if your sprite sheet is top-down shooter style.
    // sprite.setRotation(rotation); 
}

void Player::draw(sf::RenderWindow& window) {
    // Generate and draw shadow beneath feet first
    sf::FloatRect spriteBounds = sprite.getGlobalBounds();
    sf::CircleShape shadow(spriteBounds.width * 0.10f); // Tighter radius (10%)
    shadow.setOrigin(shadow.getRadius(), shadow.getRadius());
    shadow.setScale(1.f, 0.4f); // Squash into perspective ellipse
    shadow.setFillColor(sf::Color(0, 0, 0, 120)); // Soft translucent black
    // The player's actual drawn feet are natively offset inside the frame to the left and up
    shadow.setPosition(sprite.getPosition().x - spriteBounds.width * 0.02f, 
                       sprite.getPosition().y + spriteBounds.height * 0.23f);
    window.draw(shadow);

    // Draw main character sprite on top
    window.draw(sprite);

    // DEBUG: Draw player collision bounds in transparent green
    sf::FloatRect bounds = getGlobalBounds();
    sf::RectangleShape box(sf::Vector2f(bounds.width, bounds.height));
    box.setPosition(bounds.left, bounds.top);
    box.setFillColor(sf::Color(0, 255, 0, 0)); 
    window.draw(box);
}

void Player::setPosition(float x, float y) {
    sprite.setPosition(x, y);
}

sf::Vector2f Player::getPosition() const {
    return sprite.getPosition();
}

sf::FloatRect Player::getGlobalBounds() const {
    sf::FloatRect bounds = sprite.getGlobalBounds();
    // Reduce bounds drastically to tightly fit the feet of the visual sprite
    // This allows the player to get much closer to walls without invisible bumping
    return sf::FloatRect(
        bounds.left + bounds.width * 0.40f,       // Center horizontally
        bounds.top + bounds.height * 0.70f,       // Push down strictly to feet
        bounds.width * 0.15f,                     // Narrow width to cover only feet
        bounds.height * 0.12f                     // Very short height for depth illusion
    );
}

bool Player::attack() {
    if (!isAttacking && stamina >= 10.0f) {
        drainStamina(10.0f);
        isAttacking = true;
        attackTimer = 0.0f;
        currentFrameX = 0;
        animationTime = 0.0f;
        sprite.setTexture(attackTextures[currentDir]);
        return true;
    }
    return false;
}

sf::FloatRect Player::getAttackBounds() const {
    sf::FloatRect bounds = sprite.getGlobalBounds();
    float rangeX = 80.0f;
    float rangeY = 80.0f;
    
    // Instead of pushing the rect completely outside the player's bounds, we keep
    // the bounds overlaying the player, and stretch it outwards in the direction facing.
    // This solves the issue of the attack "missing" enemies that are very close.
    if (currentDir == 0) { // Up
        return sf::FloatRect(bounds.left, bounds.top - rangeY, bounds.width, bounds.height + rangeY);
    } else if (currentDir == 1) { // Down
        return sf::FloatRect(bounds.left, bounds.top, bounds.width, bounds.height + rangeY);
    } else if (currentDir == 2) { // Right
        return sf::FloatRect(bounds.left, bounds.top, bounds.width + rangeX, bounds.height);
    } else if (currentDir == 3) { // Left
        return sf::FloatRect(bounds.left - rangeX, bounds.top, bounds.width + rangeX, bounds.height);
    }
    return sf::FloatRect(0,0,0,0);
}

// Resource UI Logic
float Player::getHealth() const {
    return health;
}

float Player::getStamina() const {
    return stamina;
}

void Player::takeDamage(float amount) {
    if (damageCooldown <= 0.0f) {
        health -= amount;
        if (health < 0) health = 0;
        damageCooldown = 1.0f; // 1 second of invincibility
    }
}

bool Player::drainStamina(float amount) {
    if (stamina >= amount) {
        stamina -= amount;
        return true;
    }
    return false;
}

void Player::reset(float x, float y) {
    health = maxHealth;
    stamina = maxStamina;
    damageCooldown = 0.0f;
    isAttacking = false;
    attackTimer = 0.0f;
    currentFrameX = 0;
    animationTime = 0.0f;
    currentDir = 1; // face down
    velocity = sf::Vector2f(0.f, 0.f);
    sprite.setPosition(x, y);
    sprite.setTexture(idleTextures[currentDir]);
    sprite.setTextureRect(sf::IntRect(0, 0, frameSize.x, frameSize.y));
}
