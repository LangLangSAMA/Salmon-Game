// Header
#include "world.hpp"

// stlib
#include <string.h>
#include <cassert>
#include <sstream>

// Testing
// #include <iostream>
// #include <iomanip>

// using std::cout;
// using std::endl;

// Same as static in c, local to compilation unit
namespace
{
const size_t MAX_TURTLES = 15;
const size_t MAX_FISH = 3;
const size_t TURTLE_DELAY_MS = 2000;
const size_t FISH_DELAY_MS = 5000;

namespace
{
void glfw_err_cb(int error, const char *desc)
{
    fprintf(stderr, "%d: %s", error, desc);
}
} // namespace
} // namespace

World::World() : m_points(0),
                 m_next_turtle_spawn(0.f),
                 m_next_fish_spawn(0.f)
{
    // Seeding rng with random device
    m_rng = std::default_random_engine(std::random_device()());
}

World::~World()
{
}

// World initialization
bool World::init(vec2 screen)
{
    //-------------------------------------------------------------------------
    // GLFW / OGL Initialization
    // Core Opengl 3.
    glfwSetErrorCallback(glfw_err_cb);
    if (!glfwInit())
    {
        fprintf(stderr, "Failed to initialize GLFW");
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, 1);
#if __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    glfwWindowHint(GLFW_RESIZABLE, 0);
    m_window = glfwCreateWindow((int)screen.x, (int)screen.y, "Salmon Game Assignment", nullptr, nullptr);
    if (m_window == nullptr)
        return false;

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1); // vsync

    // Load OpenGL function pointers
    gl3w_init();

    // Setting callbacks to member functions (that's why the redirect is needed)
    // Input is handled using GLFW, for more info see
    // http://www.glfw.org/docs/latest/input_guide.html
    glfwSetWindowUserPointer(m_window, this);
    auto key_redirect = [](GLFWwindow *wnd, int _0, int _1, int _2, int _3) { ((World *)glfwGetWindowUserPointer(wnd))->on_key(wnd, _0, _1, _2, _3); };
    auto cursor_pos_redirect = [](GLFWwindow *wnd, double _0, double _1) { ((World *)glfwGetWindowUserPointer(wnd))->on_mouse_move(wnd, _0, _1); };
    glfwSetKeyCallback(m_window, key_redirect);
    glfwSetCursorPosCallback(m_window, cursor_pos_redirect);

    // Create a frame buffer
    m_frame_buffer = 0;
    glGenFramebuffers(1, &m_frame_buffer);
    glBindFramebuffer(GL_FRAMEBUFFER, m_frame_buffer);

    // For some high DPI displays (ex. Retina Display on Macbooks)
    // https://stackoverflow.com/questions/36672935/why-retina-screen-coordinate-value-is-twice-the-value-of-pixel-value
    int fb_width, fb_height;
    glfwGetFramebufferSize(m_window, &fb_width, &fb_height);
    m_screen_scale = static_cast<float>(fb_width) / screen.x;

    // Initialize the screen texture
    m_screen_tex.create_from_screen(m_window);

    //-------------------------------------------------------------------------
    // Loading music and sounds
    if (SDL_Init(SDL_INIT_AUDIO) < 0)
    {
        fprintf(stderr, "Failed to initialize SDL Audio");
        return false;
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) == -1)
    {
        fprintf(stderr, "Failed to open audio device");
        return false;
    }

    m_background_music = Mix_LoadMUS(audio_path("music.wav"));
    m_salmon_dead_sound = Mix_LoadWAV(audio_path("salmon_dead.wav"));
    m_salmon_eat_sound = Mix_LoadWAV(audio_path("salmon_eat.wav"));

    if (m_background_music == nullptr || m_salmon_dead_sound == nullptr || m_salmon_eat_sound == nullptr)
    {
        fprintf(stderr, "Failed to load sounds\n %s\n %s\n %s\n make sure the data directory is present",
                audio_path("music.wav"),
                audio_path("salmon_dead.wav"),
                audio_path("salmon_eat.wav"));
        return false;
    }

    // Playing background music indefinitely
    Mix_PlayMusic(m_background_music, -1);

    fprintf(stderr, "Loaded music\n");

    m_debug = false;

    m_current_speed = 1.f;

    m_collision_duration = 0.f;

    m_is_predict = false;

    m_frequency = 500.f;
    m_frequency_counter = m_frequency;
    advanced_fish = false;

    m_pebble_period = 250.f;

    return m_salmon.init() && m_water.init() && m_pebbles_emitter.init() && m_border.init() && m_box.init() && m_dot.init();
}

// Releases all the associated resources
void World::destroy()
{
    glDeleteFramebuffers(1, &m_frame_buffer);

    if (m_background_music != nullptr)
        Mix_FreeMusic(m_background_music);
    if (m_salmon_dead_sound != nullptr)
        Mix_FreeChunk(m_salmon_dead_sound);
    if (m_salmon_eat_sound != nullptr)
        Mix_FreeChunk(m_salmon_eat_sound);

    Mix_CloseAudio();

    m_salmon.destroy();
    m_pebbles_emitter.destroy();
    for (auto &turtle : m_turtles)
        turtle.destroy();
    for (auto &fish : m_fish)
        fish.destroy();
    m_turtles.clear();
    m_fish.clear();
    m_border.destroy();
    m_dot.destroy();
    m_box.destroy();
    glfwDestroyWindow(m_window);
}

// Update our game world
bool World::update(float elapsed_ms)
{
    if (m_collision_duration >= 0.f)
    {
        m_collision_duration -= elapsed_ms;
        return false;
    }

    int w, h;
    glfwGetFramebufferSize(m_window, &w, &h);
    vec2 screen = {(float)w / m_screen_scale, (float)h / m_screen_scale};

    if (m_pebble_period < 0.f)
    {
        float angle = m_salmon.get_rotation();
        float v_x = cosf(angle) * m_dist(m_rng) * 10;
        float v_y = sinf(angle) * m_dist(m_rng) * 10;
        vec2 vel = {v_x, v_y};
        float rad = m_dist(m_rng) * 5 + 10.f;
        vec2 pos = {m_salmon.get_position().x + cosf(angle) * 75, m_salmon.get_position().y + sinf(angle) * 75};
        m_pebbles_emitter.spawn_pebble(pos, vel, rad);
        m_pebble_period = 250.f;
    }

    m_pebble_period -= elapsed_ms;

    if (m_debug)
    {
        if (m_is_predict)
            m_salmon.predict_collision();
        if (m_is_collided)
        {
            m_salmon.reflect();
            m_is_collided = false;
        }
        else
        {
            if (m_salmon.collides_with_wall())
            {
                m_collision_duration = 500.f;
                m_is_collided = true;
            }
        }
    }
    else
    {
        if (m_salmon.collides_with_wall())
            m_salmon.reflect();
    }

    // Checking Salmon - Turtle collisions
    for (const auto &turtle : m_turtles)
    {
        if (m_salmon.collides_with(turtle))
        {
            if (m_salmon.is_alive())
            {
                Mix_PlayChannel(-1, m_salmon_dead_sound, 0);
                m_water.set_salmon_dead();
            }
            m_salmon.kill();
            m_debug = false;
            m_water.set_debug_mode(false);
            break;
        }
    }

    // Checking Salmon - Fish collisions
    auto fish_it = m_fish.begin();
    while (fish_it != m_fish.end())
    {
        if (m_salmon.is_alive() && m_salmon.collides_with(*fish_it))
        {
            fish_it = m_fish.erase(fish_it);
            m_salmon.light_up();
            Mix_PlayChannel(-1, m_salmon_eat_sound, 0);
            ++m_points;
        }
        else
            ++fish_it;
    }

    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // HANDLE SALMON - WALL COLLISIONS HERE
    // DON'T WORRY ABOUT THIS UNTIL ASSIGNMENT 2
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // HANDLE PEBBLE COLLISIONS HERE
    // DON'T WORRY ABOUT THIS UNTIL ASSIGNMENT 3
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    // Updating all entities, making the turtle and fish
    // faster based on current.
    // In a pure ECS engine we would classify entities by their bitmap tags during the update loop
    // rather than by their class.
    m_salmon.update(elapsed_ms);
    for (auto &turtle : m_turtles)
        turtle.update(elapsed_ms * m_current_speed);

    m_frequency_counter -= elapsed_ms;
    if (m_frequency_counter < 0)
    {
        advanced_fish = true;
    }
    for (auto &fish : m_fish)
    {
        if (advanced_fish)
        {
            fish.update_path(elapsed_ms, m_salmon.get_position());
        }
        fish.update(elapsed_ms * m_current_speed / 2);
    }

    if (advanced_fish)
    {
        m_frequency_counter = m_frequency;
        advanced_fish = false;
    }

    m_dot.update(m_salmon.get_collision_point());

    m_box.update(m_salmon.get_position());

    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // HANDLE PEBBLE SPAWN/UPDATES HERE
    // DON'T WORRY ABOUT THIS UNTIL ASSIGNMENT 3
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    m_pebbles_emitter.update(elapsed_ms);

    // Removing out of screen turtles
    auto turtle_it = m_turtles.begin();
    while (turtle_it != m_turtles.end())
    {
        float w = turtle_it->get_bounding_box().x / 2;
        if (turtle_it->get_position().x + w < 0.f)
        {
            turtle_it = m_turtles.erase(turtle_it);
            continue;
        }

        ++turtle_it;
    }

    // Removing out of screen fish
    fish_it = m_fish.begin();
    while (fish_it != m_fish.end())
    {
        float w = fish_it->get_bounding_box().x / 2;
        if (fish_it->get_position().x + w < 0.f)
        {
            fish_it = m_fish.erase(fish_it);
            continue;
        }

        ++fish_it;
    }

    // Spawning new turtles
    m_next_turtle_spawn -= elapsed_ms * m_current_speed;
    if (m_turtles.size() <= MAX_TURTLES && m_next_turtle_spawn < 0.f)
    {
        if (!spawn_turtle())
            return false;

        Turtle &new_turtle = m_turtles.back();

        // Setting random initial position
        new_turtle.set_position({screen.x + 150, 50 + m_dist(m_rng) * (screen.y - 100)});

        // Next spawn
        m_next_turtle_spawn = (TURTLE_DELAY_MS / 2) + m_dist(m_rng) * (TURTLE_DELAY_MS / 2);
    }

    // Spawning new fish
    m_next_fish_spawn -= elapsed_ms * m_current_speed;
    if (m_fish.size() <= MAX_FISH && m_next_fish_spawn < 0.f)
    {
        if (!spawn_fish())
            return false;
        Fish &new_fish = m_fish.back();

        new_fish.set_position({screen.x + 150, 50 + m_dist(m_rng) * (screen.y - 100)});
        new_fish.set_debug_mode(m_debug);
        new_fish.set_is_vertex_collision(m_is_vertex_collision);
        new_fish.init_path();

        m_next_fish_spawn = (FISH_DELAY_MS / 2) + m_dist(m_rng) * (FISH_DELAY_MS / 2);
    }

    // If salmon is dead, restart the game after the fading animation
    if (!m_salmon.is_alive() &&
        m_water.get_salmon_dead_time() > 5)
    {
        m_salmon.destroy();
        m_salmon.init();
        m_pebbles_emitter.destroy();
        m_pebbles_emitter.init();
        m_turtles.clear();
        m_fish.clear();
        m_water.reset_salmon_dead_time();
        m_current_speed = 1.f;
    }
    return true;
}

// Render our game world
// http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-14-render-to-texture/
void World::draw()
{
    // Clearing error buffer
    gl_flush_errors();

    // Getting size of window
    int w, h;
    glfwGetFramebufferSize(m_window, &w, &h);

    // Updating window title with points
    std::stringstream title_ss;
    title_ss << "Points: " << m_points;
    glfwSetWindowTitle(m_window, title_ss.str().c_str());

    /////////////////////////////////////
    // First render to the custom framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, m_frame_buffer);

    // Clearing backbuffer
    glViewport(0, 0, w, h);
    glDepthRange(0.00001, 10);
    const float clear_color[3] = {0.3f, 0.3f, 0.8f};
    glClearColor(clear_color[0], clear_color[1], clear_color[2], 1.0);
    glClearDepth(1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Fake projection matrix, scales with respect to window coordinates
    // PS: 1.f / w in [1][1] is correct.. do you know why ? (:
    float left = 0.f;                         // *-0.5;
    float top = 0.f;                          // (float)h * -0.5;
    float right = (float)w / m_screen_scale;  // *0.5;
    float bottom = (float)h / m_screen_scale; // *0.5;

    float sx = 2.f / (right - left);
    float sy = 2.f / (top - bottom);
    float tx = -(right + left) / (right - left);
    float ty = -(top + bottom) / (top - bottom);
    mat3 projection_2D{{sx, 0.f, 0.f}, {0.f, sy, 0.f}, {tx, ty, 1.f}};

    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // DRAW DEBUG INFO HERE
    // DON'T WORRY ABOUT THIS UNTIL ASSIGNMENT 2
    // You will want to create your own data structures for passing in
    // relevant information to your debug draw call.
    // The shaders coloured.vs.glsl and coloured.fs.glsl should be helpful.
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    // Drawing entities
    if (m_debug)
    {
        m_box.draw(projection_2D);
        m_border.draw(projection_2D);
    }
    for (auto &turtle : m_turtles)
        turtle.draw(projection_2D);
    for (auto &fish : m_fish)
        fish.draw(projection_2D);
    m_pebbles_emitter.draw(projection_2D);
    m_salmon.draw(projection_2D);

    if (m_debug)
        if (m_is_collided || m_is_predict)
            m_dot.draw(projection_2D);

    /////////////////////
    // Truely render to the screen
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Clearing backbuffer
    glViewport(0, 0, w, h);
    glDepthRange(0, 10);
    glClearColor(0, 0, 0, 1.0);
    glClearDepth(1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Bind our texture in Texture Unit 0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_screen_tex.id);

    m_water.draw(projection_2D);

    //////////////////
    // Presenting
    glfwSwapBuffers(m_window);
}

// Should the game be over ?
bool World::is_over() const
{
    return glfwWindowShouldClose(m_window);
}

// Creates a new turtle and if successfull adds it to the list of turtles
bool World::spawn_turtle()
{
    Turtle turtle;
    if (turtle.init())
    {
        m_turtles.emplace_back(turtle);
        return true;
    }
    fprintf(stderr, "Failed to spawn turtle");
    return false;
}

// Creates a new fish and if successfull adds it to the list of fish
bool World::spawn_fish()
{
    Fish fish;
    if (fish.init())
    {
        m_fish.emplace_back(fish);
        return true;
    }
    fprintf(stderr, "Failed to spawn fish");
    return false;
}

// On key callback
void World::on_key(GLFWwindow *, int key, int, int action, int mod)
{
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // HANDLE SALMON MOVEMENT HERE
    // key is of 'type' GLFW_KEY_
    // action can be GLFW_PRESS GLFW_RELEASE GLFW_REPEAT
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    // Resetting game
    if (action == GLFW_RELEASE && key == GLFW_KEY_R && !m_debug)
    {
        int w, h;
        glfwGetWindowSize(m_window, &w, &h);
        m_salmon.destroy();
        m_salmon.init();
        m_pebbles_emitter.destroy();
        m_pebbles_emitter.init();
        m_turtles.clear();
        m_fish.clear();
        m_water.reset_salmon_dead_time();
        m_debug = false;
        m_water.set_debug_mode(false);
        m_current_speed = 1.f;
    }

    if (action == GLFW_PRESS && key == GLFW_KEY_C)
    {
        m_debug = !m_debug;
        m_water.set_debug_mode(m_debug);
        for (auto &fish : m_fish)
            fish.set_debug_mode(m_debug);
    }

    if (action == GLFW_RELEASE && key == GLFW_KEY_Z)
    {
        m_is_vertex_collision = !m_is_vertex_collision;
        if (m_is_vertex_collision)
            fprintf(stderr, "Vertex collision check enabled for newly spawned fish\n");
        else
            fprintf(stderr, "Vertex collision check disabled for newly spawned fish\n");
    }

    if (action == GLFW_RELEASE && key == GLFW_KEY_X)
    {
        m_salmon.advanced_mode();
    }

    if (action == GLFW_PRESS && (key == GLFW_KEY_UP || key == GLFW_KEY_W))
        m_salmon.move_up = true;
    if (action == GLFW_PRESS && (key == GLFW_KEY_DOWN || key == GLFW_KEY_S))
        m_salmon.move_down = true;
    if (action == GLFW_PRESS && (key == GLFW_KEY_LEFT || key == GLFW_KEY_A))
        m_salmon.move_left = true;
    if (action == GLFW_PRESS && (key == GLFW_KEY_RIGHT || key == GLFW_KEY_D))
        m_salmon.move_right = true;

    if (action == GLFW_RELEASE && (key == GLFW_KEY_UP || key == GLFW_KEY_W))
        m_salmon.move_up = false;
    if (action == GLFW_RELEASE && (key == GLFW_KEY_DOWN || key == GLFW_KEY_S))
        m_salmon.move_down = false;
    if (action == GLFW_RELEASE && (key == GLFW_KEY_LEFT || key == GLFW_KEY_A))
        m_salmon.move_left = false;
    if (action == GLFW_RELEASE && (key == GLFW_KEY_RIGHT || key == GLFW_KEY_D))
        m_salmon.move_right = false;

    // Control the current speed with `<` `>`
    if (action == GLFW_RELEASE && (mod & GLFW_MOD_SHIFT) && key == GLFW_KEY_COMMA)
        m_current_speed -= 0.1f;
    if (action == GLFW_RELEASE && (mod & GLFW_MOD_SHIFT) && key == GLFW_KEY_PERIOD)
        m_current_speed += 0.1f;

    if (action == GLFW_RELEASE && key == GLFW_KEY_P)
    {
        float angle = m_salmon.get_rotation();
        float v_x = cosf(angle) * m_dist(m_rng) * 10;
        float v_y = sinf(angle) * m_dist(m_rng) * 10;
        vec2 vel = {v_x, v_y};
        float rad = m_dist(m_rng) * 5 + 10.f;
        vec2 pos = {m_salmon.get_position().x + cosf(angle) * 75, m_salmon.get_position().y + sinf(angle) * 75};
        m_pebbles_emitter.spawn_pebble(pos, vel, rad);
    }

    if (action == GLFW_RELEASE && key == GLFW_KEY_V)
    {
        m_is_predict = !m_is_predict;
        if (m_is_predict)
            fprintf(stderr, "Predict collision Enbaled\n");
        else
            fprintf(stderr, "Predict collision Disabled\n");
    }

    if (action == GLFW_RELEASE && key == GLFW_KEY_N)
    {
        if (m_frequency > 0.0f)
        {
            m_frequency -= 100.0f;
        }
        fprintf(stderr, "The current reaction time is %f\n", m_frequency);
    }

    if (action == GLFW_RELEASE && key == GLFW_KEY_M)
    {
        m_frequency += 100.0f;
        fprintf(stderr, "The current reaction time is %f\n", m_frequency);
    }

    m_current_speed = fmax(0.f, m_current_speed);
}

void World::on_mouse_move(GLFWwindow *window, double xpos, double ypos)
{
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // HANDLE SALMON ROTATION HERE
    // xpos and ypos are relative to the top-left of the window, the salmon's
    // default facing direction is (1, 0)
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
}
