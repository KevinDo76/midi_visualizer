#include "ballLaunchAnimation.h"
#include <SDL3/SDL.h>
#include <iostream>

#define HORIZONTAL_TRAVEL_SPEED 1000
#define START_NOTE_POSITION 400
#define GRAVITATION_ACCELERATION -50
#define DROP_HEIGHT 800

ballLaunchAnimation::ballLaunchAnimation(SDL_Window *window, midiFile &midiObj)
{
    int screenWidth = 2000;
    int screenHeight = 1000;
    SDL_GetWindowSizeInPixels(window, &screenWidth, &screenHeight);
    const float gravitationalAcceleration = -20;
    float minimumStartTime = 0;
    for (int i=0;i<midiObj.unifiedNotes.size();i++)
    {
        float horizontalSpeed = (float)rand()/(float)RAND_MAX*300+100;
        float desiredTravelDistance = START_NOTE_POSITION + (float)midiObj.unifiedNotes[i].note/127*(screenWidth-START_NOTE_POSITION);
        float travelTime = desiredTravelDistance/horizontalSpeed;
        float verticalLaunchVelocity = (-GRAVITATION_ACCELERATION*travelTime)/2;
        calculatedInitBallConfig.push_back({(float)(midiObj.unifiedNotes[i].startTime)-travelTime,verticalLaunchVelocity,travelTime,horizontalSpeed,desiredTravelDistance});
        if ((float)(midiObj.unifiedNotes[i].startTime)-travelTime<minimumStartTime)
        {
            minimumStartTime = (float)(midiObj.unifiedNotes[i].startTime)-travelTime;
        }
    }

    expectedStartTime = minimumStartTime;
}

//thx wikipedia https://en.wikipedia.org/w/index.php?title=Midpoint_circle_algorithm&oldid=889172082#C_example
void drawcircle(SDL_Renderer* renderer, int x0, int y0, int radius)
{
    int x = radius-1;
    int y = 0;
    int dx = 1;
    int dy = 1;
    int err = dx - (radius << 1);

    while (x >= y)
    {
        SDL_RenderPoint(renderer, x0 + x, y0 + y);
        SDL_RenderPoint(renderer, x0 + y, y0 + x);
        SDL_RenderPoint(renderer, x0 - y, y0 + x);
        SDL_RenderPoint(renderer, x0 - x, y0 + y);
        SDL_RenderPoint(renderer, x0 - x, y0 - y);
        SDL_RenderPoint(renderer, x0 - y, y0 - x);
        SDL_RenderPoint(renderer, x0 + y, y0 - x);
        SDL_RenderPoint(renderer, x0 + x, y0 - y);

        if (err <= 0)
        {
            y++;
            err += dy;
            dy += 2;
        }
        
        if (err > 0)
        {
            x--;
            dx += 2;
            err += dx - (radius << 1);
        }
    }
}


void ballLaunchAnimation::drawBalls(SDL_Window* window, SDL_Renderer *renderer, midiFile &midiObj)
{
    int screenWidth = 2000;
    int screenHeight = 1000;
    SDL_GetWindowSizeInPixels(window, &screenWidth, &screenHeight);
    SDL_SetRenderDrawColor(renderer, 150,150,150,255);
    SDL_RenderLine(renderer, 0, 800, screenWidth, 800);
    for (int i=0;i<128;i++)
    {
        float desiredTravelDistance = START_NOTE_POSITION + (float)i/127*(screenWidth-START_NOTE_POSITION);
        SDL_RenderLine(renderer, desiredTravelDistance, DROP_HEIGHT, desiredTravelDistance, screenHeight);
    }
    for (int i=0;i<calculatedInitBallConfig.size();i++)
    {
        float physicTime = midiObj.currentTime-calculatedInitBallConfig[i].spawnTime;
        float positionY = DROP_HEIGHT - calculatedInitBallConfig[i].verticleLaunchVelocity*physicTime-0.5*GRAVITATION_ACCELERATION*physicTime*physicTime;
        if (physicTime>=0 && physicTime<=calculatedInitBallConfig[i].flightTime)
        {
            float positionX = calculatedInitBallConfig[i].horizontalSpeed*(physicTime);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
            drawcircle(renderer, positionX, positionY, 5);
        } else if (positionY<=screenHeight && physicTime>=0) {
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, SDL_ALPHA_OPAQUE);
            drawcircle(renderer, calculatedInitBallConfig[i].finalX, positionY, 5);
        }
    }
}
