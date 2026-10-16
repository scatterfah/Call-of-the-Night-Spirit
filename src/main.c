#include<raylib.h>
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

            }
            else if(hover_exit && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
                quiet = true;
            }
        }
        else if(screen == Screen_Play){
            if(IsKeyPressed(KEY_ESCAPE)){
                screen = Screen_Menu;
                ResumeMusicStream(menu_music);
            }
        }
        
        UpdateMusicStream(menu_music);
        BeginDrawing();
        ClearBackground(BLACK);
        if(screen == Screen_Menu){
            int w = MeasureText("Call the Night Spirit", 60);
            int x = (1080 - w) / 2;
            DrawText("Call the Night Spirit",x,150,60,RED); //DrawText(char text, int posX, int posY, int fontSize, Color color);

            int ww = MeasureText("START",30);
            int xx = start_button.x+(start_button.width - ww)/2;
            int yy = start_button.y+(start_button.height - 30)/2;
            Color button_color = hover ? PURPLE : DARKPURPLE;
            DrawRectangleRec(start_button, button_color);
            DrawText("START", xx, yy, 30, WHITE );

            int ex = MeasureText("EXIT",30);
            int xe = exit_button.x+(exit_button.width-ex)/2;
            int ye = exit_button.y+(exit_button.height-30)/2;
            Color button_color1 =hover_exit ? PURPLE : DARKPURPLE;
            DrawRectangleRec(exit_button, button_color1);
            DrawText("QUIET", xe, ye, 30, WHITE);
           
            
        }
        EndDrawing();
    }
    UnloadMusicStream(menu_music);
    CloseAudioDevice();
    CloseWindow();

    return 0;
}