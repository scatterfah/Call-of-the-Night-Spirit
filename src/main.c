#include <raylib.h>
#include "game.h"
#include "player.h"

typedef enum{
    Screen_Menu, Screen_Play, Screen_Scores
} GameScreen;

int main(void){
    InitWindow(1080,720,"Call of the Night Spirit"); //Initialize the window.
    SetTargetFPS(60); // Set target fps.
    SetExitKey(KEY_NULL);
    InitAudioDevice();
    GameScreen screen = Screen_Menu;
    Rectangle start_button = {390, 350, 300, 70};

    /*The four numbers are x, y, width, height. Width 300 centres the button, because (1080 - 300) / 2 = 390. The y of 350 puts it below the title.*/

    Rectangle exit_button = {390, 450, 300, 70};
    bool quiet = false;

    GameState game;                      // NEW: everything the game remembers

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
                game_init(&game);        // NEW: create the structures, start values
            }
            else if(hover_exit && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
                quiet = true;
            }
        }
        else if(screen == Screen_Play){
            if(IsKeyPressed(KEY_ESCAPE)){
                screen = Screen_Menu;
                ResumeMusicStream(menu_music);
                game_free(&game);        // NEW: free what game_init made
            }
            else{
                player_update(&game);    // NEW: move the player
            }
        }

        UpdateMusicStream(menu_music);
        BeginDrawing();
        ClearBackground(BLACK);
        if(screen == Screen_Menu){
            int w = MeasureText("Call of the Night Spirit", 60);
            int x = (1080 - w) / 2;
            DrawText("Call of the Night Spirit",x,150,60,RED); //DrawText(char text, int posX, int posY, int fontSize, Color color);

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
            player_draw(&game);          // NEW: draw the player
            DrawText("Press ESC to go back", 20, 20, 20, GRAY);
        }
        EndDrawing();
    }

    if(screen == Screen_Play){
        game_free(&game);                // NEW: quit while in the game
    }

    UnloadMusicStream(menu_music);
    CloseAudioDevice();
    CloseWindow();

    return 0;
}