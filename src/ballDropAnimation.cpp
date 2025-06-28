#include <print>
#include "midiParser.h"
#include "ballDropAnimation.h"
#include <iostream>
#include <cmath>
#define HORIZTONAL_VELOCITY 100
#define GRAV_ACCELERATION -100
#define COEFFICIENT_OF_RESTITUTION 1
#define X_OFFSET 500
ballDropAnimation::ballDropAnimation(midiFile &midiFile)
{
    float duration;
    ballRenderY = 0;
    for (int i=0;i<midiFile.unifiedNotes.size();i++)
    {
        bool found = false;
        for (int j=0;j<seperateActions.size();j++)
        {
            if (seperateActions[j].size()==0)
            {
                seperateActions[j].push_back({midiFile.unifiedNotes[i].startTime, 0, midiFile.unifiedNotes[i].startTick, 1, midiFile.unifiedNotes[i].note, midiFile.unifiedNotes[i].track, midiFile.unifiedNotes[i].program});
                found = true;
                break;
            }
            if (std::abs(seperateActions[j].back().startTime - midiFile.unifiedNotes[i].startTime)<0.1 || midiFile.unifiedNotes[i].program !=seperateActions[j].back().program)
            {
                continue;
            } else {
                seperateActions[j].push_back({midiFile.unifiedNotes[i].startTime, 0, midiFile.unifiedNotes[i].startTick, 1, midiFile.unifiedNotes[i].note, midiFile.unifiedNotes[i].track, midiFile.unifiedNotes[i].program});
                found = true;
                break;
            }
        }
        if (!found) 
        {
            seperateActions.push_back({});
            seperateAnimationFrame.push_back({});
            currentBlock.push_back(-1);
            particles.push_back({});
            seperateActions.back().push_back({midiFile.unifiedNotes[i].startTime, 0, midiFile.unifiedNotes[i].startTick, 1, midiFile.unifiedNotes[i].note, midiFile.unifiedNotes[i].track, midiFile.unifiedNotes[i].program});
        }
    }

    for (int i=0;i<seperateActions.size();i++)
    {
        for (int j=0;j<seperateActions[i].size();j++)
        {
            double deltaTimeToNext = 0;
            if (seperateActions[i].size()>1&&j<seperateActions[i].size()-1)
            {
                deltaTimeToNext = seperateActions[i][j+1].startTime-seperateActions[i][j].startTime; 
            }
            seperateActions[i][j].deltaTimeToNext = deltaTimeToNext;   
        }
    }


    for (int i=0;i<seperateActions.size();i++)
    {
        float startVelocityY = -50;
        float startPositionY = 900;

        float newPositionY = startPositionY + startVelocityY*1 + 0.5*GRAV_ACCELERATION*1;
        float newVelocityY = -(startVelocityY + GRAV_ACCELERATION*1*COEFFICIENT_OF_RESTITUTION);

        for (int j=0;j<seperateActions[i].size();j++) {
            seperateAnimationFrame[i].push_back({startVelocityY, startPositionY, (float)seperateActions[i][j].deltaTimeToNext, (float)seperateActions[i][j].startTime});
            
            newPositionY = startPositionY + startVelocityY*seperateActions[i][j].deltaTimeToNext + 0.5*GRAV_ACCELERATION*seperateActions[i][j].deltaTimeToNext*seperateActions[i][j].deltaTimeToNext;
            newVelocityY = std::clamp(-(startVelocityY + GRAV_ACCELERATION*seperateActions[i][j].deltaTimeToNext)*COEFFICIENT_OF_RESTITUTION, -150.0, 150.0);

            startPositionY = newPositionY;
            startVelocityY = newVelocityY;
        }
    }


    float startVelocityY = -50;
    float startPositionY = 900;

    float newPositionY = startPositionY + startVelocityY*1 + 0.5*GRAV_ACCELERATION*1;
    float newVelocityY = -(startVelocityY + GRAV_ACCELERATION*1*COEFFICIENT_OF_RESTITUTION);
    
    
    //startPositionY = newPositionY;
    //startVelocityY = newVelocityY;
    

    std::cout<<seperateActions.size()<<" drop\n";
}


//thx wikipedia https://en.wikipedia.org/w/index.php?title=Midpoint_circle_algorithm&oldid=889172082#C_example
void drawcircleHere(SDL_Renderer* renderer, int x0, int y0, int radius)
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


void ballDropAnimation::drawBallDrop(SDL_Window *window, SDL_Renderer *renderer, midiFile &midiObj, float timeDelta)
{
    int screenWidth = 2000;
    int screenHeight = 1000;
    SDL_GetWindowSizeInPixels(window, &screenWidth, &screenHeight);
    for (int i=0;i<seperateAnimationFrame.size();i++)
    {
        drawBallDropSeperate(window, renderer, midiObj, i, screenHeight/seperateAnimationFrame.size()*i, screenHeight/seperateAnimationFrame.size(), timeDelta);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
        SDL_RenderLine(renderer, 0, screenHeight/seperateAnimationFrame.size()*i, screenWidth, screenHeight/seperateAnimationFrame.size()*i);
    }
}

void ballDropAnimation::drawBallDropSeperate(SDL_Window *window, SDL_Renderer *renderer, midiFile &midiObj, int index, float startY, float height, float timeDelta)
{
    int screenWidth = 2000;
    int screenHeight = 1000;
    SDL_GetWindowSizeInPixels(window, &screenWidth, &screenHeight);
    for (int i=0;i<seperateActions[index].size();i++)
    {
        if ((midiObj.currentTime >= seperateAnimationFrame[index][i].startTime && midiObj.currentTime <= (seperateAnimationFrame[index][i].startTime+seperateAnimationFrame[index][i].duration)) || (midiObj.currentTime >= seperateAnimationFrame[index][i].startTime && i==seperateActions[index].size()-1))
        {
            float physicTime = midiObj.currentTime - seperateAnimationFrame[index][i].startTime;
            float positionY = seperateAnimationFrame[index][i].startPositionY + seperateAnimationFrame[index][i].startVelocityY*physicTime+0.5*GRAV_ACCELERATION*physicTime*physicTime;
            ballRenderY = positionY;

            SDL_FPoint particle = {X_OFFSET, height-positionY};
            particles[index].push_back(particle);
            if (particles[index].size()>200)
            {
                particles[index].erase(particles[index].begin());
            }
            SDL_FRect rect = {X_OFFSET, startY + height/2, 10, 10};
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, SDL_ALPHA_OPAQUE);
            SDL_RenderRect(renderer, &rect);
            //drawcircleHere(renderer, X_OFFSET+5, startY + height/2, 5);
            currentBlock[index] = i;
            break;
        }
        if (i==seperateActions[index].size()-1)
        {
            currentBlock[index] = i;
        }
    }
    
    for (int i=0;i<particles[index].size();i++)
    {
        particles[index][i].x-=HORIZTONAL_VELOCITY/(1/timeDelta*1000);
        float positionY = particles[index][i].y;
        positionY += (height/2-(height - ballRenderY))+startY;
        if (particles[index][i].x>screenWidth || particles[index][i].x<0 || positionY+10>startY+height || positionY<startY)
        {
            continue;
        }
        SDL_FPoint particle = {particles[index][i].x, positionY};
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderPoint(renderer, particle.x, particle.y);
    }
    //SDL_RenderPoints(renderer, &particles[index][0], particles[index].size());
    
    for (int i=0;i<seperateActions[index].size();i++)
    {
        float positionX = seperateActions[index][i].startTime*HORIZTONAL_VELOCITY-(float)midiObj.currentTime*HORIZTONAL_VELOCITY+X_OFFSET;
        float positionY = height-seperateAnimationFrame[index][i].startPositionY;
        float mutliplier = 1+(i<=currentBlock[index])*((midiObj.currentTime-seperateActions[index][i].startTime)*4);
        if (seperateAnimationFrame[index][i].startVelocityY>0)
        {
            positionY+=10*mutliplier;
        } else 
        {
            positionY-=5*mutliplier;    
        }
        positionY += (height/2-(height - ballRenderY))+startY;
        if (positionX>screenWidth || positionX<0 || positionY+10>startY+height || positionY<startY)
        {
            continue;
        }
        SDL_FRect rect = {positionX, positionY, 10, 5};
        SDL_SetRenderDrawColor(renderer, (float)seperateActions[index][i].note/127*205+50, (float)seperateActions[index][i].note/127*205+50, (float)seperateActions[index][i].note/127*205+50, SDL_ALPHA_OPAQUE);
        if (i<=currentBlock[index])
        {
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, SDL_ALPHA_OPAQUE);
        }
        SDL_RenderFillRect(renderer, &rect);
    }
}
