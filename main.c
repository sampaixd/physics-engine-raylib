#include "raylib.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define DEV_MODE // comment out to remove dev UI
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

typedef struct balls_list_t
{
    ball_t *balls;
    int max;
    int pointer;
} balls_list_t;

typedef struct rects_list_t
{
    rect_t *rects;
    int max;
    int pointer;
} rects_list_t;

const int screenWidth = 1280;
const int screenHeight = 720;
const int targetFPS = 60;
const int listStartMax = 16;

const float deltaFrameTime = 1 / (float)targetFPS;
const float gravity = 9.82 * 0;
const float mapBoundraryCollisionBouce = 1; // 1 means no force is lost upon wall collision (elastic), everything above 1 will cause a increase in force for every collision
// 1000 units tolerance for the velocity, if it goes beyound that it's possible for the shapes to go out of bounds
// NOTE! the corners don't have this tolerance as of now, might fix that later
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

void HandleAddingBallToList(balls_list_t *balls, ball_t newBall)
{
    if (balls->pointer >= balls->max)
    {
        IncreaseBallsListSize(balls);
    }
    AddBall(balls, newBall);
}

void AddRect(rects_list_t *rects, rect_t newRect)
{
    rects->rects[rects->pointer] = newRect;
    rects->pointer += 1;
}

void IncreaseRectsListSize(rects_list_t *rects)
{
    rects->max *= 2;
    rects->rects = realloc(rects->rects, sizeof(rect_t) * rects->max);
}

void HandleAddingRectToList(rects_list_t *rects, rect_t newRect)
{
    if (rects->pointer >= rects->max)
    {
        IncreaseRectsListSize(rects);
    }
    AddRect(rects, newRect);
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
#ifdef DEV_MODE
    printf("new posX: %.2f - new posY: %.2f\n"
           "shapePosX: %.2f - shapePosY: %.2f\n",
           posX, posY,
           shapePos->x, shapePos->y);
    printf("velY: %.2f, velX: %.2f\n",
           shapeVelocity->y, shapeVelocity->x);
#endif
    shapeVelocity->x = (posX - shapePos->x);
    shapeVelocity->y = (posY - shapePos->y);
}

// grabs shape with mouse
int GrabShape(Vector2 mousePos, balls_list_t *balls, rects_list_t *rects, Vector2 *mouseShapeOffset)
{
    for (int i = 0; i < balls->pointer; i++)
    {
        if (CheckCollisionPointCircle(mousePos, balls->balls[i].base.position, balls->balls[i].radius))
        {
            SetMouseShapeOffset(mouseShapeOffset, mousePos, balls->balls[i].base.position);
            return i;
        }
    }
    for (int i = 0; i < rects->pointer; i++)
    {
        if (CheckCollisionPointRec(mousePos, GetRectangleFromRect_t(rects->rects[i])))
        {
            SetMouseShapeOffset(mouseShapeOffset, mousePos, rects->rects[i].base.position);
            return i + balls->pointer;
        }
    }
    return -1;
}

void Gravity(balls_list_t *balls, rects_list_t *rects)
{
    for (int i = 0; i < balls->pointer; i++)
    {
        balls->balls[i].base.velocity.y += gravity * deltaFrameTime;
    }
    for (int i = 0; i < rects->pointer; i++)
    {
        rects->rects[i].base.velocity.y += gravity * deltaFrameTime;
    }
}

void MoveShapeWithVelocity(Vector2 *position, Vector2 velocity)
{
    position->x += velocity.x;
    position->y += velocity.y;
}

void MoveShapes(balls_list_t *balls, rects_list_t *rects)
{
    // TODO add spinning of shapes
    for (int i = 0; i < balls->pointer; i++)
    {
        MoveShapeWithVelocity(&balls->balls[i].base.position, balls->balls[i].base.velocity);
    }
    for (int i = 0; i < rects->pointer; i++)
    {
        MoveShapeWithVelocity(&rects->rects[i].base.position, rects->rects[i].base.velocity);
    }
}

void HandleMapWallCollision(balls_list_t *balls, rects_list_t *rects)
{
    for (int i = 0; i < balls->pointer; i++)
    {
        if (CheckCollisionCircleRec(balls->balls[i].base.position, balls->balls[i].radius, mapBoundraryUpper))
        {
            balls->balls[i].base.velocity.y *= -mapBoundraryCollisionBouce;
            balls->balls[i].base.position.y = 0 + balls->balls[i].radius; // makes sure that the circle cannot go inside the map boundrary
        }
        else if (CheckCollisionCircleRec(balls->balls[i].base.position, balls->balls[i].radius, mapBoundraryLower))
        {
            balls->balls[i].base.velocity.y *= -mapBoundraryCollisionBouce;
            balls->balls[i].base.position.y = screenHeight - balls->balls[i].radius; // makes sure that the circle cannot go inside the map boundrary
        }
        if (CheckCollisionCircleRec(balls->balls[i].base.position, balls->balls[i].radius, mapBoundraryLeft))
        {
            balls->balls[i].base.velocity.x *= -mapBoundraryCollisionBouce;
            balls->balls[i].base.position.x = 0 + balls->balls[i].radius; // makes sure that the circle cannot go inside the map boundrary
        }
        else if (CheckCollisionCircleRec(balls->balls[i].base.position, balls->balls[i].radius, mapBoundraryRight))
        {
            balls->balls[i].base.velocity.x *= -mapBoundraryCollisionBouce;
            balls->balls[i].base.position.x = screenWidth - balls->balls[i].radius; // makes sure that the circle cannot go inside the map boundrary
        }
    }
    for (int i = 0; i < rects->pointer; i++)
    {
        // add later
    }
}

void HandleRectBallCollision(balls_list_t *balls, rects_list_t *rects)
{
}

void CalculateChangeInAngularVelocities(rect_t *rect1, rect_t *rect2, float I1, float I2, float velocityRelativeMagnitude)
{
    rect1->base.spinningVelocity -= velocityRelativeMagnitude * rect2->base.mass * (I1 + I2) / (I1 * I2);
    rect2->base.spinningVelocity += velocityRelativeMagnitude * rect1->base.mass * (I1 + I2) / (I1 * I2);
    
}

void CalculateRectRectCollisionVelocities(rect_t *rect1, rect_t *rect2, float velocityRelativeMagnitude, float angle, float restitution)
{
    float impulse = 2 * rect1->base.mass * rect2->base.mass * velocityRelativeMagnitude / (rect1->base.mass + rect2->base.mass);
    Vector2 impulseVector = {impulse * cos(angle), impulse * sin(angle)};
    rect1->base.velocity.x -= impulseVector.x * restitution / rect1->base.mass;
    rect1->base.velocity.y -= impulseVector.y * restitution / rect1->base.mass;
    rect2->base.velocity.x += impulseVector.x * restitution / rect2->base.mass;
    rect2->base.velocity.y += impulseVector.y * restitution / rect2->base.mass;
    
}

void CalculateCollisionRectRect(rect_t *rect1, rect_t *rect2)
{
    // calculate necessary data
    float I1 = (rect1->size.y * rect1->size.y + rect1->size.x * rect1->size.x) / 12;
    float I2 = (rect2->size.y * rect2->size.y + rect2->size.x * rect2->size.x) / 12;
    float vrelX = rect1->base.velocity.x - rect2->base.velocity.x;
    float vrelY = rect1->base.velocity.y - rect2->base.velocity.y;
    float velocityRelativeMagnitude = sqrt(vrelX * vrelX + vrelY * vrelY);
    float angle = atan2(vrelY, vrelY);
    float velMagRect1 = sqrt(rect1->base.velocity.x * rect1->base.velocity.x + rect1->base.velocity.y * rect1->base.velocity.y);
    float velMagRect2 = sqrt(rect2->base.velocity.x * rect2->base.velocity.x + rect2->base.velocity.y * rect2->base.velocity.y);
    CalculateRectRectCollisionVelocities(rect1, rect2, velocityRelativeMagnitude, angle, 1);    // 1 for elastic collision
    CalculateChangeInAngularVelocities(rect1, rect2, I1, I2, velocityRelativeMagnitude);
}

void HandleRectRectCollision(rects_list_t *rects)
{
    for (int i = 0; i < rects->pointer - 1; i++)
    {
        for (int j = i + 1; j < rects->pointer; j++)
        {
            if (CheckCollisionRecs(GetRectangleFromRect_t(rects->rects[i]), GetRectangleFromRect_t(rects->rects[j])))
            {
                CalculateCollisionRectRect(&rects->rects[i], &rects->rects[j]);
            }
        }
    }
}

void HandleBallsCollisionVelocityChange(ball_t *ball1, ball_t *ball2, Vector2 normalVector, Vector2 tangentVector, float distanceBetweenBalls)
{
    float unitX = normalVector.x / distanceBetweenBalls;
    float unitY = normalVector.y / distanceBetweenBalls;

    float impulse = 2 * ball1->base.mass * ball2->base.mass *
                    (ball1->base.velocity.x * unitX + ball1->base.velocity.y * unitY -
                     (ball2->base.velocity.x * unitX + ball2->base.velocity.y * unitY)) /
                    (ball1->base.mass + ball2->base.mass);

    ball1->base.velocity.x -= impulse * unitX / ball1->base.mass;
    ball1->base.velocity.y -= impulse * unitY / ball1->base.mass;
    ball2->base.velocity.x += impulse * unitX / ball2->base.mass;
    ball2->base.velocity.y += impulse * unitY / ball2->base.mass;
}

// 1 = bigger, -1 = smaller, 0 = equal
Vector2 GetBallsOffset(Vector2 ball1, Vector2 ball2)
{
    Vector2 offset = {0, 0};
    if (ball1.x > ball2.x)
    {
        offset.x = 1;
    }
    else if (ball1.x < ball2.x)
    {
        offset.x = -1;
    }

    if (ball1.y > ball2.y)
    {
        offset.y = 1;
    }
    else if (ball1.y < ball2.y)
    {
        offset.y = -1;
    }
    return offset;
}
// currently depricated
float GetRadianBetweenBalls(ball_t *ball1, ball_t *ball2)
{

    float dX = fabs(ball1->base.position.x - ball2->base.position.x);
    float dY = fabs(ball1->base.position.y - ball2->base.position.y);

    double atanRadian = atan(dX / dY);
    float radian = 0;
    Vector2 ballsOffset = GetBallsOffset(ball1->base.position, ball2->base.position);
    if (ballsOffset.x <= 0 && ballsOffset.y >= 0)
    {
        radian = atanRadian;
    }
    // bottom right
    else if (ballsOffset.x <= 0 && ballsOffset.y <= 0)
    {
        radian = PI - atanRadian;
    }
    // bottom left
    else if (ballsOffset.x >= 0 && ballsOffset.y <= 0)
    {
        radian = PI + atanRadian;
    }
    // top left
    else
    {
        radian = (2 * PI) - atanRadian;
    }
    return radian;
}

void PushBallsApart(ball_t *ball1, ball_t *ball2, Vector2 normalVector, float distanceBetweenBalls)
{
    // get data needed
    // float radianBetweenBalls = GetRadianBetweenBalls(ball1, ball2);
    float intersectLength = ball1->radius + ball2->radius - distanceBetweenBalls;
    float unitX = normalVector.x / distanceBetweenBalls;
    float unitY = normalVector.y / distanceBetweenBalls;
    // move balls apart
    ball1->base.position.x += unitX * intersectLength / 2;
    ball1->base.position.y += unitY * intersectLength / 2;

    ball2->base.position.x -= unitX * intersectLength / 2;
    ball2->base.position.y -= unitY * intersectLength / 2;
}

Vector2 GetNormalVector(Vector2 ball1, Vector2 ball2)
{
    return (Vector2){ball1.x - ball2.x, ball1.y - ball2.y};
}

void CalculateCollisionBallBall(ball_t *ball1, ball_t *ball2)
{
    Vector2 normalVector = GetNormalVector(ball1->base.position, ball2->base.position);
    Vector2 tangentVector = (Vector2){-normalVector.y, normalVector.x};
    float distanceBetweenBalls = sqrt(normalVector.x * normalVector.x + normalVector.y * normalVector.y);

    PushBallsApart(ball1, ball2, normalVector, distanceBetweenBalls);
    HandleBallsCollisionVelocityChange(ball1, ball2, normalVector, tangentVector, distanceBetweenBalls);
}

void HandleBallBallCollision(balls_list_t *balls)
{
    for (int i = 0; i < balls->pointer - 1; i++)
    {
        for (int j = i + 1; j < balls->pointer; j++)
        {
            if (CheckCollisionCircles(balls->balls[i].base.position, balls->balls[i].radius, balls->balls[j].base.position, balls->balls[j].radius))
            {
                CalculateCollisionBallBall(&balls->balls[i], &balls->balls[j]);
            }
        }
    }
}

void HandleCollision(balls_list_t *balls, rects_list_t *rects)
{
    HandleBallBallCollision(balls);
    HandleRectRectCollision(rects);
}

void DrawRects(rects_list_t rects)
{
    for (int i = 0; i < rects.pointer; i++)
    {
        DrawRectanglePro(GetRectangleFromRect_t(rects.rects[i]), (Vector2){rects.rects[i].size.x / 2, rects.rects[i].size.y / 2}, rects.rects[i].base.radian, BLUE);
    }
}

void DrawBalls(balls_list_t balls)
{
    for (int i = 0; i < balls.pointer; i++)
    {
        DrawCircleV(balls.balls[i].base.position, balls.balls[i].radius, RED);
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
    balls_list_t balls = {malloc(sizeof(ball_t) * listStartMax), listStartMax, 0};
    rects_list_t rects = {malloc(sizeof(rect_t) * listStartMax), listStartMax, 0};

    HandleAddingBallToList(&balls, (ball_t){(shape_t){(Vector2){screenWidth / 2, screenHeight / 2}, 10, 0, (Vector2){0, 0}, 0, false}, 50});
    HandleAddingBallToList(&balls, (ball_t){(shape_t){(Vector2){screenWidth / 4, screenHeight / 4}, 10, 0, (Vector2){0, 0}, 0, false}, 50});
    HandleAddingBallToList(&balls, (ball_t){(shape_t){(Vector2){3 * screenWidth / 4, 3 * screenHeight / 4}, 10, 0, (Vector2){0, 0}, 0, false}, 50});

    HandleAddingRectToList(&rects, (rect_t){(shape_t){(Vector2){100, 100}, 10, 0, (Vector2){0, 0}, 0, false}, (Vector2){100, 100}});
    HandleAddingRectToList(&rects, (rect_t){(shape_t){(Vector2){300, 300}, 10, 0, (Vector2){0, 0}, 0, false}, (Vector2){100, 100}});
    HandleAddingRectToList(&rects, (rect_t){(shape_t){(Vector2){500, 500}, 10, 0, (Vector2){0, 0}, 0, false}, (Vector2){100, 100}});
    while (!WindowShouldClose())
    {
        if (IsKeyDown(KEY_A) && selectedShape == -1)
        {
            selectedShape = GrabShape(GetMousePosition(), &balls, &rects, &mouseShapeOffset);
        }
        if (IsKeyReleased(KEY_A))
        {
            selectedShape = -1;
        }

        HandleCollision(&balls, &rects);
        Gravity(&balls, &rects);
        MoveShapes(&balls, &rects);
        HandleMapWallCollision(&balls, &rects);
        if (selectedShape != -1)
        {
            if (selectedShape > balls.pointer - 1)
            {
                MoveShapeBasedOnMousePosition(&rects.rects[selectedShape - balls.pointer].base.position, &rects.rects[selectedShape - balls.pointer].base.velocity, mouseShapeOffset);
            }
            else
            {
                MoveShapeBasedOnMousePosition(&balls.balls[selectedShape].base.position, &balls.balls[selectedShape].base.velocity, mouseShapeOffset);
            }
        }

        BeginDrawing();

        ClearBackground(LIGHTGRAY);
        DrawBalls(balls);
        DrawRects(rects);
#ifdef DEV_MODE
        DrawText(TextFormat("ball 1 data\n"
                            "position x:y - %.2f:%.2f\n"
                            "velocity x:y - %.2f:%.2f\n"
                            "current shape grabbed: %d\n"
                            "is holding left mouse button: %d",
                            balls.balls[0].base.position.x, balls.balls[0].base.position.y,
                            balls.balls[0].base.velocity.x, balls.balls[0].base.velocity.y,
                            selectedShape, IsKeyDown(KEY_A)),
                 20, 20, 20, DARKGREEN);
#endif
        EndDrawing();
    }
    return 0;
}