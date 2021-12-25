#include "raylib.h"
#include "/F/Repositories/emsdk/upstream/emscripten/system/include/emscripten.h"
#include "math.h"
#include <stdio.h>

//robot values
float x = 6;
float y = 0;
float rotation;
float speed = 5;
#define fsdist 5 //front sensor distance from middle
#define lsdist 4 //left sensor distance from middle
#define rsdist (-4) //right sensor distance from middle
#define backdist 5 //back distance from middle

//global variables
Camera3D camera;

//functions
void UpdateDrawFrame(void);
int getPin(int pin);
void setPin(int pin, int value);
void Log(char* text);
char* IntStr(int num);
char* FloatStr(float num);
void DrawThin(Vector2 pos);
float GetDistance(Vector2 start, Vector2 end, Vector2 sensorPos, float rot);

//maze values
#define MAZE_SIZE 12
#define MAZE_THICKNESS 2
#define POINTS 5
int mazePoints[POINTS][2][2] = {
        {{0, 0}, {0, 1}},
        {{0, 1}, {1, 1}},
        {{1, 1}, {1, 2}},
        {{1, 2}, {-1, 2}},
        {{-1, 2}, {-1, 3}}
};

Model robotModel;

int main() {
    //window
    int resolution = 50;
    int screenWidth = 16 * resolution;
    int screenHeight = 9 * resolution;
    InitWindow(screenWidth, screenHeight, "raylib [core] example - basic window");

    //camera
    camera.position = (Vector3) {5, 20.0f, 0};
    camera.target = (Vector3) {0.0f, 0.0f, 0.0f};
    camera.up = (Vector3) {0.0f, 1.0f, 0.0f};
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    SetCameraMode(camera, CAMERA_FREE);

    robotModel = LoadModel("resources/mazesolver.obj");

    //run loop
    emscripten_set_main_loop(UpdateDrawFrame, 30, 1);

    CloseWindow();

    return 0;
}

void UpdateDrawFrame(void) {
    //camera movement
    UpdateCamera(&camera);

    BeginDrawing();
    ClearBackground(GREEN);

    //2d elements
    DrawText("RoboKnighs Make-a-thon", 5, 6, 1, BLACK);

    //robot
    Vector3 pos = {x, 0, y};
    //rotation = fmodf(rotation,  360.0f);
    //if (rotation < 0) rotation += 360;

    //3d
    BeginMode3D(camera);
    DrawGrid(200, 1);
    DrawModelEx(robotModel, pos, (Vector3){0, 1, 0}, rotation, (Vector3){2, 2, 2}, WHITE);

    // TODO THIS SHOULD ACCOUNT FOR ROTATION
    Vector2 forwardSensorPos = { x, y + fsdist };
    DrawThin(forwardSensorPos);
    Vector2 rightSensorPos = { x + rsdist, y };
    DrawThin(rightSensorPos);
    float forwardMin = -1;
    float backMin = -1;
    float rightMin = -1;
    float leftMin = -1;
    //maze drawing
    for (int i = 0; i < POINTS; i++){
        Vector2 start = { mazePoints[i][0][0] * MAZE_SIZE, mazePoints[i][0][1] * MAZE_SIZE };
        Vector2 end = { mazePoints[i][1][0] * MAZE_SIZE, mazePoints[i][1][1] * MAZE_SIZE };

        //cuboid drawing
        Vector3 midpoint = {(start.x + end.x) / 2,0.5f, (start.y + end.y) / 2};
        DrawCube(midpoint, start.x == end.x ? MAZE_THICKNESS : fabsf(start.x - end.x) + MAZE_THICKNESS, 5 , start.y == end.y ? MAZE_THICKNESS : fabsf(start.y - end.y) + MAZE_THICKNESS, BLACK);

        //sensor values
        //forward and back
        float forward = GetDistance(start, end, forwardSensorPos, rotation);
        if (forward > 0 && (forward < forwardMin || forwardMin == -1)){
            forwardMin = forward;
        }
        //back
        else if (forward < 0){
            float backDist = -forward - (fsdist + backdist);
            if (backDist < backMin || backMin == -1){
                backMin = backDist;
            }
        }
        //left and right
        float right = GetDistance(start, end, rightSensorPos, rotation + 90);
        if (right > 0 && (right < rightMin || rightMin == -1)){
            rightMin = right;
        }
        //left
        else if (right < 0){
            float leftDist = -forward - (fsdist + backdist);
            if (leftDist < leftMin || leftMin == -1){
                leftMin = leftDist;
            }
        }
    }


    //Log(FloatStr(forwardMin));
    //TODO SET PIN VALUES

    //TODO
    //robot movement
    for (int i = 0; i < 4; i++) {
        int val = getPin(i);
        if (i == 0 && val == 1)
            x += speed * GetFrameTime();
        if (i == 1 && val == 1)
            x -= speed * GetFrameTime();
        if (i == 2 && val == 1)
            y += speed * GetFrameTime();
        if (i == 3 && val == 1)
            y -= speed * GetFrameTime();
    }

    if (IsKeyDown(KEY_LEFT)){
        rotation -= 30 * GetFrameTime();
    }
    else if (IsKeyDown(KEY_RIGHT)){
        rotation += 30 * GetFrameTime();
    }

    if (IsKeyDown(KEY_W)){
        y += speed * GetFrameTime();
    }
    if (IsKeyDown(KEY_S)){
        y -= speed * GetFrameTime();
    }
    if (IsKeyDown(KEY_D)){
        x -= speed * GetFrameTime();
    }
    if (IsKeyDown(KEY_A)){
        x += speed * GetFrameTime();
    }

    Log(IntStr(x));
    Log(IntStr(y));

    EndMode3D();

    EndDrawing();
}

float GetDistance(Vector2 start, Vector2 end, Vector2 sensorPos, float rot){
    if (start.y == end.y){
        float distance = start.y - sensorPos.y;
        float perpendLen = tanf(rot * (PI / 180)) * distance;
        float perpendPoint = sensorPos.x + perpendLen;
        if ((start.x > end.x && perpendPoint > end.x && perpendPoint < start.x) ||
            (start.x < end.x && perpendPoint < end.x && perpendPoint > start.x)){
            float final = (1 / cosf(rot * (PI / 180))) * distance - (MAZE_THICKNESS / 2.0f);
            return final;
            /*if (final > 0 && (final < min || min == -1)){
                min = final;
            }
            //back
            else if (final < 0){
                float backDist = -final - (oppositeOffset);
                if (backDist < oppositeMin || oppositeMin == -1){
                    oppositeMin = backDist;
                }
            }*/
        }
    }
    return -1;
}

int getPin(int pin) {
    return EM_ASM_INT({
        let val = pinVal($0);
        return val;
        }, pin);
}

void setPin(int pin, int value) {
    EM_ASM({
        setPin($0, $1)
        }, pin, value);
}

char* IntStr(int num){
    char str[10000];
    sprintf(str, "%d", num);
    return str;
}

char* FloatStr(float num){
    char str[10000];
    sprintf(str, "%g", num);
    return str;
}

void DrawThin(Vector2 pos){
    DrawCube((Vector3){pos.x, 0, pos.y}, 0.3, 100, 0.3, RED);
}

EM_JS(void, Log, (char* text), {
    console.log(UTF8ToString(text));
})