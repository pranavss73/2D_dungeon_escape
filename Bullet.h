#ifndef BULLET_H
#define BULLET_H

#include <SFML/Graphics.hpp>

class Bullet {
public:
    Bullet(const sf::Vector2f& startPos, const sf::Vector2f& direction, float rotation);
    ~Bullet();

    void update(float deltaTime);
    void draw(sf::RenderWindow& window);

    sf::FloatRect getGlobalBounds() const;
    bool isActive() const;
    void destroy();

private:
    sf::RectangleShape shape;
    sf::Vector2f velocity;
    float speed;
    bool active;
    float lifetime;
    float maxLifetime;
};

#endif // BULLET_H
