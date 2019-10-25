#pragma once

#include "common.hpp"
#include "dot.hpp"
#include <vector>

// Salmon food
class Fish : public Entity
{
    // Shared between all fish, no need to load one for each instance
    static Texture fish_texture;

private:
    std::vector<vec3> m_init_pos;
    std::vector<Dot> m_path;

    vec2 direction;

    float move_distance;

    float horizontal_distance;
    float vertical_distance;
    vec2 vertical_direction;
    float combined_distance;
    vec2 combined_direction;

public:
    // Creates all the associated render resources and default transform
    bool init();

    // Releases all the associated resources
    void destroy();

    // Update fish
    // ms represents the number of milliseconds elapsed from the previous update() call
    void update(float ms, vec2 salmon_pos);

    void update_path(float ms, vec2 salmon_pos);

    void update_path_coordiante();

    // Renders the fish
    // projection is the 2D orthographic projection matrix
    void draw(const mat3 &projection) override;

    // Returns the current fish position
    vec2 get_position() const;

    // Sets the new fish position
    void set_position(vec2 position);

    // Returns the fish' bounding box for collision detection, called by collides_with()
    vec2 get_bounding_box() const;

    void get_init_pos();

    vec2 get_close_pos(vec2 salmon_pos);
};