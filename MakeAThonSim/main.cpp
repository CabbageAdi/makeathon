#include "raylib.h"
#include "/F/Repositories/emsdk/upstream/emscripten/system/include/emscripten.h"
#include "math.h"
#include "iostream"
#include "cstring"

using namespace std;

bool debug = false;

//robot values
#define xDefault 77
#define yDefault -11
#define rotDefault 0
#define maxForwardSpeed 10
#define maxRotSpeed 100

float x = xDefault;
float y = yDefault;
float rotation = rotDefault;
float speed = maxForwardSpeed;
float rotationSpeed = maxRotSpeed;
#define fsdist 6.5f //front sensor distance from middle
#define ssxdist 2 //side sensor distances from middle on x
#define ssydist 4.4f //side sensor distances from middle on y
#define backdist 1.5f //back distance from middle

#define EndYMin 8
#define EndYMax 9
#define EndXMin 5
#define EndXMax 6

//global variables
Camera3D camera;

//functions
void UpdateDrawFrame(void);
int getPin(int pin);
void setPin(int pin, float value);
void Log(string text);
void DrawMarker(Vector2 pos);
float GetDistance(Vector2 start, Vector2 end, Vector2 sensorPos, float rot);
float deg_cos(float angle);
float deg_sin(float angle);
float deg_tan(float angle);
bool doIntersect(Vector2 p1, Vector2 q1, Vector2 p2, Vector2 q2);

//maze values
#define MAZE_SIZE 22
#define MAZE_THICKNESS 2
#define POINTS 35
float mazePoints[POINTS][2][2] = {
        {{0, 0}, {0, 8}},
        {{0, 0}, {3, 0}},
        {{3, 0}, {3, 1}},
        {{4, 0}, {8, 0}},
        {{8, 0}, {8, 8}},
        {{8, 8}, {6, 8}},
        {{5, 8}, {0, 8}},
        {{2, 1}, {5, 1}},
        {{5, 1}, {5, 5}},
        {{5, 5}, {3, 5}},
        {{5, 2}, {3, 2}},
        {{3, 2}, {3, 4}},
        {{4, 5}, {4, 3}},
        {{0, 3}, {1, 3}},
        {{0, 4}, {1, 4}},
        {{0, 6}, {1, 6}},
        {{1, 1}, {1, 2}},
        {{1, 2}, {2, 2}},
        {{2, 2}, {2, 7}},
        {{2, 5}, {1, 5}},
        {{1, 7}, {5, 7}},
        {{2, 6}, {6, 6}},
        {{6, 0}, {6, 5}},
        {{6, 3}, {7, 3}},
        {{7, 3}, {7, 1}},
        {{8, 4}, {7, 4}},
        {{7, 4}, {7, 6}},
        {{7, 8}, {7, 7}},
        {{6, 8}, {6, 6}},

        {{3, 0}, {3, -1}},
        {{3, -1}, {4, -1}},
        {{4, -1}, {4, 0}},
        {{5, 8}, {5, 9}},
        {{5, 9}, {6, 9}},
        {{6, 9}, {6, 8}}
};

Model robotModel;

# define MAX_RANGE 128
# define ROTATION_MULTIPLIER 3

bool mapped = false;

int main() {
    //window
    int resolution = 65;
    int screenWidth = 16 * resolution;
    int screenHeight = 9 * resolution;
    InitWindow(screenWidth, screenHeight, "RoboKnights Make-a-thon");

    //camera
    camera.position = (Vector3) {MAZE_SIZE * 4, 230.0f, MAZE_SIZE * 4};
    camera.target = (Vector3) {MAZE_SIZE * 4, 0.0f, MAZE_SIZE * 4};
    camera.up = (Vector3) {90.0f, 0, 0.0f};
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    SetCameraMode(camera, CAMERA_FREE);

    setPin(11, 0);
    setPin(13, 0);

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

    //robot
    Vector3 pos = {x, 0, y};
    float prevX = x;
    float prevY = y;
    float prevRot = rotation;

    //3d
    BeginMode3D(camera);
    DrawGrid(200, 4);
    //draw end
    Vector3 endPos = {(EndXMax + EndXMin) * MAZE_SIZE / 2, 0, (EndYMax + EndYMin) * MAZE_SIZE / 2};
    DrawCube(endPos, MAZE_SIZE + MAZE_THICKNESS, 2, MAZE_SIZE + MAZE_THICKNESS, BLUE);
    //draw start
    Vector3 startPos = {xDefault, 0, yDefault};
    DrawCube(startPos, MAZE_SIZE + MAZE_THICKNESS, 2, MAZE_SIZE + MAZE_THICKNESS, RED);

    //draw robot
    DrawModelEx(robotModel, pos, (Vector3){0, 1, 0}, rotation, (Vector3){2, 2, 2}, WHITE);

    //robot movement
    bool move = !debug ? getPin(0) : IsKeyDown(KEY_W);
    bool pin1 = !debug ? getPin(1) : IsKeyDown(KEY_A);
    bool pin2 = !debug ? getPin(2) : IsKeyDown(KEY_D);

    if (!debug){
        float newSpeed = 0;
        for (int i = 3; i < 11; i++){
            int pin = getPin(i);
            if (pin == 1){
                newSpeed += pow(2, (i - 3));
            }
        }
        speed = (newSpeed / 255) * maxForwardSpeed;
        rotationSpeed = (newSpeed / 255) * maxRotSpeed;
    }

    if (move){
        if (!pin1 && !pin2){
            x += deg_sin(rotation) * speed * GetFrameTime();
            y += deg_cos(rotation) * speed * GetFrameTime();
        }
        if (pin1 && pin2){
            x -= deg_sin(rotation) * speed * GetFrameTime();
            y -= deg_cos(rotation) * speed * GetFrameTime();
        }
        if (pin1 && !pin2){
            rotation += rotationSpeed * GetFrameTime();
        }
        if (!pin1 && pin2){
            rotation -= rotationSpeed * GetFrameTime();
        }
    }

    //robot lines
    //WHAT IS THIS VARIABLE NAMING (if it works don't try to understand or change it)
    float biggerHypotenuse = sqrt(ssxdist * ssxdist + fsdist * fsdist);
    float biggerSubAngle = asin(ssxdist / biggerHypotenuse) * 180 / PI;
    float biggerAngle1 = rotation - biggerSubAngle;
    Vector2 topCorner1 = {x + deg_sin(biggerAngle1) * biggerHypotenuse, y + deg_cos(biggerAngle1) * biggerHypotenuse};
    float biggerAngle2 = 90 - (rotation + biggerSubAngle);
    Vector2 topCorner2 = {x + deg_cos(biggerAngle2) * biggerHypotenuse, y + deg_sin(biggerAngle2) * biggerHypotenuse};

    float smallerHypotenuse = -sqrt(ssxdist * ssxdist + backdist * backdist);
    float smallerSubAngle = asin(ssxdist / smallerHypotenuse) * 180 / PI;
    float smallerAngle1 = rotation - smallerSubAngle;
    Vector2 bottomCorner1 = {x + deg_sin(smallerAngle1) * smallerHypotenuse, y + deg_cos(smallerAngle1) * smallerHypotenuse};
    float smallerAngle2 = 90 - (rotation + smallerSubAngle);
    Vector2 bottomCorner2 = {x + deg_cos(smallerAngle2) * smallerHypotenuse, y + deg_sin(smallerAngle2) * smallerHypotenuse};

    DrawMarker(topCorner1);
    DrawMarker(topCorner2);

    Vector2 leftSensorX = { x + deg_sin(90 - rotation) * ssxdist , y - deg_cos(90 - rotation) * ssxdist };
    Vector2 rightSensorX = { x - deg_sin(90 - rotation) * ssxdist , y + deg_cos(90 - rotation) * ssxdist };
    Vector2 leftSensorPos = { leftSensorX.x + deg_sin(rotation) * ssydist, leftSensorX.y + deg_cos(rotation) * ssydist };
    Vector2 rightSensorPos = { rightSensorX.x + deg_sin(rotation) * ssydist, rightSensorX.y + deg_cos(rotation) * ssydist };
    float forwardMin = -1;
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
        //forward
        float forward = GetDistance(start, end, topCorner1, rotation);
        if (forward > 0 && (forward < forwardMin || forwardMin == -1)){
            forwardMin = forward;
        }
        float forward2 = GetDistance(start, end, topCorner2, rotation);
        if (forward2 > 0 && (forward2 < forwardMin || forwardMin == -1)){
            forwardMin = forward2;
        }
        //left
        float left = GetDistance(start, end, leftSensorPos, rotation + 90);
        if (left > 0 && (left < leftMin || leftMin == -1)){
            leftMin = left;
        }
        //right
        float right = GetDistance(start, end, rightSensorPos, rotation - 90);
        if (right > 0 && (right < rightMin || rightMin == -1)){
            rightMin = right;
        }
    }

    //set pins
    setPin(0, forwardMin > MAX_RANGE ? 5 : (forwardMin * 5) / MAX_RANGE);
    setPin(1, rightMin > MAX_RANGE ? 5 : (rightMin * 5) / MAX_RANGE);
    setPin(2, leftMin > MAX_RANGE ? 5 : (leftMin * 5) / MAX_RANGE);
    setPin(3, ((((int)rotation % 360 + (((int)rotation % 360) > 0 ? 0 : 360))) / 1023.0) * 5);

    //intersection checking
    Vector2 robotEdges[4][2] = {
            { bottomCorner1, bottomCorner2 },
            { bottomCorner1, topCorner1 },
            { topCorner1, topCorner2 },
            { topCorner2, bottomCorner2 },
    };
    //check each maze
    bool intersecting = false;
    for (int i = 0; i < POINTS; i++){
        Vector2 start = { mazePoints[i][0][0] * MAZE_SIZE, mazePoints[i][0][1] * MAZE_SIZE };
        Vector2 end = { mazePoints[i][1][0] * MAZE_SIZE, mazePoints[i][1][1] * MAZE_SIZE };
        if (start.x == end.x){
            Vector2 point1 = {start.x, 0};
            Vector2 point2 = {start.x, 0};
            if (start.y > end.y){
                point1.y = start.y + MAZE_THICKNESS / 2;
                point2.y = end.y - MAZE_THICKNESS / 2;
            }
            if (start.y < end.y){
                point1.y = end.y + MAZE_THICKNESS / 2;
                point2.y = start.y - MAZE_THICKNESS / 2;
            }
            Vector2 line1s = {point1.x + MAZE_THICKNESS / 2, point1.y};
            Vector2 line1e = {point2.x + MAZE_THICKNESS / 2, point2.y};
            Vector2 line2s = {point1.x - MAZE_THICKNESS / 2, point1.y};
            Vector2 line2e = {point2.x - MAZE_THICKNESS / 2, point2.y};
            Vector2 line3s = {point1.x + MAZE_THICKNESS / 2, point1.y};
            Vector2 line3e = {point1.x - MAZE_THICKNESS / 2, point1.y};
            Vector2 line4s = {point2.x + MAZE_THICKNESS / 2, point2.y};
            Vector2 line4e = {point2.x - MAZE_THICKNESS / 2, point2.y};
            for (int j = 0; j < 4; j++){
                if (doIntersect(robotEdges[j][0], robotEdges[j][1], line1s, line1e) ||
                        doIntersect(robotEdges[j][0], robotEdges[j][1], line2s, line2e) ||
                        doIntersect(robotEdges[j][0], robotEdges[j][1], line3s, line3e) ||
                        doIntersect(robotEdges[j][0], robotEdges[j][1], line4s, line4e)){
                    intersecting = true;
                    break;
                }
            }
        }
        if (start.y == end.y){
            Vector2 point1 = {0, start.y};
            Vector2 point2 = {0, start.y};
            if (start.x > end.x){
                point1.x = start.x + MAZE_THICKNESS / 2;
                point2.x = end.x - MAZE_THICKNESS / 2;
            }
            if (start.x < end.x){
                point1.x = end.x + MAZE_THICKNESS / 2;
                point2.x = start.x - MAZE_THICKNESS / 2;
            }
            Vector2 line1s = {point1.x, point1.y + MAZE_THICKNESS / 2};
            Vector2 line1e = {point2.x, point2.y + MAZE_THICKNESS / 2};
            Vector2 line2s = {point1.x, point1.y - MAZE_THICKNESS / 2};
            Vector2 line2e = {point2.x, point2.y - MAZE_THICKNESS / 2};
            Vector2 line3s = {point1.x, point1.y + MAZE_THICKNESS / 2};
            Vector2 line3e = {point1.x, point1.y - MAZE_THICKNESS / 2};
            Vector2 line4s = {point2.x, point2.y + MAZE_THICKNESS / 2};
            Vector2 line4e = {point2.x, point2.y - MAZE_THICKNESS / 2};
            for (int j = 0; j < 4; j++){
                if (doIntersect(robotEdges[j][0], robotEdges[j][1], line1s, line1e) ||
                    doIntersect(robotEdges[j][0], robotEdges[j][1], line2s, line2e) ||
                    doIntersect(robotEdges[j][0], robotEdges[j][1], line3s, line3e) ||
                    doIntersect(robotEdges[j][0], robotEdges[j][1], line4s, line4e)){
                    intersecting = true;
                    break;
                }
            }
        }
        if (intersecting){
            break;
        }
    }

    if (intersecting){
        x = prevX;
        y = prevY;
        rotation = prevRot;
    }

    if (x > EndXMin * MAZE_SIZE && x < EndXMax * MAZE_SIZE && y > EndYMin * MAZE_SIZE && y < EndYMax * MAZE_SIZE){
        if (mapped){
            setPin(11, 1); //set mapped pin to true
            mapped = true;
        }
        else{
            setPin(13, 1); //signal finished to website
        }
        x = xDefault;
        y = yDefault;
        rotation = rotDefault;
        speed = maxForwardSpeed;
        rotationSpeed = maxRotSpeed;
    }
    if (getPin(12) == 1){ //arduino program stopped
        x = xDefault;
        y = yDefault;
        rotation = rotDefault;
        speed = maxForwardSpeed;
        rotationSpeed = maxRotSpeed;
    }

    //if (debug) Log("forward: " + to_string(forwardMin) + ", can move: " + to_string(!intersecting));

    EndMode3D();

    //text
    DrawText("RoboKnights Make-a-thon", 5, 6, 14, BLACK);
    DrawText("Made with <3 by Adi Mathur", 5, 22, 10, GRAY);

    EndDrawing();
}

float GetDistance(Vector2 start, Vector2 end, Vector2 sensorPos, float rot){
    if (start.y == end.y){
        float distance = start.y - sensorPos.y;
        float perpendLen = deg_tan(rot) * distance;
        float perpendPoint = sensorPos.x + perpendLen;
        if ((start.x > end.x && perpendPoint > end.x && perpendPoint < start.x) ||
            (start.x < end.x && perpendPoint < end.x && perpendPoint > start.x)){
            float final = (1 / deg_cos(rot)) * distance - (MAZE_THICKNESS / 2.0f);
            return final;
        }
    }
    if (start.x == end.x){
        float distance = start.x - sensorPos.x;
        rot = 90 - rot;
        float perpendLen = deg_tan(rot) * distance;
        float perpendPoint = sensorPos.y + perpendLen;
        if ((start.y > end.y && perpendPoint > end.y && perpendPoint < start.y) ||
                (start.y < end.y && perpendPoint < end.y && perpendPoint > start.y)){
            float final = (1 / deg_cos(rot)) * distance - (MAZE_THICKNESS / 2.0f);
            return final;
        }
    }
    return -1;
}

// https://www.geeksforgeeks.org/check-if-two-given-line-segments-intersect/
bool onSegment(Vector2 p, Vector2 q, Vector2 r)
{
    if (q.x <= max(p.x, r.x) && q.x >= min(p.x, r.x) &&
        q.y <= max(p.y, r.y) && q.y >= min(p.y, r.y))
        return true;

    return false;
}
int orientation(Vector2 p, Vector2 q, Vector2 r)
{
    int val = (q.y - p.y) * (r.x - q.x) -
              (q.x - p.x) * (r.y - q.y);

    if (val == 0) return 0;

    return (val > 0)? 1: 2;
}
bool doIntersect(Vector2 p1, Vector2 q1, Vector2 p2, Vector2 q2)
{
    int o1 = orientation(p1, q1, p2);
    int o2 = orientation(p1, q1, q2);
    int o3 = orientation(p2, q2, p1);
    int o4 = orientation(p2, q2, q1);
    if (o1 != o2 && o3 != o4)
        return true;
    if (o1 == 0 && onSegment(p1, p2, q1)) return true;
    if (o2 == 0 && onSegment(p1, q2, q1)) return true;
    if (o3 == 0 && onSegment(p2, p1, q2)) return true;
    if (o4 == 0 && onSegment(p2, q1, q2)) return true;
    return false;
}

float deg_sin(float angle){
    return sinf(angle * (PI / 180));
}

float deg_cos(float angle){
    return cosf(angle * (PI / 180));
}

float deg_tan(float angle){
    return tanf(angle * (PI / 180));
}

int getPin(int pin) {
    return EM_ASM_INT({
        let val = pinVal($0);
        return val;
        }, pin);
}

void setPin(int pin, float value) {
    EM_ASM({
        setPin($0, $1)
        }, pin, value);
}

void DrawMarker(Vector2 pos){
    if (!debug) return;
    DrawCube((Vector3){pos.x, 0, pos.y}, 0.3, 100, 0.3, RED);
}

void Log(string text){
    char str[10000];
    strcpy(str, text.c_str());
    EM_ASM({
        console.log(UTF8ToString($0));
    }, str);
}
