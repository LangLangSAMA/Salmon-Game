// Header
#include "fish.hpp"

#include <cmath>

Texture Fish::fish_texture;

bool Fish::init(bool m_debug)
{
    // Load shared texture
    if (!fish_texture.is_valid())
    {
        if (!fish_texture.load_from_file(textures_path("fish.png")))
        {
            fprintf(stderr, "Failed to load turtle texture!");
            return false;
        }
    }

    // The position corresponds to the center of the texture.
    float wr = fish_texture.width * 0.5f;
    float hr = fish_texture.height * 0.5f;

    TexturedVertex vertices[4];
    vertices[0].position = {-wr, +hr, -0.01f};
    vertices[0].texcoord = {0.f, 1.f};
    vertices[1].position = {+wr, +hr, -0.01f};
    vertices[1].texcoord = {
        1.f,
        1.f,
    };
    vertices[2].position = {+wr, -hr, -0.01f};
    vertices[2].texcoord = {1.f, 0.f};
    vertices[3].position = {-wr, -hr, -0.01f};
    vertices[3].texcoord = {0.f, 0.f};

    // Counterclockwise as it's the default opengl front winding direction.
    uint16_t indices[] = {0, 3, 1, 1, 3, 2};

    // Clearing errors
    gl_flush_errors();

    // Vertex Buffer creation
    glGenBuffers(1, &mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(TexturedVertex) * 4, vertices, GL_STATIC_DRAW);

    // Index Buffer creation
    glGenBuffers(1, &mesh.ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint16_t) * 6, indices, GL_STATIC_DRAW);

    // Vertex Array (Container for Vertex + Index buffer)
    glGenVertexArrays(1, &mesh.vao);
    if (gl_has_errors())
        return false;

    // Loading shaders
    if (!effect.load_from_file(shader_path("textured.vs.glsl"), shader_path("textured.fs.glsl")))
        return false;

    motion.radians = 0.f;
    motion.speed = 380.f;

    // Setting initial values, scale is negative to make it face the opposite way
    // 1.0 would be as big as the original texture.
    physics.scale = {-0.4f, 0.4f};

    for (int i = 0; i < 1500; i += 60)
    {
        Dot dot;
        if (dot.init(PATH))
        {
            m_path.push_back(dot);
        }
    }

    horizontal_distance = 0.f;
    horizontal_directon = {1.f, 0.f};
    vertical_distance = 0.f;
    vertical_direction = {0.f, 1.f};
    combined_distance = 0.f;
    combined_direction = {1.f, 0.f};

    debug = m_debug;

    get_init_pos();

    return true;
}

// Releases all graphics resources
void Fish::destroy()
{
    for (auto &dot : m_path)
        dot.destroy();
    m_path.clear();
    glDeleteBuffers(1, &mesh.vbo);
    glDeleteBuffers(1, &mesh.ibo);
    glDeleteBuffers(1, &mesh.vao);

    glDeleteShader(effect.vertex);
    glDeleteShader(effect.fragment);
    glDeleteShader(effect.program);
}

void Fish::get_init_pos()
{
    float x = fish_texture.width / 2;
    float y = fish_texture.height / 2;

    m_init_pos.push_back({-x, 0, 1.f});
    m_init_pos.push_back({-x / 2, -y, 1.f});
    m_init_pos.push_back({-x / 2, y, 1.f});
    m_init_pos.push_back({0, y, 1.f});
    m_init_pos.push_back({x / 2, -y, 1.f});
    m_init_pos.push_back({x / 2, y, 1.f});
    m_init_pos.push_back({0, -y, 1.f});
    m_init_pos.push_back({x, 0, 1.f});
}

vec2 Fish::get_close_pos(vec2 salmon_pos)
{
    vec2 result;
    float val = 9999999.f;
    for (int i = 0; i < m_init_pos.size(); i++)
    {
        vec3 pos_transformed = mul(transform.out, m_init_pos[i]);
        vec2 pos = {pos_transformed.x, pos_transformed.y};
        vec2 pos_diff = sub(salmon_pos, pos);
        float temp = dot(pos_diff, pos_diff);
        if (temp < val)
        {
            val = temp;
            result = pos_diff;
        }
    }

    return result;
}

void Fish::update(float ms)
{
    // Move fish along -X based on how much time has passed, this is to (partially) avoid
    // having entities move at different speed based on the machine.
    float step = 1.0 * motion.speed * (ms / 1000);

    vec2 pos = motion.position;

    if (horizontal_distance > 0)
    {
        pos = sub(pos, mul(horizontal_directon, step));
        horizontal_distance -= step;
    }
    else if (vertical_distance > 0)
    {
        pos = sub(pos, mul(vertical_direction, step));
        vertical_distance -= step;
    }
    else if (combined_distance > 0)
    {
        pos = sub(pos, mul(combined_direction, step));
        combined_distance -= step;
    }
    else
    {
        pos = sub(pos, {step, 0.f});
    }

    motion.position = pos;
}

void Fish::update_path(float ms, vec2 salmon_pos)
{
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // HANDLE FISH AI HERE
    // DON'T WORRY ABOUT THIS UNTIL ASSIGNMENT 2
    // You will likely want to write new functions and need to create
    // new data structures to implement a more sophisticated Fish AI.
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    float step = 1.0 * motion.speed * (ms / 1000);

    vec2 pos_diff = get_close_pos(salmon_pos);
    float x = pos_diff.x;
    float y = pos_diff.y;

    if (abs(y) < 150)
    {
        if (x < 0)
        {
            if (y >= 0)
            {
                vertical_direction = {0.f, 1.f};
            }
            else
            {
                vertical_direction = {0.f, -1.f};
            }
            vertical_distance = 150 - abs(y);
            horizontal_distance = abs(x) - 150;
        }
        else
        {
            combined_direction = normalize(pos_diff);
            combined_distance = 150 - sqrt(dot(pos_diff, pos_diff));
        }
    }
    else
    {
        vertical_distance = 0.f;
        horizontal_distance = 0.f;
        combined_distance = 0.f;
    }
}

void Fish::update_path_coordiante()
{
    float horizontal_it = 0.f;
    float vertical_it = 0.f;
    float combined_it = 0.f;

    vec2 pos = motion.position;

    for (int i = m_path.size() - 1; i >= 0; i--)
    {
        if (horizontal_it < horizontal_distance)
        {
            pos = sub(pos, mul(horizontal_directon, 60.f));
            horizontal_it += 60;
        }
        else if (vertical_it < vertical_distance)
        {
            if (vertical_distance < 60.f)
            {
                pos = sub(pos, mul(vertical_direction, vertical_distance));
            }
            else
            {
                pos = sub(pos, mul(vertical_direction, 60));
            }
            vertical_it += 60;
        }
        else if (combined_it < combined_distance)
        {
            pos = sub(pos, mul(combined_direction, 60));

            combined_it += 60;
        }
        else
        {
            pos = add(pos, {-60.f, 0.f});
        }
        m_path[i].update(pos);
    }
}

void Fish::draw(const mat3 &projection)
{
    if (debug)
    {
        for (int i = 0; i < m_path.size(); i++)
        {
            if (m_path[i].get_position().x < motion.position.x)
            {
                m_path[i].draw(projection);
            }
        }
    }
    // Transformation code, see Rendering and Transformation in the template specification for more info
    // Incrementally updates transformation matrix, thus ORDER IS IMPORTANT
    transform.begin();
    transform.translate(motion.position);
    transform.rotate(motion.radians);
    transform.scale(physics.scale);
    transform.end();

    // Setting shaders
    glUseProgram(effect.program);

    // Enabling alpha channel for textures
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    // Getting uniform locations for glUniform* calls
    GLint transform_uloc = glGetUniformLocation(effect.program, "transform");
    GLint color_uloc = glGetUniformLocation(effect.program, "fcolor");
    GLint projection_uloc = glGetUniformLocation(effect.program, "projection");

    // Setting vertices and indices
    glBindVertexArray(mesh.vao);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ibo);

    // Input data location as in the vertex buffer
    GLint in_position_loc = glGetAttribLocation(effect.program, "in_position");
    GLint in_texcoord_loc = glGetAttribLocation(effect.program, "in_texcoord");
    glEnableVertexAttribArray(in_position_loc);
    glEnableVertexAttribArray(in_texcoord_loc);
    glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void *)0);
    glVertexAttribPointer(in_texcoord_loc, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void *)sizeof(vec3));

    // Enabling and binding texture to slot 0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fish_texture.id);

    // Setting uniform values to the currently bound program
    glUniformMatrix3fv(transform_uloc, 1, GL_FALSE, (float *)&transform.out);
    float color[] = {1.f, 1.f, 1.f};
    glUniform3fv(color_uloc, 1, color);
    glUniformMatrix3fv(projection_uloc, 1, GL_FALSE, (float *)&projection);

    // Drawing!
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);
}

vec2 Fish::get_position() const
{
    return motion.position;
}

void Fish::set_position(vec2 position)
{
    motion.position = position;
}

vec2 Fish::get_bounding_box() const
{
    // Returns the local bounding coordinates scaled by the current size of the fish
    // fabs is to avoid negative scale due to the facing direction.
    return {std::fabs(physics.scale.x) * fish_texture.width, std::fabs(physics.scale.y) * fish_texture.height};
}

void Fish::set_debug_mode(bool m_debug)
{
    debug = m_debug;
}