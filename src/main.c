#include <raylib.h>
#include "game.h"
#include "player.h"
#include "zombie.h"
#include "combat.h"

typedef enum{
    Screen_Menu, Screen_Play, Screen_Scores
} GameScreen;

int main(void){
    InitWindow(1080,720,"Call of the Night Spirit");
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);
    InitAudioDevice();
    GameScreen screen = Screen_Menu;
    Rectangle start_button = {390, 350, 300, 70};
    Rectangle exit_button = {390, 450, 300, 70};
    bool quiet = false;

    GameState game;

    Music menu_music = LoadMusicStream("assets/an_untold_legend_v2.wav");
    SetMusicVolume(menu_music, 0.5f);
    PlayMusicStream(menu_music);

    while(!quiet && !WindowShouldClose()){
        Vector2 mouse = GetMousePosition();
        bool hover = CheckCollisionPointRec(mouse, start_button);
        bool hover_exit = CheckCollisionPointRec(mouse, exit_button);

        if(screen == Screen_Menu){
            if(hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
                screen = Screen_Play;
                PauseMusicStream(menu_music);
                game_init(&game);
                zombies_reset();
                combat_reset();                  // NEW: reset the fire cooldown
            }
            else if(hover_exit && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
                quiet = true;
            }
        }
        else if(screen == Screen_Play){
            if(IsKeyPressed(KEY_ESCAPE)){
                screen = Screen_Menu;
                ResumeMusicStream(menu_music);
                game_free(&game);
            }
            else{
                player_update(&game);
                combat_fire(&game);              // NEW: shoot while the mouse is held
                combat_update(&game);            // NEW: move bullets, hit zombies
                zombies_update(&game);           // removes the dead and counts kills
            }
        }

        UpdateMusicStream(menu_music);
        BeginDrawing();
        ClearBackground(BLACK);
        if(screen == Screen_Menu){
            int w = MeasureText("Call of the Night Spirit", 60);
            int x = (1080 - w) / 2;
            DrawText("Call of the Night Spirit",x,150,60,RED);

            int ww = MeasureText("Start",30);
            int xx = start_button.x+(start_button.width - ww)/2;
            int yy = start_button.y+(start_button.height - 30)/2;
            Color button_color = hover ? PURPLE : DARKPURPLE;
            DrawRectangleRec(start_button, button_color);
            DrawText("Start", xx, yy, 30, WHITE );

            int ex = MeasureText("Exit",30);
            int xe = exit_button.x+(exit_button.width-ex)/2;
            int ye = exit_button.y+(exit_button.height-30)/2;
            Color button_color1 =hover_exit ? PURPLE : DARKPURPLE;
            DrawRectangleRec(exit_button, button_color1);
            DrawText("Exit", xe, ye, 30, WHITE);
        }
        else if(screen == Screen_Play){
            zombies_draw(&game);
            combat_draw(&game);                  // NEW: bullets above zombies
            player_draw(&game);

            DrawText(TextFormat("HP: %d    Kills: %d    Zombies: %d",
                     game.player_hp, game.kills, (int)list_size(game.zombies)),
                     20, 20, 24, WHITE);
            DrawText("WASD: move   Hold left mouse: shoot   ESC: menu", 20, 50, 18, GRAY);
        }
        EndDrawing();
    }

    if(screen == Screen_Play){
        game_free(&game);
    }

    UnloadMusicStream(menu_music);
    CloseAudioDevice();
    CloseWindow();

    return 0;
}