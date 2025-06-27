#pragma once
#include "midiParser.h"
#include <SDL3/SDL.h>
struct ballStartData
{
    float spawnTime;
    float verticleLaunchVelocity;
    float flightTime;
    float horizontalSpeed;
    float finalX;
};

class ballLaunchAnimation
{
public:
    ballLaunchAnimation(SDL_Window *window, midiFile& midiObj);
    void drawBalls(SDL_Window* window, SDL_Renderer* renderer, midiFile& midiObj);
    float expectedStartTime;
private:
    std::vector<ballStartData>calculatedInitBallConfig;
};