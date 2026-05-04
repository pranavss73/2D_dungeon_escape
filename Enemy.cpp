#include "Enemy.h"
#include <iostream>
#include <cmath>

Enemy::Enemy() {
    speed = 100.0f; 
    state = SPAWNING;
    columns = 1;
    rows = 1;
    currentFrameX = 0;
    currentFrameY = 0;
    animationTime = 0.0f;
    animationSpeed = 0.2f;

    hp = 10;
    type = 0;
    knockbackVelocity = sf::Vector2f(0.f, 0.f);
    baseColumns = 1;
    attackColumns = 1;
    hurtColumns = 1;

    effectColumns = 11;
    currentEffectFrame = 0;
    effectAnimationTime = 0.0f;
    effectAnimationSpeed = 0.08f; // fast effect animation
}

Enemy::~Enemy() {}

bool Enemy::load(const std::string& texturePath, int cols, int rws) {
    if (!texture.loadFromFile(texturePath)) {
        std::cout << "Failed to load enemy texture: " << texturePath << std::endl;
        return false;
    }
    sprite.setTexture(texture);
    columns = cols;
    baseColumns = cols;
    rows = rws;
    
    sf::Vector2u texSize = texture.getSize();
    frameSize.x = texSize.x / columns;
    frameSize.y = texSize.y / rows;

    sprite.setTextureRect(sf::IntRect(0, 0, frameSize.x, frameSize.y));
    sprite.setOrigin(frameSize.x / 2.0f, frameSize.y / 2.0f);
    sprite.setScale(0.8f, 0.8f);

    if (effectTexture.loadFromFile("assets/effect/enemy_die_spawn.png")) {
        effectSprite.setTexture(effectTexture);
        sf::Vector2u effSize = effectTexture.getSize();
        effectFrameSize.x = effSize.x / effectColumns;
        effectFrameSize.y = 65; // Since total is 130 and dyning is 0-65, spawning 65-130
        
        effectSprite.setTextureRect(sf::IntRect(0, 65, effectFrameSize.x, effectFrameSize.y));
        // Origin depends on state and will be set in update
    }
    
    return true;
}

bool Enemy::loadAdvancedEnemy(int t) {
    type = t;
    hp = (t + 1) * 10;
    if (t >= 2) speed = 130.0f; // Increase speed slightly for types 2 and 3
    
    std::string prefix = "enemy2";
    if (t == 2) prefix = "enemy3";
    if (t == 3) prefix = "enemy4";
    
    if (!texture.loadFromFile("assets/enemies/" + prefix + "_run_with_shadow.png")) return false;
    columns = 8;
    baseColumns = 8;
    rows = 4;

    if (!attackTexture.loadFromFile("assets/enemies/" + prefix + "_attack_with_shadow.png")) return false;
    attackColumns = 8;

    if (!hurtTexture.loadFromFile("assets/enemies/" + prefix + "_hurt_with_shadow.png")) return false;
    hurtColumns = 6;

    sf::Vector2u texSize = texture.getSize();
    frameSize.x = texSize.x / columns;
    frameSize.y = texSize.y / rows;

    sprite.setTexture(texture);
    sprite.setTextureRect(sf::IntRect(0, 0, frameSize.x, frameSize.y));
    sprite.setOrigin(frameSize.x / 2.0f, frameSize.y / 2.0f);
    sprite.setScale(1.5f, 1.5f); // Trimmed down visual size so they aren't unnaturally wide compared to player

    if (effectTexture.loadFromFile("assets/effect/enemy_die_spawn.png")) {
        effectSprite.setTexture(effectTexture);
        sf::Vector2u effSize = effectTexture.getSize();
        effectFrameSize.x = effSize.x / effectColumns;
        effectFrameSize.y = 65; 
        
        effectSprite.setTextureRect(sf::IntRect(0, 65, effectFrameSize.x, effectFrameSize.y));
    }
    
    return true;
}

void Enemy::updateAnimation(float deltaTime, bool moving, const sf::Vector2f& velocity) {
    if (moving || state == ATTACKING) {
        float currentAnimSpeed = animationSpeed;
        if (state == ATTACKING) currentAnimSpeed *= 0.5f; // Fast attack

        animationTime += deltaTime;
        if (animationTime >= currentAnimSpeed) {
            animationTime = 0.0f;
            currentFrameX++;
            
            if (state == ATTACKING && currentFrameX >= columns) {
                // Return to alive state after full swing
                state = ALIVE;
                sprite.setTexture(texture);
                columns = baseColumns;
                currentFrameX = 0;
            } else {
                currentFrameX = currentFrameX % columns;
            }
            
            currentFrameY = std::min(currentFrameY, rows - 1);
            sprite.setTextureRect(sf::IntRect(currentFrameX * frameSize.x, currentFrameY * frameSize.y, frameSize.x, frameSize.y));
        }
    } else {
        currentFrameX = 0;
        sprite.setTextureRect(sf::IntRect(0, currentFrameY * frameSize.y, frameSize.x, frameSize.y));
    }
}

void Enemy::update(float deltaTime, const sf::Vector2f& playerPos, const Map& gameMap) {
    if (state == DEAD) return;

    if (state == SPAWNING) {
        // Advance effect animation
        effectAnimationTime += deltaTime;
        if (effectAnimationTime >= effectAnimationSpeed) {
            effectAnimationTime = 0.0f;
            currentEffectFrame++;
            if (currentEffectFrame >= effectColumns) {
                // Done spawning
                state = ALIVE;
                currentEffectFrame = 0;
            } else {
                effectSprite.setTextureRect(sf::IntRect(currentEffectFrame * effectFrameSize.x, 65, effectFrameSize.x, effectFrameSize.y));
            }
        }
        return; // Don't move or AI while spawning
    }

    if (state == DYING) {
        // Advance effect animation
        effectAnimationTime += deltaTime;
        if (effectAnimationTime >= effectAnimationSpeed) {
            effectAnimationTime = 0.0f;
            currentEffectFrame++;
            if (currentEffectFrame >= effectColumns) {
                // Done dying
                state = DEAD;
            } else {
                effectSprite.setTextureRect(sf::IntRect(currentEffectFrame * effectFrameSize.x, 0, effectFrameSize.x, effectFrameSize.y));
            }
        }
        return; // Don't move or AI while dying
    }

    if (state == HURT) {
        // Apply knockback
        sprite.move(knockbackVelocity.x * deltaTime, knockbackVelocity.y * deltaTime);
        knockbackVelocity.x *= std::pow(0.01f, deltaTime); // friction
        knockbackVelocity.y *= std::pow(0.01f, deltaTime);
        
        // Ensure within map constraints roughly by freezing if moving too far
        
        // Handle Hurt Animation
        animationTime += deltaTime;
        if (animationTime >= animationSpeed * 0.7f) { // hurt frames play
            animationTime = 0.0f;
            currentFrameX++;
            if (currentFrameX >= columns) {
                // Done hurting, back to alive
                state = ALIVE;
                sprite.setTexture(texture);
                columns = baseColumns;
                currentFrameX = 0;
            } else {
                sprite.setTextureRect(sf::IntRect(currentFrameX * frameSize.x, currentFrameY * frameSize.y, frameSize.x, frameSize.y));
            }
        }
        return; 
    }

    sf::Vector2f curPos = sprite.getPosition();
    float dx = playerPos.x - curPos.x;
    float dy = playerPos.y - curPos.y;
    float distance = std::sqrt(dx * dx + dy * dy);

    bool moving = false;
    sf::Vector2f velocity(0.f, 0.f);
    
    if (type >= 1) {
        if (distance <= 60.0f && state == ALIVE) {
            state = ATTACKING;
            sprite.setTexture(attackTexture);
            columns = attackColumns;
            currentFrameX = 0;
            animationTime = 0.0f;
            
            // Immediately figure out direction before swinging
            if (std::abs(dx) > std::abs(dy)) {
                currentFrameY = (dx > 0) ? 3 : 2; 
            } else {
                currentFrameY = (dy > 0) ? 0 : 1; 
            }
            sprite.setTextureRect(sf::IntRect(currentFrameX * frameSize.x, currentFrameY * frameSize.y, frameSize.x, frameSize.y));
        }
    }

    if (state == ALIVE) {
        // Simple AI: follow player if distance > 0.5 and < 800
        if (distance > 0.5f && distance < 800.0f) {
            sf::Vector2f direction(dx / distance, dy / distance);
            velocity = direction * speed;
            
            // Apply X movement and check map collision
            sprite.move(velocity.x * deltaTime, 0);
            sf::FloatRect boundsAfterX = getGlobalBounds();
            for (const auto& obs : gameMap.getObstacles()) {
                if (boundsAfterX.intersects(obs)) {
                    sprite.move(-velocity.x * deltaTime, 0);
                    velocity.x = 0; 
                    break;
                }
            }

            // Apply Y movement and check map collision
            sprite.move(0, velocity.y * deltaTime);
            sf::FloatRect boundsAfterY = getGlobalBounds();
            for (const auto& obs : gameMap.getObstacles()) {
                if (boundsAfterY.intersects(obs)) {
                    sprite.move(0, -velocity.y * deltaTime);
                    velocity.y = 0; 
                    break;
                }
            }
            
            if (velocity.x != 0 || velocity.y != 0) moving = true;
        }
    }

    // Direction Mapping relies ONLY on movement for ALIVE
    if (moving && state == ALIVE) {
        if (type >= 1) {
            // Type >= 1 Mapping: 0=Down, 1=Up, 2=Left, 3=Right
            if (std::abs(dx) > std::abs(dy)) {
                currentFrameY = (dx > 0) ? 3 : 2; 
            } else {
                currentFrameY = (dy > 0) ? 0 : 1; 
            }
        } else {
            // Type 0 Mapping: 0=Up, 1=Left, 2=Down, 3=Right
            if (std::abs(dx) > std::abs(dy)) {
                currentFrameY = (dx > 0) ? 3 : 1; 
            } else {
                currentFrameY = (dy > 0) ? 2 : 0; 
            }
        }
    }

    updateAnimation(deltaTime, moving, velocity);
}

void Enemy::draw(sf::RenderWindow& window) {
    if (state == ALIVE || state == ATTACKING || state == HURT) {
        if (type == 0) { // Advanced enemies have baked-in shadows
            // Draw shadow first
            sf::FloatRect spriteBounds = sprite.getGlobalBounds();
            sf::CircleShape shadow(spriteBounds.width * 0.15f); // Tighter
            shadow.setOrigin(shadow.getRadius(), shadow.getRadius());
            shadow.setScale(1.5f, 0.8f);
            shadow.setFillColor(sf::Color(0, 0, 0, 120));
            // Push slightly upwards from 0.40f to perfectly grab feet
            shadow.setPosition(sprite.getPosition().x, sprite.getPosition().y + spriteBounds.height * 0.48f);
            window.draw(shadow);
        }

        window.draw(sprite);

        // DEBUG: Draw the underlying footprint / bounding box in transparent blue
        sf::FloatRect bounds = getGlobalBounds();
        sf::RectangleShape debugBox(sf::Vector2f(bounds.width, bounds.height));
        debugBox.setPosition(bounds.left, bounds.top);
        debugBox.setFillColor(sf::Color(0, 0, 255, 50)); // Translucent blue
        debugBox.setOutlineThickness(1.2f);
        debugBox.setOutlineColor(sf::Color::Blue);       // Solid blue edge
        window.draw(debugBox);
    } else if (state == SPAWNING) {
        // Draw shadow first
        sf::FloatRect spriteBounds = sprite.getGlobalBounds();
        sf::CircleShape shadow(spriteBounds.width * 0.15f); 
        shadow.setOrigin(shadow.getRadius(), shadow.getRadius());
        shadow.setScale(1.f, 0.4f);
        shadow.setFillColor(sf::Color(0, 0, 0, 120));
        shadow.setPosition(sprite.getPosition().x, sprite.getPosition().y + spriteBounds.height * 0.35f);
        window.draw(shadow);

        // Draw the skeleton immediately, but rooted in place
        window.draw(sprite);
        
        // Draw the spawning effect on top
        sf::Vector2f pos = sprite.getPosition();
        effectSprite.setPosition(pos);
        effectSprite.setOrigin(effectFrameSize.x / 2.0f, effectFrameSize.y / 2.0f - 25.0f);
        window.draw(effectSprite);
    } else if (state == DYING) {
        // Only draw the dying effect, hide the skeleton body
        sf::Vector2f pos = sprite.getPosition();
        effectSprite.setPosition(pos);
        effectSprite.setOrigin(effectFrameSize.x / 2.0f, effectFrameSize.y / 2.0f);
        window.draw(effectSprite);
    }
}

void Enemy::setPosition(float x, float y) {
    sprite.setPosition(x, y);
}

sf::Vector2f Enemy::getPosition() const {
    return sprite.getPosition();
}

sf::FloatRect Enemy::getGlobalBounds() const {
    sf::FloatRect bounds = sprite.getGlobalBounds();
    
    if (type >= 1) {
        // Advanced enemies are scaled 1.2x. We clamp down their movement footprint  
        // to a tiny pinpoint at their center-feet to ensure they effortlessly glide through corridors.
        return sf::FloatRect(
            bounds.left + bounds.width * 0.42f,       // Dead center horizontally
            bounds.top + bounds.height * 0.5f,       // Strictly bottom feet area
            bounds.width * 0.16f,                     // Tiniest width
            bounds.height * 0.12f                     // Tiniest depth height
        );
    }

    // Reduce bounds to the bottom 30% for realistic Z-plane perspective collisions
    return sf::FloatRect(
        bounds.left + bounds.width * 0.2f,        // Narrow left
        bounds.top + bounds.height * 0.7f,        // Push down to feet
        bounds.width * 0.6f,                      // Narrow width
        bounds.height * 0.3f                      // Shorten height to just the feet area
    );
}

sf::FloatRect Enemy::getDamageBounds() const {
    if (state == ATTACKING) {
        // Return full native bounds so sword impacts connect naturally
        return sprite.getGlobalBounds();
    }
    return getGlobalBounds();
}

bool Enemy::isDead() const {
    return state == DEAD;
}

int Enemy::getType() const {
    return type;
}

EnemyState Enemy::getState() const {
    return state;
}

void Enemy::kill() {
    if (state != DEAD && state != DYING) {
        state = DYING;
        currentEffectFrame = 0;
        effectAnimationTime = 0.0f;
        effectSprite.setTextureRect(sf::IntRect(0, 0, effectFrameSize.x, effectFrameSize.y));
    }
}

bool Enemy::takeDamage(int dmg, const sf::Vector2f& sourcePos) {
    if (state == DEAD || state == DYING || state == SPAWNING) return false;
    
    hp -= dmg;
    
    // Calculate knockback direction
    sf::Vector2f curPos = sprite.getPosition();
    float dx = curPos.x - sourcePos.x;
    float dy = curPos.y - sourcePos.y;
    float dist = std::sqrt(dx*dx + dy*dy);
    
    if (hp <= 0) {
        kill();
        return true; 
    }
    
    // Enter HURT state
    if (dist > 0) {
        sf::Vector2f dir(dx/dist, dy/dist);
        knockbackVelocity = dir * 250.0f; // Set a brief backward movement speed
    }
    
    if (type >= 1) {
        state = HURT;
        sprite.setTexture(hurtTexture);
        columns = hurtColumns;
        currentFrameX = 0;
        animationTime = 0.0f;
        
        // Keep facing player roughly during hurt
        if (std::abs(dx) > std::abs(dy)) {
            currentFrameY = (dx < 0) ? 3 : 2; 
        } else {
            currentFrameY = (dy < 0) ? 0 : 1; 
        }
        sprite.setTextureRect(sf::IntRect(currentFrameX * frameSize.x, currentFrameY * frameSize.y, frameSize.x, frameSize.y));
    }
    
    return false;
}

