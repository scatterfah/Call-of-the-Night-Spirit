#include <raylib.h>
#include "game.h"
#include "player.h"
#include "zombie.h"
#include "combat.h"
#include "world.h"
#include "pickup.h"

typedef enum{
    Screen_Menu, Screen_Play, Screen_Scores, Screen_GameOver
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
    int final_kills = 0;                         // kills of the game that just ended

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
                world_reset();
                zombies_reset();
                combat_reset();
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
                combat_fire(&game);
                combat_throw_bomb(&game);
                combat_update(&game);
                zombies_update(&game);
                world_update(&game);             // open gates on kill quota and scroll camera
                combat_update_targets(&game);    // rebuild target heap after dead zombies are removed
                pickups_update(&game);           // items on the ground

                if(game.player_hp <= 0){
                    final_kills = game.kills;    // remember the score before freeing
                    game_free(&game);
                    screen = Screen_GameOver;
                }
            }
        }
        else if(screen == Screen_GameOver){
            if(IsKeyPressed(KEY_ENTER)){         // play again
                screen = Screen_Play;
                game_init(&game);
                world_reset();
                zombies_reset();
                combat_reset();
            }
            else if(IsKeyPressed(KEY_ESCAPE)){   // back to the menu
                screen = Screen_Menu;
                ResumeMusicStream(menu_music);
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
            BeginMode2D(world_camera(&game));
                world_draw();
                pickups_draw(&game);             // items under the zombies
                zombies_draw(&game);
                combat_draw(&game);
                player_draw(&game);
            EndMode2D();

            hud_draw(&game);
            DrawText("WASD: move  LMB: shoot  RMB/G: bomb  Q: auto-aim  ESC: menu", 20, 695, 18, LIGHTGRAY);
        }
        else if(screen == Screen_GameOver){
            int gw = MeasureText("GAME OVER", 80);
            DrawText("GAME OVER", (1080 - gw) / 2, 220, 80, RED);

            const char *score = TextFormat("Zombies killed: %d", final_kills);
            int sw = MeasureText(score, 36);
            DrawText(score, (1080 - sw) / 2, 340, 36, WHITE);

            int pw = MeasureText("ENTER: play again", 26);
            DrawText("ENTER: play again", (1080 - pw) / 2, 440, 26, LIGHTGRAY);
            int mw = MeasureText("ESC: main menu", 26);
            DrawText("ESC: main menu", (1080 - mw) / 2, 480, 26, LIGHTGRAY);
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