#include <raylib.h>
#include "game.h"
#include "player.h"
#include "zombie.h"
#include "combat.h"
#include "world.h"
#include "pickup.h"
#include "score.h"

typedef enum{
    Screen_Menu, Screen_Play, Screen_Scores, Screen_GameOver
} GameScreen;

int main(void){
    InitWindow(1080, 720, "Call of the Night Spirit");
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);
    InitAudioDevice();

    score_load();                                // load saved high scores from scores.txt

    GameScreen screen = Screen_Menu;
    Rectangle start_button  = {390, 300, 300, 65};
    Rectangle scores_button = {390, 390, 300, 65};
    Rectangle exit_button   = {390, 480, 300, 65};
    bool quiet = false;

    GameState game;
    int  final_kills    = 0;                     // kills of the game that just ended
    int  final_wave     = 0;                     // wave reached in the game that just ended
    bool new_high_score = false;

    Music menu_music = LoadMusicStream("assets/an_untold_legend_v2.wav");
    SetMusicVolume(menu_music, 0.5f);
    PlayMusicStream(menu_music);

    while(!quiet && !WindowShouldClose()){
        Vector2 mouse = GetMousePosition();
        bool hover_start  = CheckCollisionPointRec(mouse, start_button);
        bool hover_scores = CheckCollisionPointRec(mouse, scores_button);
        bool hover_exit   = CheckCollisionPointRec(mouse, exit_button);

        if(screen == Screen_Menu){
            if(hover_start && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
                screen = Screen_Play;
                PauseMusicStream(menu_music);
                game_init(&game);
                world_reset();
                zombies_reset();
                combat_reset();
            }
            else if(hover_scores && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
                screen = Screen_Scores;
            }
            else if(hover_exit && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
                quiet = true;
            }
        }
        else if(screen == Screen_Scores){
            if(IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER)){
                screen = Screen_Menu;
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
                    final_kills    = game.kills; // remember the score before freeing
                    final_wave     = waves_number();
                    new_high_score = score_record(final_kills, final_wave);
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
            DrawText("Call of the Night Spirit", (1080 - w) / 2, 125, 60, RED);

            if(score_best() > 0){
                const char *best = TextFormat("Highest Score: %d kills", score_best());
                DrawText(best, (1080 - MeasureText(best, 26)) / 2, 220, 26, GOLD);
            }

            int ww = MeasureText("Start", 30);
            DrawRectangleRec(start_button, hover_start ? PURPLE : DARKPURPLE);
            DrawText("Start",
                     (int)(start_button.x + (start_button.width - ww) / 2),
                     (int)(start_button.y + (start_button.height - 30) / 2),
                     30, WHITE);

            int sw = MeasureText("High Scores", 30);
            DrawRectangleRec(scores_button, hover_scores ? PURPLE : DARKPURPLE);
            DrawText("High Scores",
                     (int)(scores_button.x + (scores_button.width - sw) / 2),
                     (int)(scores_button.y + (scores_button.height - 30) / 2),
                     30, WHITE);

            int ex = MeasureText("Exit", 30);
            DrawRectangleRec(exit_button, hover_exit ? PURPLE : DARKPURPLE);
            DrawText("Exit",
                     (int)(exit_button.x + (exit_button.width - ex) / 2),
                     (int)(exit_button.y + (exit_button.height - 30) / 2),
                     30, WHITE);
        }
        else if(screen == Screen_Scores){
            int tw = MeasureText("HIGH SCORES", 64);
            DrawText("HIGH SCORES", (1080 - tw) / 2, 100, 64, GOLD);

            int count = score_count();
            if(count == 0){
                const char *empty = "No scores saved yet. Play a game to set a record!";
                DrawText(empty, (1080 - MeasureText(empty, 26)) / 2, 320, 26, LIGHTGRAY);
            } else {
                DrawRectangle(260, 200, 560, 320, (Color){24, 20, 34, 255});
                DrawRectangleLines(260, 200, 560, 320, PURPLE);

                for(int i = 0; i < count; i++){
                    ScoreEntry e = score_get(i);
                    const char *row = TextFormat("#%d    %d kills    (Wave %d)",
                                                 i + 1, e.kills, e.wave);
                    Color color = (i == 0) ? GOLD : WHITE;
                    DrawText(row, (1080 - MeasureText(row, 30)) / 2, 232 + i * 56, 30, color);
                }
            }

            const char *back = "ESC: back to menu";
            DrawText(back, (1080 - MeasureText(back, 26)) / 2, 585, 26, LIGHTGRAY);
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
            DrawText("GAME OVER", (1080 - gw) / 2, 175, 80, RED);

            if(new_high_score){
                const char *badge = "NEW HIGH SCORE!";
                DrawText(badge, (1080 - MeasureText(badge, 34)) / 2, 270, 34, GOLD);
            }

            const char *score = TextFormat("Zombies killed: %d   (Wave %d)", final_kills, final_wave);
            DrawText(score, (1080 - MeasureText(score, 34)) / 2, 330, 34, WHITE);

            const char *best = TextFormat("Highest score: %d kills", score_best());
            DrawText(best, (1080 - MeasureText(best, 28)) / 2, 385, 28, GOLD);

            int pw = MeasureText("ENTER: play again", 26);
            DrawText("ENTER: play again", (1080 - pw) / 2, 470, 26, LIGHTGRAY);
            int mw = MeasureText("ESC: main menu", 26);
            DrawText("ESC: main menu", (1080 - mw) / 2, 510, 26, LIGHTGRAY);
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