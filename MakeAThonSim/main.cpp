#include "raylib.h"
#include "/F/Repositories/emsdk/upstream/emscripten/system/include/emscripten.h"
#include "math.h"
#include "iostream"
#include "cstring"

using namespace std;

bool debug = false;

//robot values
#define xDefault 33
#define yDefault 8

float x = xDefault;
float y = yDefault;
float rotation = 0;
float speed = 10;
float rotationSpeed = 100;
#define fsdist 6.5f //front sensor distance from middle
#define ssxdist 2 //side sensor distances from middle on x
#define ssydist 4.4f //side sensor distances from middle on y
#define backdist 1.5f //back distance from middle

#define EndYMin 3
#define EndYMax 4
#define EndXMin 3
#define EndXMax 4

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
#define POINTS 10
float mazePoints[POINTS][2][2] = {
        {{0, 0}, {1, 0}},
        {{0, 0}, {0, 3}},
        {{0, 3}, {1, 3}},
        {{0, 2}, {1, 2}},
        {{1, 1}, {2, 1}},
        {{2, 1}, {2, 0}},
        {{2, 0}, {3, 0}},
        {{3, 0}, {3, 3}},
        {{3, 3}, {2, 3}},
        {{2, 3}, {2, 2}}
};

Model robotModel;

# define MAX_RANGE 128

int main() {
    //window
    int resolution = 50;
    int screenWidth = 16 * resolution;
    int screenHeight = 9 * resolution;
    InitWindow(screenWidth, screenHeight, "raylib [core] example - basic window");

    //camera
    camera.position = (Vector3) {5, 100.0f, 0};
    camera.target = (Vector3) {0.0f, 0.0f, 0.0f};
    camera.up = (Vector3) {0.0f, 1.0f, 0.0f};
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    SetCameraMode(camera, CAMERA_FREE);

    setPin(4, 0);

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
    float prevX = x;
    float prevY = y;
    float prevRot = rotation;

    //3d
    BeginMode3D(camera);
    DrawGrid(200, 1);
    DrawModelEx(robotModel, pos, (Vector3){0, 1, 0}, rotation, (Vector3){2, 2, 2}, WHITE);

    Vector2 forwardSensorPos = { x + deg_sin(rotation) * fsdist, y + deg_cos(rotation) * fsdist };
    Vector2 leftSensorX = { x + deg_sin(90 - rotation) * ssxdist , y - deg_cos(90 - rotation) * ssxdist };
    Vector2 rightSensorX = { x - deg_sin(90 - rotation) * ssxdist , y + deg_cos(90 - rotation) * ssxdist };
    Vector2 leftSensorPos = { leftSensorX.x + deg_sin(rotation) * ssydist, leftSensorX.y + deg_cos(rotation) * ssydist };
    Vector2 rightSensorPos = { rightSensorX.x + deg_sin(rotation) * ssydist, rightSensorX.y + deg_cos(rotation) * ssydist };
    //DrawMarker(forwardSensorPos);
    //DrawMarker(leftSensorPos);
    //DrawMarker(rightSensorPos);
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
        float forward = GetDistance(start, end, forwardSensorPos, rotation);
        if (forward > 0 && (forward < forwardMin || forwardMin == -1)){
            forwardMin = forward;
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
        //right
/*        else if (left < 0){
            float rightDist = -forward - (2 * ssxdist);
            if (rightDist < rightMin || rightMin == -1){
                rightMin = rightDist;
            }
        }*/
    }

    //set pins
    setPin(0, forwardMin > MAX_RANGE ? 5 : (forwardMin * 5) / MAX_RANGE);
    setPin(1, rightMin > MAX_RANGE ? 5 : (rightMin * 5) / MAX_RANGE);
    setPin(2, leftMin > MAX_RANGE ? 5 : (leftMin * 5) / MAX_RANGE);

    //robot movement
    bool move = !debug ? getPin(0) : IsKeyDown(KEY_W);
    bool pin1 = !debug ? getPin(1) : IsKeyDown(KEY_A);
    bool pin2 = !debug ? getPin(2) : IsKeyDown(KEY_D);
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

    //intersection checking
    //robot lines
    //WHAT IS THIS VARIABLE NAMING (if it works don't try to understand or change it)
    float biggerHypotenuse = sqrt(ssxdist * ssxdist + fsdist * fsdist);
    float biggerSubAngle = asin(ssxdist / biggerHypotenuse) * 180 / PI;
    float biggerAngle1 = rotation - biggerSubAngle;
    Vector2 topCorner1 = {x + deg_sin(biggerAngle1) * biggerHypotenuse, y + deg_cos(biggerAngle1) * biggerHypotenuse};
    float biggerAngle2 = 90 - (rotation + biggerSubAngle);
    Vector2 topCorner2 = {x + deg_cos(biggerAngle2) * biggerHypotenuse, y + deg_sin(biggerAngle2) * biggerHypotenuse};

    //DrawMarker(topCorner1);
    //DrawMarker(topCorner2);

    float smallerHypotenuse = -sqrt(ssxdist * ssxdist + backdist * backdist);
    float smallerSubAngle = asin(ssxdist / smallerHypotenuse) * 180 / PI;
    float smallerAngle1 = rotation - smallerSubAngle;
    Vector2 bottomCorner1 = {x + deg_sin(smallerAngle1) * smallerHypotenuse, y + deg_cos(smallerAngle1) * smallerHypotenuse};
    float smallerAngle2 = 90 - (rotation + smallerSubAngle);
    Vector2 bottomCorner2 = {x + deg_cos(smallerAngle2) * smallerHypotenuse, y + deg_sin(smallerAngle2) * smallerHypotenuse};

    //DrawMarker(bottomCorner1);
    //DrawMarker(bottomCorner2);
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
            DrawMarker(line1s);
            DrawMarker(line1e);
            DrawMarker(line2s);
            DrawMarker(line2e);
            DrawMarker(line3s);
            DrawMarker(line3e);
            DrawMarker(line4s);
            DrawMarker(line4e);
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
            DrawMarker(line1s);
            DrawMarker(line1e);
            DrawMarker(line2s);
            DrawMarker(line2e);
            DrawMarker(line3s);
            DrawMarker(line3e);
            DrawMarker(line4s);
            DrawMarker(line4e);
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
        x = xDefault;
        y = yDefault;
        rotation = 0;
        setPin(4, 1);
    }

    //Log("left: " + to_string(leftMin) + ", right: " + to_string(rightMin) + ", forward: " + to_string(forwardMin));
    Log("x: " + to_string(x) + ", y: " + to_string(y));

    EndMode3D();

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

// Given three collinear points p, q, r, the function checks if
// point q lies on line segment 'pr'
bool onSegment(Vector2 p, Vector2 q, Vector2 r)
{
    if (q.x <= max(p.x, r.x) && q.x >= min(p.x, r.x) &&
        q.y <= max(p.y, r.y) && q.y >= min(p.y, r.y))
        return true;

    return false;
}

// To find orientation of ordered triplet (p, q, r).
// The function returns following values
// 0 --> p, q and r are collinear
// 1 --> Clockwise
// 2 --> Counterclockwise
int orientation(Vector2 p, Vector2 q, Vector2 r)
{
    // See https://www.geeksforgeeks.org/orientation-3-ordered-points/
    // for details of below formula.
    int val = (q.y - p.y) * (r.x - q.x) -
              (q.x - p.x) * (r.y - q.y);

    if (val == 0) return 0;  // collinear

    return (val > 0)? 1: 2; // clock or counterclock wise
}

// The main function that returns true if line segment 'p1q1'
// and 'p2q2' intersect.
bool doIntersect(Vector2 p1, Vector2 q1, Vector2 p2, Vector2 q2)
{
    // Find the four orientations needed for general and
    // special cases
    int o1 = orientation(p1, q1, p2);
    int o2 = orientation(p1, q1, q2);
    int o3 = orientation(p2, q2, p1);
    int o4 = orientation(p2, q2, q1);

    // General case
    if (o1 != o2 && o3 != o4)
        return true;

    // Special Cases
    // p1, q1 and p2 are collinear and p2 lies on segment p1q1
    if (o1 == 0 && onSegment(p1, p2, q1)) return true;

    // p1, q1 and q2 are collinear and q2 lies on segment p1q1
    if (o2 == 0 && onSegment(p1, q2, q1)) return true;

    // p2, q2 and p1 are collinear and p1 lies on segment p2q2
    if (o3 == 0 && onSegment(p2, p1, q2)) return true;

    // p2, q2 and q1 are collinear and q1 lies on segment p2q2
    if (o4 == 0 && onSegment(p2, q1, q2)) return true;

    return false; // Doesn't fall in any of the above cases
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