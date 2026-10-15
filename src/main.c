#include<raylib.h>
int main(void){
    InitWindow(1080,720,"Call of the Night Spirit"); //Initialize the window.
    SetTargetFPS(60); // Set target fps.
    while(!WindowShouldClose()){
        BeginDrawing();
        ClearBackground(WHITE);
        EndDrawing();
    }
    CloseWindow();

    return 0;
}