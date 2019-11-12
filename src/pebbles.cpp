#define _USE_MATH_DEFINES

// Header
#include "pebbles.hpp"

#include <cmath>
#include <iostream>

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// DON'T WORRY ABOUT THIS CLASS UNTIL ASSIGNMENT 3
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

static const int MAX_PEBBLES = 25;
static const float GRAVITY = 0.05f;
constexpr int NUM_SEGMENTS = 12;

bool Pebbles::init()
{
    std::vector<GLfloat> screen_vertex_buffer_data;
    constexpr float z = -0.1f;

    for (int i = 0; i < NUM_SEGMENTS; i++)
    {
        screen_vertex_buffer_data.push_back(std::cos(M_PI * 2.0 * float(i) / (float)NUM_SEGMENTS));
        screen_vertex_buffer_data.push_back(std::sin(M_PI * 2.0 * float(i) / (float)NUM_SEGMENTS));
        screen_vertex_buffer_data.push_back(z);

        screen_vertex_buffer_data.push_back(std::cos(M_PI * 2.0 * float(i + 1) / (float)NUM_SEGMENTS));
        screen_vertex_buffer_data.push_back(std::sin(M_PI * 2.0 * float(i + 1) / (float)NUM_SEGMENTS));
        screen_vertex_buffer_data.push_back(z);

        screen_vertex_buffer_data.push_back(0);
        screen_vertex_buffer_data.push_back(0);
        screen_vertex_buffer_data.push_back(z);
    }

    // Clearing errors
    gl_flush_errors();

    // Vertex Buffer creation
    glGenBuffers(1, &mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, screen_vertex_buffer_data.size() * sizeof(GLfloat), screen_vertex_buffer_data.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &m_instance_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_instance_vbo);

    if (gl_has_errors())
        return false;

    // Loading shaders
    if (!effect.load_from_file(shader_path("pebble.vs.glsl"), shader_path("pebble.fs.glsl")))
        return false;

    return true;
}

// Releases all graphics resources
void Pebbles::destroy()
{
    glDeleteBuffers(1, &mesh.vbo);
    glDeleteBuffers(1, &m_instance_vbo);

    glDeleteShader(effect.vertex);
    glDeleteShader(effect.fragment);
    glDeleteShader(effect.program);

    m_pebbles.clear();
}

void Pebbles::update(float ms)
{
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // HANDLE PEBBLE UPDATES HERE
    // You will need to handle both the motion of pebbles
    // and the removal of dead pebbles.
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    auto pebble_it = m_pebbles.begin();
    while (pebble_it != m_pebbles.end())
    {
        float x = pebble_it->position.x;
        float y = pebble_it->position.y;
        if (x < -10.f || x > 1300.f || y > 900.f)
        {
            pebble_it = m_pebbles.erase(pebble_it);
        }
        else
        {
            pebble_it->velocity.y += GRAVITY * ms * 0.1f;
            pebble_it->position = add(pebble_it->position, pebble_it->velocity);
            ++pebble_it;
        }
    }
}

void Pebbles::spawn_pebble(vec2 position, vec2 velocity, float radius)
{
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // HANDLE PEBBLE SPAWNING HERE
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    if (m_pebbles.size() <= MAX_PEBBLES)
    {
        Pebble pebble;
        pebble.radius = radius;
        pebble.position = position;
        pebble.velocity = velocity;

        m_pebbles.emplace_back(pebble);
    }
}

void Pebbles::collides_with_pebbles()
{
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // HANDLE PEBBLE COLLISIONS HERE
    // You will need to write additional functions from scratch.
    // Make sure to handle both collisions between pebbles
    // and collisions between pebbles and salmon/fish/turtles.
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    int i = 0;
    for (auto &p1 : m_pebbles)
    {
        int j = 0;
        for (auto &p2 : m_pebbles)
        {
            if (i == j)
                continue;

            float x_diff = p1.position.x - p2.position.x;
            float y_diff = p1.position.y - p2.position.y;

            float d = len({x_diff, y_diff});

            if (d < p1.radius + p2.radius)
            {
                // for p1
                float p1_dist = d - p1.radius;
                p1.position.x = p1_dist * x_diff / d;
                p1.position.y = p1_dist * y_diff / d;
                // for p2
                float p2_dist = d - p2.radius;
                p2.position.x = p2_dist * x_diff / d;
                p2.position.y = p2_dist * y_diff / d;

                // for collision
                float m1 = p1.radius;
                float m2 = p2.radius;

                vec2 p1_init_v = p1.velocity;
                vec2 p2_init_v = p2.velocity;

                p1.velocity = add(mul(p1_init_v, (m1 - m2) / (m1 + m2)), mul(p2_init_v, 2.f * m2 / (m1 + m2)));
                p2.velocity = add(mul(p2_init_v, (m2 - m1) / (m1 + m2)), mul(p1_init_v, 2.f * m1 / (m1 + m2)));
                break;
            }
            j++;
        }
        i++;
    }
}

void Pebbles::collides_with(const Turtle &turtle)
{
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // HANDLE PEBBLE COLLISIONS HERE
    // You will need to write additional functions from scratch.
    // Make sure to handle both collisions between pebbles
    // and collisions between pebbles and salmon/fish/turtles.
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    for (auto &pebble : m_pebbles)
    {
        // collision check for turtle
        vec2 t_pos = turtle.get_position();
        vec2 t_bbox = turtle.get_bounding_box();

        float t_left = t_pos.x - t_bbox.x * 0.5f;
        float t_right = t_pos.x + t_bbox.x * 0.5f;
        float t_top = t_pos.y - t_bbox.y * 0.5f;
        float t_bottom = t_pos.y + t_bbox.y * 0.5f;

        vec2 p_pos = pebble.position;
        float p_left = p_pos.x - pebble.radius;
        float p_right = p_pos.x + pebble.radius;
        float p_top = p_pos.y - pebble.radius;
        float p_bottom = p_pos.y + pebble.radius;

        if (p_pos.x >= t_left && p_pos.x <= t_right)
        {
            if (p_pos.y < t_top && p_bottom >= t_top)
            {
                pebble.velocity.y = -fabs(pebble.velocity.y);
            }
            if (p_pos.y > t_bottom && p_top <= t_bottom)
            {
                pebble.velocity.y = fabs(pebble.velocity.y);
            }
        }

        if (p_pos.y >= t_top && p_pos.y <= t_bottom)
        {
            if (p_pos.x < t_left && p_right >= t_left)
            {
                pebble.velocity.x = -fabs(pebble.velocity.x);
            }
            if (p_pos.x > t_right && p_left <= t_right)
            {
                pebble.velocity.x = fabs(pebble.velocity.x);
            }
        }
    }
}

void Pebbles::collides_with(const Fish &fish)
{
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // HANDLE PEBBLE COLLISIONS HERE
    // You will need to write additional functions from scratch.
    // Make sure to handle both collisions between pebbles
    // and collisions between pebbles and salmon/fish/turtles.
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    for (auto &pebble : m_pebbles)
    {
        // collision check for turtle
        vec2 f_pos = fish.get_position();
        vec2 f_bbox = fish.get_bounding_box();

        float f_left = f_pos.x - f_bbox.x * 0.5f;
        float f_right = f_pos.x + f_bbox.x * 0.5f;
        float f_top = f_pos.y - f_bbox.y * 0.5f;
        float f_bottom = f_pos.y + f_bbox.y * 0.5f;

        vec2 p_pos = pebble.position;
        float p_left = p_pos.x - pebble.radius;
        float p_right = p_pos.x + pebble.radius;
        float p_top = p_pos.y - pebble.radius;
        float p_bottom = p_pos.y + pebble.radius;

        if (p_pos.x >= f_left && p_pos.x <= f_right)
        {
            if (p_pos.y < f_top && p_bottom >= f_top)
            {
                pebble.velocity.y = -fabs(pebble.velocity.y);
            }
            if (p_pos.y > f_bottom && p_top <= f_bottom)
            {
                pebble.velocity.y = fabs(pebble.velocity.y);
            }
        }

        if (p_pos.y >= f_top && p_pos.y <= f_bottom)
        {
            if (p_pos.x < f_left && p_right >= f_left)
            {
                pebble.velocity.x = -fabs(pebble.velocity.x);
            }
            if (p_pos.x > f_right && p_left <= f_right)
            {
                pebble.velocity.x = fabs(pebble.velocity.x);
            }
        }
    }
}

void Pebbles::collides_with(const Salmon &salmon)
{
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // HANDLE PEBBLE COLLISIONS HERE
    // You will need to write additional functions from scratch.
    // Make sure to handle both collisions between pebbles
    // and collisions between pebbles and salmon/fish/turtles.
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    for (auto &pebble : m_pebbles)
    {
        // collision check for turtle
        vec2 s_pos = salmon.get_position();
        vec2 s_bbox = salmon.get_bounding_box();

        float s_left = s_pos.x - s_bbox.x * 0.5f;
        float s_right = s_pos.x + s_bbox.x * 0.5f;
        float s_top = s_pos.y - s_bbox.y * 0.5f;
        float s_bottom = s_pos.y + s_bbox.y * 0.5f;

        vec2 p_pos = pebble.position;
        float p_left = p_pos.x - pebble.radius;
        float p_right = p_pos.x + pebble.radius;
        float p_top = p_pos.y - pebble.radius;
        float p_bottom = p_pos.y + pebble.radius;

        if (p_pos.x >= s_left && p_pos.x <= s_right)
        {
            if (p_pos.y < s_top && p_bottom >= s_top)
            {
                pebble.velocity.y = -fabs(pebble.velocity.y);
            }
            if (p_pos.y > s_bottom && p_top <= s_bottom)
            {
                pebble.velocity.y = fabs(pebble.velocity.y);
            }
        }

        if (p_pos.y >= s_top && p_pos.y <= s_bottom)
        {
            if (p_pos.x < s_left && p_right >= s_left)
            {
                pebble.velocity.x = -fabs(pebble.velocity.x);
            }
            if (p_pos.x > s_right && p_left <= s_right)
            {
                pebble.velocity.x = fabs(pebble.velocity.x);
            }
        }
    }
}

// Draw pebbles using instancing
void Pebbles::draw(const mat3 &projection)
{
    // Setting shaders
    glUseProgram(effect.program);

    // Enabling alpha channel for textures
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);

    // Getting uniform locations
    GLint projection_uloc = glGetUniformLocation(effect.program, "projection");
    GLint color_uloc = glGetUniformLocation(effect.program, "color");

    // Pebble color
    float color[] = {0.4f, 0.4f, 0.4f};
    glUniform3fv(color_uloc, 1, color);
    glUniformMatrix3fv(projection_uloc, 1, GL_FALSE, (float *)&projection);

    // Draw the screen texture on the geometry
    // Setting vertices
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);

    // Mesh vertex positions
    // Bind to attribute 0 (in_position) as in the vertex shader
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
    glVertexAttribDivisor(0, 0);

    // Load up pebbles into buffer
    glBindBuffer(GL_ARRAY_BUFFER, m_instance_vbo);
    glBufferData(GL_ARRAY_BUFFER, m_pebbles.size() * sizeof(Pebble), m_pebbles.data(), GL_DYNAMIC_DRAW);

    // Pebble translations
    // Bind to attribute 1 (in_translate) as in vertex shader
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Pebble), (GLvoid *)offsetof(Pebble, position));
    glVertexAttribDivisor(1, 1);

    // Pebble radii
    // Bind to attribute 2 (in_scale) as in vertex shader
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(Pebble), (GLvoid *)offsetof(Pebble, radius));
    glVertexAttribDivisor(2, 1);

    // Draw using instancing
    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glDrawArraysInstanced.xhtml
    glDrawArraysInstanced(GL_TRIANGLES, 0, NUM_SEGMENTS * 3, m_pebbles.size());

    // Reset divisor
    glVertexAttribDivisor(1, 0);
    glVertexAttribDivisor(2, 0);
}