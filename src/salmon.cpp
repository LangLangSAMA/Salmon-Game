// Header
#include "salmon.hpp"

// internal
#include "turtle.hpp"
#include "fish.hpp"

// stlib
#include <string>
#include <algorithm>

// math
#include <math.h>
#define PI 3.14159265

// Testing
#include <iostream>
#include <iomanip>
using std::cout;
using std::endl;

bool Salmon::init()
{
    m_vertices.clear();
    m_indices.clear();

    // Reads the salmon mesh from a file, which contains a list of vertices and indices
    FILE *mesh_file = fopen(mesh_path("salmon.mesh"), "r");
    if (mesh_file == nullptr)
        return false;

    // Reading vertices and colors
    size_t num_vertices;
    fscanf(mesh_file, "%zu\n", &num_vertices);
    for (size_t i = 0; i < num_vertices; ++i)
    {
        float x, y, z;
        float _u[3]; // unused
        int r, g, b;
        fscanf(mesh_file, "%f %f %f %f %f %f %d %d %d\n", &x, &y, &z, _u, _u + 1, _u + 2, &r, &g, &b);
        Vertex vertex;
        vertex.position = {x, y, -z};
        vertex.color = {(float)r / 255, (float)g / 255, (float)b / 255};
        m_vertices.push_back(vertex);
    }

    // Reading associated indices
    size_t num_indices;
    fscanf(mesh_file, "%zu\n", &num_indices);
    for (size_t i = 0; i < num_indices; ++i)
    {
        int idx[3];
        fscanf(mesh_file, "%d %d %d\n", idx, idx + 1, idx + 2);
        m_indices.push_back((uint16_t)idx[0]);
        m_indices.push_back((uint16_t)idx[1]);
        m_indices.push_back((uint16_t)idx[2]);
    }

    // Done reading
    fclose(mesh_file);

    // Clearing errors
    gl_flush_errors();

    // Vertex Buffer creation
    glGenBuffers(1, &mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * m_vertices.size(), m_vertices.data(), GL_STATIC_DRAW);

    // Index Buffer creation
    glGenBuffers(1, &mesh.ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint16_t) * m_indices.size(), m_indices.data(), GL_STATIC_DRAW);

    // Vertex Array (Container for Vertex + Index buffer)
    glGenVertexArrays(1, &mesh.vao);
    if (gl_has_errors())
        return false;

    // Loading shaders
    if (!effect.load_from_file(shader_path("salmon.vs.glsl"), shader_path("salmon.fs.glsl")))
        return false;

    // Setting initial values
    motion.position = {200.f, 400.f};
    motion.radians = 0.f;
    motion.speed = 250.f;
    advancedMode = false;

    if (advancedMode)
    {
        physics.scale = {-10.f, 10.f};
    }
    else
    {
        physics.scale = {-35.f, 35.f};
    }

    m_is_alive = true;
    collision_flag = false;
    m_light_up_countdown_ms = -1.f;

    boundary = EMPTY;

    vec3 collision_point = {-1.f, -1.f, -1.f};

    return true;
}

// Releases all graphics resources
void Salmon::destroy()
{
    glDeleteBuffers(1, &mesh.vbo);
    glDeleteBuffers(1, &mesh.ibo);
    glDeleteBuffers(1, &mesh.vao);

    glDeleteShader(effect.vertex);
    glDeleteShader(effect.fragment);
    glDeleteShader(effect.program);
}

// Called on each frame by World::update()
void Salmon::update(float ms)
{
    float step = motion.speed * (ms / 1000);
    if (m_is_alive)
    {
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // UPDATE SALMON POSITION HERE BASED ON KEY PRESSED (World::on_key())
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        if (advancedMode)
        // advanced mode, fish moving along the direciton it is aligned with
        {
            if (move_left)
                motion.radians -= 0.1f;
            if (move_right)
                motion.radians += 0.1f;
            if (move_up)
                move({step * cos(motion.radians), step * sin(motion.radians)});
            if (move_down)
                move({-step * cos(motion.radians), -step * sin(motion.radians)});
        }
        else
        // simple mode
        {
            if (move_left)
                motion.radians -= 0.1f;
            if (move_right)
                motion.radians += 0.1f;

            if (move_up)
            {

                move({step * cos(motion.radians), step * sin(motion.radians)});
            }
            if (move_down)
            {
                move({-step * cos(motion.radians), -step * sin(motion.radians)});
            }
        }
    }
    else
    {
        // If dead we make it face upwards and sink deep down
        set_rotation(PI);
        move({0.f, step});
    }

    if (m_light_up_countdown_ms > 0.f)
        m_light_up_countdown_ms -= ms;
}

void Salmon::reflect()
{
    float displacement = 50.f;
    switch (boundary)
    {
    case TOP:
    case BOTTOM:
        motion.radians = -motion.radians;
        break;
    case LEFT:
    case RIGHT:
        if (motion.radians < 0)
        {
            motion.radians = -motion.radians - PI;
        }
        else
        {
            motion.radians = -motion.radians + PI;
        }
        break;
    default:
        break;
    }

    switch (boundary)
    {
    case TOP:
        move({0.f, displacement});
        break;
    case BOTTOM:
        move({0.f, -displacement});
        break;
    case LEFT:
        move({displacement, 0.f});
        break;
    case RIGHT:
        move({-displacement, 0.f});
        break;
    default:
        break;
    }
}

void Salmon::predict_collision()
{
    float x_direction = cosf(motion.radians);
    float y_direction = -sinf(motion.radians);

    if (move_down)
    {
        x_direction = -x_direction;
        y_direction = -y_direction;
    }

    float c_x, c_y, x, y;

    if (y_direction > 0)
    {
        c_x = collision_top.x;
        c_y = collision_top.y;
        x = c_x + cosf(motion.radians) / sinf(motion.radians) * (50 - c_y);

        if (x >= 50.f && x <= 1100.f)
        {
            collision_point = {x, 50};
        }
    }
    if (y_direction < 0)
    {
        c_x = collision_bottom.x;
        c_y = collision_bottom.y;
        x = c_x + cosf(motion.radians) / sinf(motion.radians) * (750 - c_y);

        if (x >= 50.f && x <= 1100.f)
        {
            collision_point = {x, 750};
        }
    }
    if (x_direction < 0)
    {
        c_x = collision_left.x;
        c_y = collision_left.y;
        y = c_y + sinf(motion.radians) / cosf(motion.radians) * (50 - c_x);

        if (y >= 50.f && y <= 750.f)
        {
            collision_point = {50, y};
        }
    }
    if (x_direction > 0)
    {
        c_x = collision_right.x;
        c_y = collision_right.y;
        y = c_y + sinf(motion.radians) / cosf(motion.radians) * (1150 - c_x);

        if (y >= 50.f && y <= 750.f)
        {
            collision_point = {1150, y};
        }
    }
}

void Salmon::draw(const mat3 &projection)
{
    transform.begin();

    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // SALMON TRANSFORMATION CODE HERE

    // see Transformations and Rendering in the specification pdf
    // the following functions are available:
    // translate()
    // rotate()
    // scale()

    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // REMOVE THE FOLLOWING LINES BEFORE ADDING ANY TRANSFORMATION CODE
    transform.translate(motion.position);
    transform.rotate(motion.radians);
    transform.scale(physics.scale);
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    transform.end();

    // Setting shaders
    glUseProgram(effect.program);

    // Enabling alpha channel for textures
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);

    // Getting uniform locations
    GLint transform_uloc = glGetUniformLocation(effect.program, "transform");
    GLint color_uloc = glGetUniformLocation(effect.program, "fcolor");
    GLint projection_uloc = glGetUniformLocation(effect.program, "projection");
    GLint light_up_uloc = glGetUniformLocation(effect.program, "light_up");

    // Setting vertices and indices
    glBindVertexArray(mesh.vao);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ibo);

    // Input data location as in the vertex buffer
    GLint in_position_loc = glGetAttribLocation(effect.program, "in_position");
    GLint in_color_loc = glGetAttribLocation(effect.program, "in_color");
    glEnableVertexAttribArray(in_position_loc);
    glEnableVertexAttribArray(in_color_loc);
    glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0);
    glVertexAttribPointer(in_color_loc, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)sizeof(vec3));

    // Setting uniform values to the currently bound program
    glUniformMatrix3fv(transform_uloc, 1, GL_FALSE, (float *)&transform.out);

    // !!! Salmon Color
    float color[] = {1.f, 1.f, 1.f};
    glUniform3fv(color_uloc, 1, color);
    glUniformMatrix3fv(projection_uloc, 1, GL_FALSE, (float *)&projection);

    // Get killed by turtle
    if (!is_alive())
    {
        float death_color[] = {1.f, 0.5f, 0.5f};
        glUniform3fv(color_uloc, 1, death_color);
    }

    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // HERE TO SET THE CORRECTLY LIGHT UP THE SALMON IF HE HAS EATEN RECENTLY
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    int light_up = 0;
    if (m_light_up_countdown_ms > 0.f)
        light_up = 1;
    glUniform1iv(light_up_uloc, 1, &light_up);

    // Get number of infices from buffer,
    // we know our vbo contains both colour and position information, so...
    GLint size = 0;
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
    GLsizei num_indices = size / sizeof(uint16_t);

    // Drawing!
    glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_SHORT, nullptr);
}

void Salmon::collision_check()
{
    vec2 top;
    vec2 bottom;
    vec2 left;
    vec2 right;
    float x_left = 99999.f;
    float x_right = -99999.f;
    float y_top = 99999.f;
    float y_bottom = -99999.f;
    for (size_t i = 0; i < m_vertices.size(); i++)
    {
        Vertex vertex;
        vertex = m_vertices[i];
        vec3 point = {vertex.position.x, vertex.position.y, 1};
        vec3 result = mul(transform.out, point);
        float x = result.x;
        float y = result.y;
        if (x < x_left)
        {
            x_left = x;
            left = {x, y};
        }
        if (x > x_right)
        {
            x_right = x;
            right = {x, y};
        }
        if (y < y_top)
        {
            y_top = y;
            top = {x, y};
        }
        if (y > y_bottom)
        {
            y_bottom = y;
            bottom = {x, y};
        }
    }

    collision_top = top;
    collision_bottom = bottom;
    collision_left = left;
    collision_right = right;

    boundary = EMPTY;

    if (y_top < 50)
    {
        boundary = TOP;
        collision_point = {top.x, top.y};
    }
    else if (y_bottom > 750)
    {
        boundary = BOTTOM;
        collision_point = {bottom.x, bottom.y};
    }
    else if (x_left < 50)
    {
        boundary = LEFT;
        collision_point = {left.x, left.y};
    }
    else if (x_right > 1150)
    {
        boundary = RIGHT;
        collision_point = {right.x, right.y};
    }
}

bool Salmon::collides_with_wall()
{
    bool is_collided = false;

    if (m_is_alive)
    {
        collision_check();

        if (!collision_flag)
        {
            collision_flag = true;
            return false;
        }

        if (boundary != EMPTY)
        {
            is_collided = true;
        }
    }

    return is_collided;
}

// Simple bounding box collision check
// This is a SUPER APPROXIMATE check that puts a circle around the bounding boxes and sees
// if the center point of either object is inside the other's bounding-box-circle. You don't
// need to try to use this technique.
bool Salmon::collides_with(const Turtle &turtle)
{
    float dx = motion.position.x - turtle.get_position().x;
    float dy = motion.position.y - turtle.get_position().y;
    float d_sq = dx * dx + dy * dy;
    float other_r = std::max(turtle.get_bounding_box().x, turtle.get_bounding_box().y);
    float my_r = std::max(physics.scale.x, physics.scale.y);
    float r = std::max(other_r, my_r);
    r *= 0.6f;
    if (d_sq < r * r)
        return true;
    return false;
}

bool Salmon::collides_with(const Fish &fish)
{
    float dx = motion.position.x - fish.get_position().x;
    float dy = motion.position.y - fish.get_position().y;
    float d_sq = dx * dx + dy * dy;
    float other_r = std::max(fish.get_bounding_box().x, fish.get_bounding_box().y);
    float my_r = std::max(physics.scale.x, physics.scale.y);
    float r = std::max(other_r, my_r);
    r *= 0.6f;
    if (d_sq < r * r)
        return true;
    return false;
}

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// HANDLE SALMON - WALL COLLISIONS HERE
// DON'T WORRY ABOUT THIS UNTIL ASSIGNMENT 2
// You will want to write new functions from scratch for checking/handling
// salmon - wall collisions.
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

vec2 Salmon::get_position() const
{
    return motion.position;
}

void Salmon::move(vec2 off)
{
    motion.position.x += off.x;
    motion.position.y += off.y;
}

void Salmon::set_rotation(float radians)
{
    motion.radians = radians;
}

bool Salmon::is_alive() const
{
    return m_is_alive;
}

// Called when the salmon collides with a turtle
void Salmon::kill()
{
    m_is_alive = false;
}

// Called when the salmon collides with a fish
void Salmon::light_up()
{
    if (advancedMode)
    // advanced mode: increase size after eating a fish
    {
        increase_size();
    }
    m_light_up_countdown_ms = 1500.f;
}

// Increase the Salmon size
void Salmon::increase_size()
{
    physics.scale.x -= 5.f;
    physics.scale.y += 5.f;
}

// Toggle advanced mode, reset physical size
void Salmon::advanced_mode()
{
    advancedMode = !advancedMode;
    if (advancedMode)
    {
        physics.scale = {-10.f, 10.f};
    }
    else
    {
        physics.scale = {-35.f, 35.f};
    }
}

vec2 Salmon::get_collision_point()
{
    return collision_point;
}

float Salmon::get_rotation()
{
    return motion.radians;
}