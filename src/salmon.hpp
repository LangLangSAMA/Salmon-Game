#pragma once

#include "common.hpp"
#include <vector>

class Turtle;
class Fish;

enum Boundary
{
    TOP,
    BOTTOM,
    LEFT,
    RIGHT,
    EMPTY
};

class Salmon : public Entity
{
public:
    // Creates all the associated render resources and default transform
    bool init();

    // Releases all associated resources
    void destroy();

    // Update salmon position based on direction
    // ms represents the number of milliseconds elapsed from the previous update() call
    void update(float ms);

    void reflect();

    // Renders the salmon
    void draw(const mat3 &projection) override;

    // Collision routines for turtles and fish
    bool collides_with(const Turtle &turtle);
    bool collides_with(const Fish &fish);
    bool collides_with_wall();
    void collision_check();

    void predict_collision();

    // Returns the current salmon position
    vec2 get_position() const;

    // Moves the salmon's position by the specified offset
    void move(vec2 off);

    // Set salmon rotation in radians
    void set_rotation(float radians);

    // True if the salmon is alive
    bool is_alive() const;

    // Kills the salmon, changing its alive state and triggering on death events
    void kill();

    // Called when the salmon collides with a fish, starts lighting up the salmon
    void light_up();

    // Called when salmon eats a fish
    void increase_size();

    // Called when pressed X to toggle advanced mode
    void advanced_mode();

    vec2 get_collision_point();

    // booleans determine salmon movement
    bool move_up;
    bool move_down;
    bool move_left;
    bool move_right;

    // boolean toggle modes
    bool advancedMode;

private:
    float m_light_up_countdown_ms; // Used to keep track for how long the salmon should be lit up
    bool m_is_alive;               // True if the salmon is alive

    vec2 collision_point;
    vec2 collision_top;
    vec2 collision_bottom;
    vec2 collision_left;
    vec2 collision_right;
    bool collision_flag;

    Boundary boundary;

    std::vector<Vertex> m_vertices;
    std::vector<uint16_t> m_indices;
};
