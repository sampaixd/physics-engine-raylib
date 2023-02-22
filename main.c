#include "raylib.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define DEV_MODE // comment out to remove dev UI

typedef struct V64F_t
{
    double x;
    double y;
} V64F_t;
typedef struct shape_t
{
    V64F_t position; // center in circle, upper left corner for rects
    double mass;
    double radian;
    V64F_t velocity;
    double spinningVelocity; // the speed the shape is spinning in radians/frame
    bool isGrabbed;
} shape_t;

typedef struct ball_t
{
    shape_t base;
    double radius;
} ball_t;

typedef struct rect_t
{
    shape_t base;
    V64F_t size;
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
const int targetFPS = 360;
const int listStartMax = 16;

const double deltaFrameTime = 1 / (float)targetFPS;
const double gravity = 9.82 * 0;
const double mapBoundraryCollisionBouce = 1; // 1 means no force is lost upon wall collision (elastic), everything above 1 will cause a increase in force for every collision
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

Vector2 GetVector2FromV64F_t(V64F_t v64)
{
    return (Vector2){(float)v64.x, (float)v64.y};
}

Rectangle GetRectangleFromRect_t(rect_t rect)
{
    return (Rectangle){rect.base.position.x, rect.base.position.y, rect.size.x, rect.size.y};
}

void SetMouseShapeOffset(V64F_t *mouseShapeOffset, Vector2 mousePos, V64F_t shapePos)
{
    mouseShapeOffset->x = shapePos.x - mousePos.x;
    mouseShapeOffset->y = shapePos.y - mousePos.y;
}

void MoveShapeBasedOnMousePosition(V64F_t *shapePos, V64F_t *shapeVelocity, V64F_t mouseShapeOffset)
{
    double posX = GetMousePosition().x + mouseShapeOffset.x;
    double posY = GetMousePosition().y + mouseShapeOffset.y;
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
int GrabShape(Vector2 mousePos, balls_list_t *balls, rects_list_t *rects, V64F_t *mouseShapeOffset)
{
    for (int i = 0; i < balls->pointer; i++)
    {
        if (CheckCollisionPointCircle(mousePos, GetVector2FromV64F_t(balls->balls[i].base.position), balls->balls[i].radius))
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

void MoveShapeWithVelocity(V64F_t *position, V64F_t velocity)
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
        rects->rects[i].base.radian += rects->rects[i].base.spinningVelocity;
    }
}

// 1 = bigger, -1 = smaller, 0 = equal
V64F_t GetShapesOffset(V64F_t shape1, V64F_t shape2)
{
    V64F_t offset = {0, 0};
    if (shape1.x > shape2.x)
    {
        offset.x = 1;
    }
    else if (shape1.x < shape2.x)
    {
        offset.x = -1;
    }

    if (shape1.y > shape2.y)
    {
        offset.y = 1;
    }
    else if (shape1.y < shape2.y)
    {
        offset.y = -1;
    }
    return offset;
}

float GetRadianBetweenShapes(V64F_t shape1Pos, V64F_t shape2Pos)
{

    float dX = fabs(shape1Pos.x - shape2Pos.x);
    float dY = fabs(shape1Pos.y - shape2Pos.y);

    double atanRadian = atan(dX / dY);
    float radian = 0;
    V64F_t ballsOffset = GetShapesOffset(shape1Pos, shape2Pos);
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

void HandleRectBallCollision(balls_list_t *balls, rects_list_t *rects)
{
}

void CalculateChangeInAngularVelocities(rect_t *rect1, rect_t *rect2, float velocityRelativeMagnitude)
{
    float I1 = (rect1->size.y * rect1->size.y + rect1->size.x * rect1->size.x) / 12;
    float I2 = (rect2->size.y * rect2->size.y + rect2->size.x * rect2->size.x) / 12;
    rect1->base.spinningVelocity -= velocityRelativeMagnitude * rect2->base.mass * (I1 + I2) / (I1 * I2);
    rect2->base.spinningVelocity += velocityRelativeMagnitude * rect1->base.mass * (I1 + I2) / (I1 * I2);
}
V64F_t GetRectCenter(rect_t rect)
{
    return (V64F_t){rect.base.position.x + (rect.size.x / 2), rect.base.position.y + (rect.size.y / 2)};
}
void CalculateRectRectCollisionVelocities(rect_t *rect1, rect_t *rect2, float velocityRelativeMagnitude, float radian, float restitution)
{
    double impulse = 2 * rect1->base.mass * rect2->base.mass * velocityRelativeMagnitude / (rect1->base.mass + rect2->base.mass);
    V64F_t impulseVector = {impulse * cos(radian), impulse * sin(radian)};
#ifdef DEV_MODE
    printf("rects collision radian: %.2f\n", radian);
    printf("impulse: %.2f - impulse vector x val: %.2f - impulse vector y val: %.2f\n", impulse, impulseVector.x, impulseVector.y);
    printf("radian: %.2f - cos val: %.2f - sin val: %.2f\n", radian, cos(radian), sin(radian));
#endif
    rect1->base.velocity.x = ((rect1->base.mass - rect2->base.mass) * rect1->base.velocity.x + 2 * rect2->base.mass * rect2->base.velocity.x +
                              rect2->base.mass * (cos(rect1->base.radian - rect2->base.radian) * (rect1->base.velocity.x - rect2->base.velocity.x) +
                                                  sin(rect1->base.radian - rect2->base.radian) * (rect1->base.velocity.y - rect2->base.velocity.y))) /
                             (rect1->base.mass + rect2->base.mass);

    rect1->base.velocity.y = ((rect1->base.mass - rect2->base.mass) * rect1->base.velocity.y + 2 * rect2->base.mass * rect2->base.velocity.y +
                              rect2->base.mass * (cos(rect1->base.radian - rect2->base.radian) * (rect1->base.velocity.y - rect2->base.velocity.y) +
                                                  sin(rect1->base.radian - rect2->base.radian) * (rect1->base.velocity.x - rect2->base.velocity.x))) /
                             (rect1->base.mass + rect2->base.mass);

    rect2->base.velocity.x = ((rect2->base.mass - rect1->base.mass) * rect2->base.velocity.x + 2 * rect1->base.mass * rect1->base.velocity.x +
                              rect1->base.mass * (cos(rect1->base.radian - rect2->base.radian) * (rect2->base.velocity.x - rect1->base.velocity.x) +
                                                  sin(rect1->base.radian - rect2->base.radian) * (rect2->base.velocity.y - rect1->base.velocity.y))) /
                             (rect1->base.mass + rect2->base.mass);

    rect2->base.velocity.y = ((rect2->base.mass - rect1->base.mass) * rect2->base.velocity.y + 2 * rect1->base.mass * rect1->base.velocity.y +
                              rect1->base.mass * (cos(rect1->base.radian - rect2->base.radian) * (rect2->base.velocity.y - rect1->base.velocity.y) +
                                                  sin(rect1->base.radian - rect2->base.radian) * (rect2->base.velocity.x - rect1->base.velocity.x))) /
                             (rect1->base.mass + rect2->base.mass);

    // rect1->base.velocity.y = rect1->base.velocity.y + (impulseVector.x / rect1->base.mass) * restitution;
    // rect1->base.velocity.x = rect1->base.velocity.x - (impulseVector.y / rect1->base.mass) * restitution;
    // rect2->base.velocity.y = rect2->base.velocity.y - (impulseVector.x / rect2->base.mass) * restitution;
    // rect2->base.velocity.x = rect2->base.velocity.x + (impulseVector.y / rect2->base.mass) * restitution;
    printf("rect 1 velocity x:y - %.2f:%.2f\n"
           "rect 2 velocity x:y - %.2f:%.2f\n",
           rect1->base.velocity.x, rect1->base.velocity.y,
           rect2->base.velocity.x, rect2->base.velocity.y);
}

void MoveRectsApart(rect_t *rect1, rect_t *rect2, float radian)
{
    V64F_t centerRect1 = GetRectCenter(*rect1);
    V64F_t centerRect2 = GetRectCenter(*rect2);
    V64F_t collisionNormal = {centerRect2.x - centerRect1.x, centerRect2.y - centerRect1.y};
    float distanceBetweenRectCenters = sqrt(collisionNormal.x * collisionNormal.x + collisionNormal.y * collisionNormal.y);
    // float unitX = collisionNormal.x / distanceBetweenRectCenters;
    // float unitY = collisionNormal.y / distanceBetweenRectCenters;
    // collisionNormal.x /= distanceBetweenRectCenters;
    // collisionNormal.y /= distanceBetweenRectCenters;

    double overlapX = ((rect1->size.x / 2) + (rect2->size.x / 2)) - fabs(collisionNormal.x);
    double overlapY = ((rect1->size.y / 2) + (rect2->size.y / 2)) - fabs(collisionNormal.y);

    // rect1->base.position.x -= (overlapX * unitX / 2);
    // rect1->base.position.y -= (overlapY * unitY / 2);
    // rect2->base.position.x += (overlapX * unitX / 2);
    // rect2->base.position.y += (overlapY * unitY / 2);
    if (fabs(radian - PI) >= PI / 4 && fabs(radian - PI) < 3 * PI / 4) // are they left/right to each other on the unit circle
    {
        if (collisionNormal.x < 0)
        {
            rect1->base.position.x += overlapX / 2;
            rect2->base.position.x -= overlapX / 2;
        }
        else
        {
            rect1->base.position.x -= overlapX / 2;
            rect2->base.position.x += overlapX / 2;
        }
    }
    else
    {
        if (collisionNormal.y < 0)
        {
            rect1->base.position.y += overlapY / 2;
            rect2->base.position.y -= overlapY / 2;
        }
        else
        {
            rect1->base.position.y -= overlapY / 2;
            rect2->base.position.y += overlapY / 2;
        }
    }
}

void CalculateCollisionRectRect(rect_t *rect1, rect_t *rect2)
{
    // calculate necessary data
    double radian = GetRadianBetweenShapes(rect1->base.position, rect2->base.position);
    double vrelX = rect1->base.velocity.x - rect2->base.velocity.x;
    double vrelY = rect1->base.velocity.y - rect2->base.velocity.y;
    double velocityRelativeMagnitude = sqrt(vrelX * vrelX + vrelY * vrelY);
    double velMagRect1 = sqrt(rect1->base.velocity.x * rect1->base.velocity.x + rect1->base.velocity.y * rect1->base.velocity.y);
    double velMagRect2 = sqrt(rect2->base.velocity.x * rect2->base.velocity.x + rect2->base.velocity.y * rect2->base.velocity.y);
    MoveRectsApart(rect1, rect2, radian);
    CalculateRectRectCollisionVelocities(rect1, rect2, velocityRelativeMagnitude, radian, 0.6); // 1 for elastic collision
    CalculateChangeInAngularVelocities(rect1, rect2, velocityRelativeMagnitude);
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

void HandleBallsCollisionVelocityChange(ball_t *ball1, ball_t *ball2, V64F_t normalVector, V64F_t tangentVector, float distanceBetweenBalls)
{
    double unitX = normalVector.x / distanceBetweenBalls;
    double unitY = normalVector.y / distanceBetweenBalls;

    double impulse = 2 * ball1->base.mass * ball2->base.mass *
                    (ball1->base.velocity.x * unitX + ball1->base.velocity.y * unitY -
                     (ball2->base.velocity.x * unitX + ball2->base.velocity.y * unitY)) /
                    (ball1->base.mass + ball2->base.mass);

    ball1->base.velocity.x -= impulse * unitX / ball1->base.mass;
    ball1->base.velocity.y -= impulse * unitY / ball1->base.mass;
    ball2->base.velocity.x += impulse * unitX / ball2->base.mass;
    ball2->base.velocity.y += impulse * unitY / ball2->base.mass;
}

void PushBallsApart(ball_t *ball1, ball_t *ball2, V64F_t normalVector, float distanceBetweenBalls)
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

V64F_t GetNormalVector(V64F_t ball1, V64F_t ball2)
{
    return (V64F_t){ball1.x - ball2.x, ball1.y - ball2.y};
}

void CalculateCollisionBallBall(ball_t *ball1, ball_t *ball2)
{
    V64F_t normalVector = GetNormalVector(ball1->base.position, ball2->base.position);
    V64F_t tangentVector = (V64F_t){-normalVector.y, normalVector.x};
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
            if (CheckCollisionCircles(GetVector2FromV64F_t(balls->balls[i].base.position), balls->balls[i].radius, GetVector2FromV64F_t(balls->balls[j].base.position), balls->balls[j].radius))
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

void HandleMapWallCollision(balls_list_t *balls, rects_list_t *rects)
{
    for (int i = 0; i < balls->pointer; i++)
    {
        if (CheckCollisionCircleRec(GetVector2FromV64F_t(balls->balls[i].base.position), balls->balls[i].radius, mapBoundraryUpper))
        {
            balls->balls[i].base.velocity.y *= -mapBoundraryCollisionBouce;
            balls->balls[i].base.position.y = 0 + balls->balls[i].radius; // makes sure that the circle cannot go inside the map boundrary
        }
        else if (CheckCollisionCircleRec(GetVector2FromV64F_t(balls->balls[i].base.position), balls->balls[i].radius, mapBoundraryLower))
        {
            balls->balls[i].base.velocity.y *= -mapBoundraryCollisionBouce;
            balls->balls[i].base.position.y = screenHeight - balls->balls[i].radius; // makes sure that the circle cannot go inside the map boundrary
        }
        if (CheckCollisionCircleRec(GetVector2FromV64F_t(balls->balls[i].base.position), balls->balls[i].radius, mapBoundraryLeft))
        {
            balls->balls[i].base.velocity.x *= -mapBoundraryCollisionBouce;
            balls->balls[i].base.position.x = 0 + balls->balls[i].radius; // makes sure that the circle cannot go inside the map boundrary
        }
        else if (CheckCollisionCircleRec(GetVector2FromV64F_t(balls->balls[i].base.position), balls->balls[i].radius, mapBoundraryRight))
        {
            balls->balls[i].base.velocity.x *= -mapBoundraryCollisionBouce;
            balls->balls[i].base.position.x = screenWidth - balls->balls[i].radius; // makes sure that the circle cannot go inside the map boundrary
        }
    }
    for (int i = 0; i < rects->pointer; i++)
    {
        if (CheckCollisionRecs(GetRectangleFromRect_t(rects->rects[i]), mapBoundraryUpper))
        {
            rects->rects[i].base.velocity.y *= -1;
            rects->rects[i].base.position.y = 0;

            // rect_t tempMapBoundraryUpper = (rect_t){(shape_t){(V64F_t){mapBoundraryUpper.x, mapBoundraryUpper.y}, 1000, 0, (V64F_t){0, 0}, 0, false}, (V64F_t){mapBoundraryUpper.width, mapBoundraryUpper.height}};
            // CalculateCollisionRectRect(&rects->rects[i], &tempMapBoundraryUpper);
        }
        else if (CheckCollisionRecs(GetRectangleFromRect_t(rects->rects[i]), mapBoundraryLower))
        {
            rects->rects[i].base.velocity.y *= -1;
            rects->rects[i].base.position.y = screenHeight - rects->rects[i].size.y;
            // rect_t tempMapBoundraryLower = (rect_t){(shape_t){(V64F_t){mapBoundraryLower.x, mapBoundraryLower.y}, 1000, 0, (V64F_t){0, 0}, 0, false}, (V64F_t){mapBoundraryLower.width, mapBoundraryLower.height}};
            // CalculateCollisionRectRect(&rects->rects[i], &tempMapBoundraryLower);
        }
        if (CheckCollisionRecs(GetRectangleFromRect_t(rects->rects[i]), mapBoundraryLeft))
        {
            rects->rects[i].base.velocity.x *= -1;
            rects->rects[i].base.position.x = 0;

            // rect_t tempMapBoundraryLeft = (rect_t){(shape_t){(V64F_t){mapBoundraryLeft.x, mapBoundraryLeft.y}, 1000, 0, (V64F_t){0, 0}, 0, false}, (V64F_t){mapBoundraryLeft.width, mapBoundraryLeft.height}};
            // CalculateCollisionRectRect(&rects->rects[i], &tempMapBoundraryLeft);
        }
        else if (CheckCollisionRecs(GetRectangleFromRect_t(rects->rects[i]), mapBoundraryRight))
        {
            rects->rects[i].base.velocity.x *= -1;
            rects->rects[i].base.position.x = screenWidth - rects->rects[i].size.x;
            // rect_t tempMapBoundraryRight = (rect_t){(shape_t){(V64F_t){mapBoundraryRight.x, mapBoundraryRight.y}, 1000, 0, (V64F_t){0, 0}, 0, false}, (V64F_t){mapBoundraryRight.width, mapBoundraryRight.height}};
            // CalculateCollisionRectRect(&rects->rects[i], &tempMapBoundraryRight);
        }
    }
}

void DrawRects(rects_list_t rects)
{
    for (int i = 0; i < rects.pointer; i++)
    {

        Rectangle rect = GetRectangleFromRect_t(rects.rects[i]);
        // due to how DrawRectanglePro works, this is needed for collision to work correctly
        DrawRectangleRec(rect, YELLOW);
        rect.x += rect.width / 2;
        rect.y += rect.height / 2;
        //DrawRectanglePro(rect, (Vector2){rects.rects[i].size.x / 2, rects.rects[i].size.y / 2}, rects.rects[i].base.radian, BLUE);
    }
}

void DrawBalls(balls_list_t balls)
{
    for (int i = 0; i < balls.pointer; i++)
    {
        DrawCircleV(GetVector2FromV64F_t(balls.balls[i].base.position), balls.balls[i].radius, RED);
    }
}

int main()
{
    InitWindow(screenWidth, screenHeight, "physics engine");
    SetTargetFPS(targetFPS);
    // is -1 if no shape is grabbed, otherwise will be the index of the grabbed object
    // balls have their regular index, rects have their index + ballsCount
    int selectedShape = -1;
    V64F_t mouseShapeOffset = {0, 0};
    balls_list_t balls = {malloc(sizeof(ball_t) * listStartMax), listStartMax, 0};
    rects_list_t rects = {malloc(sizeof(rect_t) * listStartMax), listStartMax, 0};

    HandleAddingBallToList(&balls, (ball_t){(shape_t){(V64F_t){screenWidth / 2, screenHeight / 2}, 10, 0, (V64F_t){0, 0}, 0, false}, 50});
    HandleAddingBallToList(&balls, (ball_t){(shape_t){(V64F_t){screenWidth / 4, screenHeight / 4}, 10, 0, (V64F_t){0, 0}, 0, false}, 50});
    HandleAddingBallToList(&balls, (ball_t){(shape_t){(V64F_t){3 * screenWidth / 4, 3 * screenHeight / 4}, 10, 0, (V64F_t){0, 0}, 0, false}, 50});

    HandleAddingRectToList(&rects, (rect_t){(shape_t){(V64F_t){100, 100}, 10, 0, (V64F_t){0, 0}, 0, false}, (V64F_t){100, 100}});
    HandleAddingRectToList(&rects, (rect_t){(shape_t){(V64F_t){300, 300}, 10, 0, (V64F_t){0, 0}, 0, false}, (V64F_t){100, 100}});
    HandleAddingRectToList(&rects, (rect_t){(shape_t){(V64F_t){500, 500}, 10, 0, (V64F_t){0, 0}, 0, false}, (V64F_t){100, 100}});
    while (!WindowShouldClose())
    {
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && selectedShape == -1)
        {
            selectedShape = GrabShape(GetMousePosition(), &balls, &rects, &mouseShapeOffset);
        }
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
        {
            selectedShape = -1;
        }
        if (IsKeyPressed(KEY_R))
        {
            rects.rects[0].base.position = (V64F_t){100, 100};
            rects.rects[1].base.position = (V64F_t){300, 300};
            rects.rects[2].base.position = (V64F_t){500, 500};
            rects.rects[0].base.velocity = (V64F_t){0, 0};
            rects.rects[1].base.velocity = (V64F_t){0, 0};
            rects.rects[2].base.velocity = (V64F_t){0, 0};
            rects.rects[0].base.spinningVelocity = 0;
            rects.rects[1].base.spinningVelocity = 0;
            rects.rects[2].base.spinningVelocity = 0;
            rects.rects[0].base.radian = 0;
            rects.rects[1].base.radian = 0;
            rects.rects[2].base.radian = 0;
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
        DrawText(TextFormat("rect 1 data\n"
                            "position x:y - %.2f:%.2f\n"
                            "velocity x:y - %.2f:%.2f\n"
                            "spinning velocity: %.2f\n"
                            "is holding left mouse button: %d",
                            rects.rects[0].base.position.x, rects.rects[0].base.position.y,
                            rects.rects[0].base.velocity.x, rects.rects[0].base.velocity.y,
                            rects.rects[0].base.spinningVelocity, IsKeyDown(KEY_A)),
                 20, 200, 20, PURPLE);
#endif
        EndDrawing();
    }
    // free lists
    free(balls.balls);
    free(rects.rects);
    return 0;
}