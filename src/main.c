#include<raylib.h>
typedef enum{
    Screen_Menu, Screen_Play, Screen_Scores
} GameScreen;

int main(void){
    InitWindow(1080,720,"Call of the Night Spirit"); //Initialize the window.
    SetTargetFPS(60); // Set target fps.
    SetExitKey(KEY_NULL);
    GameScreen screen = Screen_Menu;
    Rectangle start_button = {390, 350, 300, 70}; 
    /*The four numbers are x, y, width, height.
    Width 300 centres the button, because (1080 - 300) / 2 = 390. The y of 350 puts it below the title.*/
    while(!WindowShouldClose()){
        Vector2 mouse = GetMousePosition();
        bool hover = CheckCollisionPointRec(mouse, start_button);
        if(screen == Screen_Menu){
            if(hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
                screen = Screen_Play;
            }
        }
        else if(screen == Screen_Play){
            if(IsKeyPressed(KEY_ESCAPE)){
                screen = Screen_Menu;
            }
        }
        BeginDrawing();
        ClearBackground(BLACK);
        if(screen == Screen_Menu){
            int w = MeasureText("Call the Night Spirit", 60);
            int x = (1080 - w) / 2;
            int ww = MeasureText("START",30);
            int xx = start_button.x+(start_button.width - ww)/2;
            int yy = start_button.y+(start_button.height - 30)/2;
            DrawText("Call the Night Spirit",x,150,60,RED); //DrawText(char text, int posX, int posY, int fontSize, Color color);
            Color button_color = hover ? PURPLE : DARKPURPLE;
            DrawRectangleRec(start_button, button_color);
            DrawText("START", xx, yy, 30, WHITE );
           
            
        }
        EndDrawing();
    }
    CloseWindow();

    return 0;
}