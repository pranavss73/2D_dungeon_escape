#include "Bullet.h"
#include <cmath>

Bullet::Bullet(const sf::Vector2f& startPos, const sf::Vector2f& direction, float rotation) {
    speed = 600.0f;
    active = true;
    lifetime = 0.0f;
    maxLifetime = 3.0f; // seconds

    shape.setSize(sf::Vector2f(15.f, 4.f));
    shape.setFillColor(sf::Color(255, 255, 100)); // Bright yellow core
    shape.setOutlineThickness(1.5f);
    shape.setOutlineColor(sf::Color(255, 150, 0)); // Orange glow
    shape.setOrigin(7.5f, 2.f); // Center
    shape.setPosition(startPos);
    
    // SFML handles rotation in degrees. We calculate it from direction vector.
    float angle = std::atan2(direction.y, direction.x) * 180.f / 3.14159f;
    shape.setRotation(angle);
    
    // Normalize direction vector
    float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
    if (length != 0) {
        velocity = sf::Vector2f(direction.x / length, direction.y / length) * speed;
    } else {
        velocity = sf::Vector2f(0.f, 0.f);
    }
}

Bullet::~Bullet() {}

void Bullet::update(float deltaTime) {
    if (!active) return;
    
    shape.move(velocity * deltaTime);
    
    lifetime += deltaTime;
    if (lifetime >= maxLifetime) {
        active = false;
    }
}

void Bullet::draw(sf::RenderWindow& window) {
    if (active) {
        window.draw(shape);
    }
}

sf::FloatRect Bullet::getGlobalBounds() const {
    return shape.getGlobalBounds();
}

bool Bullet::isActive() const {
    return active;
}

void Bullet::destroy() {
    active = false;
}
