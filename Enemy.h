#ifndef ENEMY_H
#define ENEMY_H

#include <SFML/Graphics.hpp>

#include "Map.h"

enum EnemyState {
    SPAWNING,
    ALIVE,
    ATTACKING,
    HURT,
    DYING,
    DEAD
};

class Enemy {
public:
    Enemy();
    ~Enemy();

    bool load(const std::string& texturePath, int columns, int rows);
    bool loadAdvancedEnemy(int t);
    void update(float deltaTime, const sf::Vector2f& playerPos, const Map& gameMap);
    void draw(sf::RenderWindow& window);

    void setPosition(float x, float y);
    sf::Vector2f getPosition() const;
    sf::FloatRect getGlobalBounds() const;
    sf::FloatRect getDamageBounds() const;
    int getType() const;

    bool isDead() const;
    void kill();
    bool takeDamage(int dmg, const sf::Vector2f& sourcePos);
    EnemyState getState() const;

private:
    sf::Texture texture;
    sf::Sprite sprite;
    
    sf::Texture effectTexture;
    sf::Sprite effectSprite;

    sf::Texture attackTexture;
    sf::Texture hurtTexture;
    int attackColumns;
    int hurtColumns;
    int baseColumns;

    int type;
    int hp;
    sf::Vector2f knockbackVelocity;

    float speed;
    EnemyState state;

    // Animation specific
    int columns;
    int rows;
    int currentFrameX;
    int currentFrameY;
    float animationTime;
    float animationSpeed;
    sf::Vector2i frameSize;

    int effectColumns;
    int currentEffectFrame;
    float effectAnimationTime;
    float effectAnimationSpeed;
    sf::Vector2i effectFrameSize;

    void updateAnimation(float deltaTime, bool moving, const sf::Vector2f& velocity);
};

#endif // ENEMY_H
