#include<raylib.h>
typedef enum{
    Screen_Menu, Screen_Play, Screen_Scores
} GameScreen;

int main(void){
    InitWindow(1080,720,"Call of the Night Spirit"); //Initialize the window.
    SetTargetFPS(60); // Set target fps.
    SetExitKey(KEY_NULL);
    GameScreen screen = Screen_Menu;
    while(!WindowShouldClose()){
        BeginDrawing();
        ClearBackground(BLACK);
        if(screen == Screen_Menu){
            int w = MeasureText("Call the Night Spirit", 60);
            int x = (1080 - w) / 2;
            DrawText("Call the Night Spirit",x,150,60,RED);
            
        }
        EndDrawing();
    }
    CloseWindow();

    return 0;
}