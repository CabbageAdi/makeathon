#include "raylib.h"

#define PLATFORM_WEB

#include "/F/Repositories/emsdk/upstream/emscripten/system/include/emscripten.h"

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------
float x = 100;
float y = 100;
float rotation;
float speed = 300;

Camera3D camera;
// Camera mode type
//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------
void UpdateDrawFrame(void);     // Update and Draw one frame

//----------------------------------------------------------------------------------
// Main Enry Point
//----------------------------------------------------------------------------------
int main() {
    // Initialization
    //--------------------------------------------------------------------------------------
    int resolution = 50;
    int screenWidth = 16 * resolution;
    int screenHeight = 9 * resolution;
    InitWindow(screenWidth, screenHeight, "raylib [core] example - basic window");

    camera.position = (Vector3){ 0.0f, 10.0f, 10.0f };  // Camera position
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };      // Camera looking at point
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
    camera.fovy = 45.0f;                                // Camera field-of-view Y
    camera.projection = CAMERA_PERSPECTIVE;

    emscripten_set_main_loop(UpdateDrawFrame, 30, 1);

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

//----------------------------------------------------------------------------------
// Module Functions Definition
//----------------------------------------------------------------------------------
void UpdateDrawFrame(void) {
    // Update
    //----------------------------------------------------------------------------------
    // TODO: Update your variables here
    //----------------------------------------------------------------------------------


    // Draw
    //----------------------------------------------------------------------------------
    BeginDrawing();

    ClearBackground(GREEN);

    DrawText("RoboKnighs Make-a-thon", 5, 6, 20, BLACK);

    //BeginMode3D(camera);
    //Vector3 pos = {x, y, 0};
    //DrawCube(pos, 2, 2, 2, WHITE);
    DrawRectangle(x, y, 70, 70, BLUE);

    for (int i = 0; i < 4; i++){
        int val = EM_ASM_INT({
            let val = pinVal($0);
            return val;
        }, i);
        if (i == 0 && val == 1)
            x += speed * GetFrameTime();
        if (i == 1 && val == 1)
            x -= speed * GetFrameTime();
        if (i == 2 && val == 1)
            y += speed * GetFrameTime();
        if (i == 3 && val == 1)
            y -= speed * GetFrameTime();
    }

    if (x > 200){
        EM_ASM({
           setPin(4, 1);
        });
    }
    else{
        EM_ASM({
            setPin(4, 0);
        });
    }

    //EndMode3D();

    EndDrawing();
    //----------------------------------------------------------------------------------
}
