#include "raylib.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

typedef struct shape_t
{
    Vector2 position; // center in circle, upper left corner for rects
    float mass;
    float radian;
    Vector2 velocity;
    float spinningVelocity; // the speed the shape is spinning in radians/frame
    bool isGrabbed;
} shape_t;

typedef struct ball_t
{
    shape_t base;
    float radius;
} ball_t;

typedef struct rect_t
{
    shape_t base;
    Vector2 size;
} rect_t;

typedef struct balls_list_t {
    ball_t *balls;
    int max;
    int pointer;
} balls_list_t;

typedef struct rects_list_t {
    rect_t *rects;
    int max;
    int pointer;
} rects_list_t;

const int screenWidth = 1280;
const int screenHeight = 720;
const int targetFPS = 60;
const float deltaFrameTime = 1 / (float)targetFPS;
const float gravity = 9.82;
const float mapBoundraryCollisionBouce = 0.2; // 1 means no force is lost upon wall collision, everything above 1 will cause a increase in force for every collision

const Rectangle mapBoundraryUpper = {0, -1000, screenWidth, 1000};
const Rectangle mapBoundraryLower = {0, screenHeight, screenWidth, 1000};
const Rectangle mapBoundraryLeft = {-1000, 0, 1000, screenHeight};
const Rectangle mapBoundraryRight = {screenWidth, 0, 1000, screenHeight};

void AddBall(balls_list_t *balls, ball_t newBall)
{
    balls->balls[balls->pointer] = newBall;
    balls->pointer += 1;
}

void IncreaseBallsListSize(balls_list_t *balls)
{
    balls->max *= 2;
    balls->balls = realloc(balls->balls, sizeof(ball_t) * balls->max);
}

void IncreaseRectsListSize(rects_list_t *rects) {
    rects->max *= 2;
    rects->rects = realloc(rects->rects, sizeof(rect_t) * rects->max);
}

Rectangle GetRectangleFromRect_t(rect_t rect)
{
    return (Rectangle){rect.base.position.x, rect.base.position.y, rect.size.x, rect.size.y};
}

void SetMouseShapeOffset(Vector2 *mouseShapeOffset, Vector2 mousePos, Vector2 shapePos)
{
    mouseShapeOffset->x = shapePos.x - mousePos.x;
    mouseShapeOffset->y = shapePos.y - mousePos.y;
}

void MoveShapeBasedOnMousePosition(Vector2 *shapePos, Vector2 *shapeVelocity, Vector2 mouseShapeOffset)
{
    float posX = GetMousePosition().x + mouseShapeOffset.x;
    float posY = GetMousePosition().y + mouseShapeOffset.y;
    printf("new posX: %.2f - new posY: %.2f\n"
           "shapePosX: %.2f - shapePosY: %.2f\n",
           posX, posY,
           shapePos->x, shapePos->y);
    printf("velY: %.2f, velX: %.2f\n",
           shapeVelocity->y, shapeVelocity->x);
    shapeVelocity->x = (posX - shapePos->x);
    shapeVelocity->y = (posY - shapePos->y);
}

// grabs shape with mouse
int GrabShape(Vector2 mousePos, ball_t *balls, int ballsCount, rect_t *rects, int rectsCount, Vector2 *mouseShapeOffset)
{
    for (int i = 0; i < ballsCount; i++)
    {
        if (CheckCollisionPointCircle(mousePos, balls[i].base.position, balls[i].radius))
        {
            SetMouseShapeOffset(mouseShapeOffset, mousePos, balls[i].base.position);
            return i;
        }
    }
    for (int i = 0; i < rectsCount; i++)
    {
        if (CheckCollisionPointRec(mousePos, GetRectangleFromRect_t(rects[i])))
        {
            SetMouseShapeOffset(mouseShapeOffset, mousePos, rects[i].base.position);
            return i + ballsCount;
        }
    }
    return -1;
}

void Gravity(ball_t *balls, int ballsCount, rect_t *rects, int rectsCount)
{
    for (int i = 0; i < ballsCount; i++)
    {
        balls[i].base.velocity.y += gravity * deltaFrameTime;
    }
    for (int i = 0; i < rectsCount; i++)
    {
        rects[i].base.velocity.y += gravity * deltaFrameTime;
    }
}

void MoveShapeWithVelocity(Vector2 *position, Vector2 velocity)
{
    position->x += velocity.x;
    position->y += velocity.y;
}

void MoveShapes(ball_t *balls, int ballsCount, rect_t *rects, int rectsCount)
{
    for (int i = 0; i < ballsCount; i++)
    {
        // TODO add spinning of shapes
        MoveShapeWithVelocity(&balls[i].base.position, balls[i].base.velocity);
    }
    for (int i = 0; i < rectsCount; i++)
    {
        MoveShapeWithVelocity(&rects[i].base.position, rects[i].base.velocity);
    }
}

void HandleMapWallCollision(ball_t *balls, int ballsCount, rect_t *rects, int rectsCount)
{
    for (int i = 0; i < ballsCount; i++)
    {
        if (CheckCollisionCircleRec(balls[i].base.position, balls[i].radius, mapBoundraryUpper))
        {
            balls[i].base.velocity.y *= -mapBoundraryCollisionBouce;
            balls[i].base.position.y = 0 + balls[i].radius; // makes sure that the circle cannot go inside the map boundrary
        }
        else if (CheckCollisionCircleRec(balls[i].base.position, balls[i].radius, mapBoundraryLower))
        {
            balls[i].base.velocity.y *= -mapBoundraryCollisionBouce;
            balls[i].base.position.y = screenHeight - balls[i].radius; // makes sure that the circle cannot go inside the map boundrary
        }
        if (CheckCollisionCircleRec(balls[i].base.position, balls[i].radius, mapBoundraryLeft))
        {
            balls[i].base.velocity.x *= -mapBoundraryCollisionBouce;
            balls[i].base.position.x = 0 + balls[i].radius; // makes sure that the circle cannot go inside the map boundrary
        }
        else if (CheckCollisionCircleRec(balls[i].base.position, balls[i].radius, mapBoundraryRight))
        {
            balls[i].base.velocity.x *= -mapBoundraryCollisionBouce;
            balls[i].base.position.x = screenWidth - balls[i].radius; // makes sure that the circle cannot go inside the map boundrary
        }
    }
    for (int i = 0; i < rectsCount; i++)
    {
        // add later
    }
}

int main()
{
    InitWindow(screenWidth, screenHeight, "physics engine");
    SetTargetFPS(targetFPS);
    // is -1 if no shape is grabbed, otherwise will be the index of the grabbed object
    // balls have their regular index, rects have their index + ballsCount
    int selectedShape = -1;
    Vector2 mouseShapeOffset = {0, 0};
    ball_t balls[1] = {};
    int ballsCount = 1;
    rect_t rects[1] = {};
    int rectsCount = 0;
    balls[0] = (ball_t){(shape_t){(Vector2){screenWidth / 2, screenHeight / 2}, 10, 0, (Vector2){0, 0}, 0, false}, 10};

    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(LIGHTGRAY);
        if (IsKeyDown(KEY_A) && selectedShape == -1)
        {
            selectedShape = GrabShape(GetMousePosition(), balls, ballsCount, rects, rectsCount, &mouseShapeOffset);
        }
        if (IsKeyReleased(KEY_A))
        {
            selectedShape = -1;
        }
        DrawCircleV(balls[0].base.position, balls[0].radius, RED);
        Gravity(balls, ballsCount, rects, rectsCount);
        MoveShapes(balls, ballsCount, rects, rectsCount);
        HandleMapWallCollision(balls, ballsCount, rects, rectsCount);
        if (selectedShape != -1)
        {
            if (selectedShape > ballsCount - 1)
            {
                MoveShapeBasedOnMousePosition(&rects[selectedShape - ballsCount].base.position, &rects[selectedShape - ballsCount].base.velocity, mouseShapeOffset);
            }
            else
            {
                MoveShapeBasedOnMousePosition(&balls[selectedShape].base.position, &balls[selectedShape].base.velocity, mouseShapeOffset);
            }
        }
        DrawText(TextFormat("ball 1 data\n"
                            "position x:y - %.2f:%.2f\n"
                            "velocity x:y - %.2f:%.2f\n"
                            "current shape grabbed: %d\n"
                            "is holding left mouse button: %d",
                            balls[0].base.position.x, balls[0].base.position.y,
                            balls[0].base.velocity.x, balls[0].base.velocity.y,
                            selectedShape, IsKeyDown(KEY_A)),
                 20, 20, 20, DARKGREEN);
        EndDrawing();
    }
    return 0;
}