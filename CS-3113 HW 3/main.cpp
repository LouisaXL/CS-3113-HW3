#define LOG(argument) std::cout << argument << '\n'
#define STB_IMAGE_IMPLEMENTATION
#define GL_SILENCE_DEPRECATION
#define GL_GLEXT_PROTOTYPES 1

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"
#include "Entity.h"
#include <vector>
#include <ctime>
#include "cmath"

// ————— CONSTANTS ————— //
constexpr int   WINDOW_WIDTH = 640 * 2,
                WINDOW_HEIGHT = 480 * 2;

constexpr float BG_RED = 0.9765625f,
                BG_GREEN = 0.97265625f,
                BG_BLUE = 0.9609375f,
                BG_OPACITY = 1.0f;

constexpr int   VIEWPORT_X = 0,
                VIEWPORT_Y = 0,
                VIEWPORT_WIDTH = WINDOW_WIDTH,
                VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr char  V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
                F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr float MILLISECONDS_IN_SECOND = 1000.0;
constexpr char  FISH_FILEPATH[] = "fish.png",
                BIGFISH_FILEPATH[] = "bigfish.png",
                HOOK_FILEPATH[] = "hook.png",
                BACKGROUND_FILEPATH[] = "background.png",
                WIN_FILEPATH[] = "win.png",
                LOOSE_FILEPATH[] = "loose.png",
                STAHIGH_FILEPATH[] = "stamina_high.png",
                STAMID_FILEPATH[] = "stamina_mid.png",
                STALOW_FILEPATH[] = "stamina_low.png";


constexpr GLint NUMBER_OF_TEXTURES = 1,
                LEVEL_OF_DETAIL = 0,
                TEXTURE_BORDER = 0;

// constexpr int NUMBER_OF_NPCS = 3; // TODO no need

constexpr float FIXED_TIMESTEP = 1.0f / 60.0f;
constexpr float ACC_OF_GRAVITY = -9.81f;
constexpr int   PLATFORM_COUNT = 1;

// ————— STRUCTS AND ENUMS —————//
enum AppStatus { RUNNING, TERMINATED };
enum FilterType { NEAREST, LINEAR };

struct GameState
{
    Entity* player;
    Entity* bigfish;
    Entity* hook;
    Entity* win;
    Entity* loose;
    Entity* background;
    //Entity** stamina;
};

// ————— VARIABLES ————— //
GameState g_game_state;

SDL_Window* g_display_window;
AppStatus g_app_status = RUNNING;

ShaderProgram g_shader_program;
glm::mat4 g_view_matrix, g_projection_matrix;

float g_previous_ticks = 0.0f;
float g_time_accumulator = 0.0f;

void initialise();
void process_input();
void update();
void render();
void shutdown();

constexpr glm::vec3 BIGFISH_INIT_SCALE = glm::vec3(8.0f, 1.0f, 0.0f);   // TODO: DELETE THIS

const float DRAG = 0.5f;    // TODO: DELETE THIS

bool game_end;
int win_loose;


// ———— GENERAL FUNCTIONS ———— //
GLuint load_texture(const char* filepath, FilterType filterType)
{
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER,
        GL_RGBA, GL_UNSIGNED_BYTE, image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
        filterType == NEAREST ? GL_NEAREST : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
        filterType == NEAREST ? GL_NEAREST : GL_LINEAR);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    stbi_image_free(image);

    return textureID;
}

void initialise()
{
    SDL_Init(SDL_INIT_VIDEO);
    g_display_window = SDL_CreateWindow("ENTITY PLEASE WORK!",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL);



    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

    if (g_display_window == nullptr)
    {
        std::cerr << "Error: SDL window could not be created.\n";
        shutdown();
    }

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

    // ————— PLAYER ————— //
    GLuint player_texture_id = load_texture(FISH_FILEPATH, NEAREST);

    int player_walking_animation[4][4] =
    {
        { 1, 5, 9, 13 },  // for fish to move to the left,
        { 3, 7, 11, 15 }, // for fish to move to the right,
        { 2, 6, 10, 14 }, // for fish to move upwards,
        { 0, 4, 8, 12 }   // for fish to move downwards
    };

    glm::vec3 acceleration = glm::vec3(0.0f, -4.905f, 0.0f);

    g_game_state.player = new Entity(
        player_texture_id,         // texture id
        1.0f,                      // speed
        acceleration,               // acceleration
        0.3f,                       // up power
        DRAG,                       // drag = friction
        player_walking_animation,  // animation index sets
        0.0f,                      // animation time
        4,                         // animation frame amount
        0,                         // current animation index
        4,                         // animation column amount
        4                          // animation row amount
    );

    g_game_state.player->face_down();
    g_game_state.player->set_position(glm::vec3(0.0f, 2.0f, 0.0f)); // TODO ???
    g_game_state.player->set_acceleration(glm::vec3(0.0f, ACC_OF_GRAVITY * 0.1, 0.0f));

    // ————— NPCs ————— //

    GLuint bigfish_texture_id = load_texture(BIGFISH_FILEPATH, LINEAR);

    g_game_state.bigfish = new Entity(
                bigfish_texture_id,  // texture id
                1.0f              // speed
    );

    g_game_state.bigfish->set_scale(glm::vec3(8.0f, 1.0f, 0.0f));
    g_game_state.bigfish->set_position(glm::vec3(0.0f, -2.0f, 0.0f));
    g_game_state.bigfish->update(0.0f, g_game_state.player, 1);

    // HOOK
    GLuint hook_texture_id = load_texture(HOOK_FILEPATH, LINEAR);
    g_game_state.hook = new Entity(
        hook_texture_id,  // texture id
        1.0f              // speed
    );
    g_game_state.hook->set_scale(glm::vec3(1.0f, 1.0f, 0.0f));
    g_game_state.hook->set_position(glm::vec3(2.5f, 1.0f, 0.0f));
    g_game_state.hook->update(0.0f, g_game_state.player, 1);

    // WIN SCREEN
    GLuint win_texture_id = load_texture(WIN_FILEPATH, LINEAR);
    g_game_state.win = new Entity(
        win_texture_id,  // texture id
        1.0f              // speed
    );
    g_game_state.win->set_scale(glm::vec3(9, 6, 0.0f));
    g_game_state.win->set_position(glm::vec3(0.0f, 0.0f, 0.0f));
    g_game_state.win->update(0.0f, g_game_state.player, 1);

    // LOOSE SCREEN
    GLuint loose_texture_id = load_texture(LOOSE_FILEPATH, LINEAR);
    g_game_state.loose = new Entity(
        loose_texture_id,  // texture id
        1.0f              // speed
    );
    g_game_state.loose->set_scale(glm::vec3(9, 6, 0.0f));
    g_game_state.loose->set_position(glm::vec3(0.0f, 0.0f, 0.0f));
    g_game_state.loose->update(0.0f, g_game_state.player, 1);

    // STAMINA ARRAY
    //g_game_state.stamina = new Entity ** 3;


    // ————— GENERAL ————— //
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    
}

void process_input()
{
    // VERY IMPORTANT: If nothing is pressed, we don't want to go anywhere
    g_game_state.player->set_movement(glm::vec3(0.0f));

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type) {
        case SDL_QUIT:
        case SDL_WINDOWEVENT_CLOSE:
            g_app_status = TERMINATED;
            break;

        case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
            case SDLK_q: g_app_status = TERMINATED;
            default:     break;
            }

        default:
            break;
        }
    }

    const Uint8* key_state = SDL_GetKeyboardState(NULL);

    if (key_state[SDL_SCANCODE_LEFT])
    {
        g_game_state.player->move_left();
    }
    else if (key_state[SDL_SCANCODE_RIGHT])
    {
        g_game_state.player->move_right();
    }

    if (key_state[SDL_SCANCODE_UP])
    {
        g_game_state.player->move_up();
    }
    else if (key_state[SDL_SCANCODE_DOWN])
    {
        g_game_state.player->move_down();
    }

    if (glm::length(g_game_state.player->get_movement()) > 1.0f)
        g_game_state.player->normalise_movement();
}


void update()
{
    // ————— DELTA TIME ————— //
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;

    // ————— FIXED TIMESTEP ————— //
    // STEP 1: Keep track of how much time has passed since last step
    delta_time += g_time_accumulator;

    // STEP 2: Accumulate the ammount of time passed while we're under our fixed timestep
    if (delta_time < FIXED_TIMESTEP)
    {
        g_time_accumulator = delta_time;
        return;
    }

    // STEP 3: Once we exceed our fixed timestep, apply that elapsed time into the 
    //         objects' update function invocation
    while (delta_time >= FIXED_TIMESTEP)
    {
        // Notice that we're using FIXED_TIMESTEP as our delta time
        g_game_state.player->update(FIXED_TIMESTEP, g_game_state.hook,
            PLATFORM_COUNT);
        delta_time -= FIXED_TIMESTEP;
    }

    g_time_accumulator = delta_time;

    if (g_game_state.player->get_position().y < -2.1f) {
        game_end = true;
        win_loose = 0;
        g_game_state.player->set_velocity(glm::vec3(0.0f, 0.0f, 0.0f));
    }

    if (g_game_state.player->check_collision(g_game_state.hook)) {
        game_end = true;
        win_loose = 1;
    }

    //g_game_state.player->update(delta_time);

    ////for (int i = 0; i < NUMBER_OF_NPCS; i++) 
    //g_game_state.base->update(delta_time);
}






void render()
{
    glClear(GL_COLOR_BUFFER_BIT);
    //GLuint background_texture_id = load_texture(BACKGROUND_FILEPATH, LINEAR); //
    


    g_game_state.player->render(&g_shader_program);

    //for (int i = 0; i < NUMBER_OF_NPCS; i++)
    g_game_state.bigfish->render(&g_shader_program);

    g_game_state.hook->render(&g_shader_program);

    if (game_end && win_loose == 1) g_game_state.win->render(&g_shader_program);        // if win
    else if (game_end && win_loose == 0) g_game_state.loose->render(&g_shader_program); // if loose

    SDL_GL_SwapWindow(g_display_window);
}


void shutdown()
{
    SDL_Quit();
    delete   g_game_state.player;
    // delete[] 
    delete g_game_state.bigfish;
}


int main(int argc, char* argv[])
{
    initialise();

    while (g_app_status == RUNNING)
    {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}