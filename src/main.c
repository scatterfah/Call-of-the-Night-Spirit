#include <raylib.h>
#include "game.h"
#include "assets.h"
#include "player.h"
#include "zombie.h"
#include "combat.h"
#include "world.h"
#include "pickup.h"
#include "score.h"

typedef enum{
    Screen_Menu, Screen_Play, Screen_Scores, Screen_GameOver, Screen_Victory
} GameScreen;

int main(void){
    InitWindow(1080, 720, "Call of the Night Spirit");
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);
    InitAudioDevice();

    assets_load();                               // load pixel-art textures from assets/
    score_load();                                // load saved high scores from scores.txt

    GameScreen screen = Screen_Menu;
    Rectangle start_button  = {390, 300, 300, 65};
    Rectangle scores_button = {390, 390, 300, 65};
    Rectangle exit_button   = {390, 480, 300, 65};
    bool quiet = false;

    GameState game;
    float play_time      = 0.0f;
    float final_time     = 0.0f;
    int   final_kills    = 0;
    int   final_wave     = 0;
    bool  new_high_score = false;

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
                screen    = Screen_Play;
                play_time = 0.0f;
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
                play_time += GetFrameTime();

                player_update(&game);
                combat_fire(&game);
                combat_throw_bomb(&game);
                combat_update(&game);
                zombies_update(&game);
                world_update(&game);
                combat_update_targets(&game);
                pickups_update(&game);

                if(boss_is_defeated()){
                    final_kills    = game.kills;
                    final_wave     = waves_number();
                    final_time     = play_time;
                    new_high_score = score_record(final_kills, final_wave);
                    game_free(&game);
                    ResumeMusicStream(menu_music);
                    screen = Screen_Victory;
                }
                else if(game.player_hp <= 0){
                    final_kills    = game.kills;
                    final_wave     = waves_number();
                    final_time     = play_time;
                    new_high_score = score_record(final_kills, final_wave);
                    game_free(&game);
                    screen = Screen_GameOver;
                }
            }
        }
        else if(screen == Screen_GameOver || screen == Screen_Victory){
            if(IsKeyPressed(KEY_ENTER)){         // play again
                screen    = Screen_Play;
                play_time = 0.0f;
                PauseMusicStream(menu_music);
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
            world_draw_menu_bg(false);

            int w = MeasureText("Call of the Night Spirit", 60);
            DrawText("Call of the Night Spirit", (1080 - w) / 2 + 3, 128, 60, BLACK);
            DrawText("Call of the Night Spirit", (1080 - w) / 2, 125, 60, (Color){235, 55, 95, 255});

            if(score_best() > 0){
                const char *best = TextFormat("Highest Score: %d kills", score_best());
                DrawText(best, (1080 - MeasureText(best, 26)) / 2, 220, 26, GOLD);
            }

            Color btn_fill  = (Color){ 48, 22, 68, 225 };
            Color btn_hover = (Color){ 104, 34, 122, 240 };

            int ww = MeasureText("Start", 30);
            DrawRectangleRec(start_button, hover_start ? btn_hover : btn_fill);
            DrawRectangleLinesEx(start_button, 2.0f, hover_start ? GOLD : PURPLE);
            DrawText("Start",
                     (int)(start_button.x + (start_button.width - ww) / 2),
                     (int)(start_button.y + (start_button.height - 30) / 2),
                     30, WHITE);

            int sw = MeasureText("High Scores", 30);
            DrawRectangleRec(scores_button, hover_scores ? btn_hover : btn_fill);
            DrawRectangleLinesEx(scores_button, 2.0f, hover_scores ? GOLD : PURPLE);
            DrawText("High Scores",
                     (int)(scores_button.x + (scores_button.width - sw) / 2),
                     (int)(scores_button.y + (scores_button.height - 30) / 2),
                     30, WHITE);

            int ex = MeasureText("Exit", 30);
            DrawRectangleRec(exit_button, hover_exit ? btn_hover : btn_fill);
            DrawRectangleLinesEx(exit_button, 2.0f, hover_exit ? GOLD : PURPLE);
            DrawText("Exit",
                     (int)(exit_button.x + (exit_button.width - ex) / 2),
                     (int)(exit_button.y + (exit_button.height - 30) / 2),
                     30, WHITE);
        }
        else if(screen == Screen_Scores){
            world_draw_menu_bg(true);

            int tw = MeasureText("HIGH SCORES", 64);
            DrawText("HIGH SCORES", (1080 - tw) / 2, 100, 64, GOLD);

            int count = score_count();
            if(count == 0){
                const char *empty = "No scores saved yet. Play a game to set a record!";
                DrawText(empty, (1080 - MeasureText(empty, 26)) / 2, 320, 26, LIGHTGRAY);
            } else {
                DrawRectangle(260, 200, 560, 320, (Color){ 20, 12, 32, 225 });
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
            world_draw_play_bg(&game);

            BeginMode2D(world_camera(&game));
                world_draw();
                pickups_draw(&game);
                zombies_draw(&game);
                combat_draw(&game);
                player_draw(&game);
                world_draw_over(&game);
            EndMode2D();

            hud_draw(&game);
            DrawText("WASD: move  LMB: shoot  RMB/G: bomb  Q: auto-aim  ESC: menu", 20, 695, 18, LIGHTGRAY);
        }
        else if(screen == Screen_Victory){
            world_draw_menu_bg(true);
            DrawRectangleGradientV(0, 0, 1080, 720, (Color){ 255, 185, 65, 35 }, (Color){ 18, 8, 28, 120 });

            const char *v_title = "VICTORY!";
            int vw = MeasureText(v_title, 82);
            DrawText(v_title, (1080 - vw) / 2 + 3, 123, 82, BLACK);
            DrawText(v_title, (1080 - vw) / 2,     120, 82, GOLD);

            const char *v_sub = "The Night Spirit has been vanquished!";
            DrawText(v_sub, (1080 - MeasureText(v_sub, 28)) / 2, 212, 28, WHITE);

            if(new_high_score){
                const char *badge = "NEW HIGH SCORE!";
                DrawText(badge, (1080 - MeasureText(badge, 32)) / 2, 255, 32, (Color){ 110, 255, 155, 255 });
            }

            int mins = (int)final_time / 60;
            int secs = (int)final_time % 60;
            DrawRectangle(280, 305, 520, 185, (Color){ 18, 10, 28, 220 });
            DrawRectangleLines(280, 305, 520, 185, GOLD);

            const char *s1 = TextFormat("Total Kills: %d   (Wave %d)", final_kills, final_wave);
            const char *s2 = TextFormat("Clear Time: %02d:%02d", mins, secs);
            const char *s3 = TextFormat("Highest Score: %d kills", score_best());
            DrawText(s1, (1080 - MeasureText(s1, 30)) / 2, 330, 30, WHITE);
            DrawText(s2, (1080 - MeasureText(s2, 28)) / 2, 382, 28, (Color){ 255, 220, 140, 255 });
            DrawText(s3, (1080 - MeasureText(s3, 26)) / 2, 432, 26, GOLD);

            const char *pa = "ENTER: play again";
            const char *mm = "ESC: main menu";
            DrawText(pa, (1080 - MeasureText(pa, 26)) / 2, 535, 26, LIGHTGRAY);
            DrawText(mm, (1080 - MeasureText(mm, 26)) / 2, 575, 26, LIGHTGRAY);
        }
        else if(screen == Screen_GameOver){
            world_draw_menu_bg(true);

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

    assets_unload();
    UnloadMusicStream(menu_music);
    CloseAudioDevice();
    CloseWindow();

    return 0;
}