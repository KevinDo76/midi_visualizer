#include <print>
#include "midiParser.h"
#include "ballDropAnimation.h"
#include <iostream>
#include <cmath>
#define HORIZTONAL_VELOCITY 50
#define GRAV_ACCELERATION -100
#define COEFFICIENT_OF_RESTITUTION 1
#define X_OFFSET 500
ballDropAnimation::ballDropAnimation(midiFile &midiFile)
{
    float duration;
    ballRenderY = 0;
    int largestChord = 1;
    for (int i=0;i<midiFile.unifiedNotes.size();i++)
    {
        if (unifiedActions.size()==0)
        {
            unifiedActions.push_back({midiFile.unifiedNotes[i].startTime, 0, midiFile.unifiedNotes[i].startTick, 1});
            continue;
        }
        if (unifiedActions.back().startTime == midiFile.unifiedNotes[i].startTime)
        {
            unifiedActions.back().combinedNoteCount++;
            if (unifiedActions.back().combinedNoteCount>largestChord)
            {
                largestChord = unifiedActions.back().combinedNoteCount;
            }
        } else {
            unifiedActions.push_back({midiFile.unifiedNotes[i].startTime, 0, midiFile.unifiedNotes[i].startTick, 1});
        }
    }
    std::cout<<largestChord<<" notes\n";


    // calculate the independance frame
    for (int i=0;i<largestChord;i++)
    {
        seperateActions.push_back({});
        seperateAnimationFrame.push_back({});
        currentBlock.push_back(-1);
    }

    for (int i=0;i<midiFile.unifiedNotes.size();i++)
    {
        for (int j=0;j<largestChord;j++)
        {
            if (seperateActions[j].size()==0)
            {
                seperateActions[j].push_back({midiFile.unifiedNotes[i].startTime, 0, midiFile.unifiedNotes[i].startTick, 1});
                break;
            }
            if (seperateActions[j].back().startTime == midiFile.unifiedNotes[i].startTime)
            {
                continue;
            } else {
                seperateActions[j].push_back({midiFile.unifiedNotes[i].startTime, 0, midiFile.unifiedNotes[i].startTick, 1});
                break;
            }
        }
    }

    for (int i=0;i<largestChord;i++)
    {
        for (int j=0;j<seperateActions[i].size();j++)
        {
            double deltaTimeToNext = 0;
            if (seperateActions[i].size()>1&&j<seperateActions[i].size()-2)
            {
                deltaTimeToNext = seperateActions[i][j+1].startTime-seperateActions[i][j].startTime; 
            }
            seperateActions[i][j].deltaTimeToNext = deltaTimeToNext;   
        }
    }


    for (int i=0;i<largestChord;i++)
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


    for (int i=0;i<unifiedActions.size();i++)
    {
        double deltaTimeToNext = 0;
        if (i<unifiedActions.size()-2)
        {
            deltaTimeToNext = unifiedActions[i+1].startTime-unifiedActions[i].startTime; 
        }
        unifiedActions[i].deltaTimeToNext = deltaTimeToNext;   
    }
    //unifiedAnimationFrame.push_back({startVelocityY, startPositionY, 1, -1});
    
    float startVelocityY = -50;
    float startPositionY = 900;

    float newPositionY = startPositionY + startVelocityY*1 + 0.5*GRAV_ACCELERATION*1;
    float newVelocityY = -(startVelocityY + GRAV_ACCELERATION*1*COEFFICIENT_OF_RESTITUTION);
    
    
    //startPositionY = newPositionY;
    //startVelocityY = newVelocityY;
    

    for (int i=0;i<unifiedActions.size();i++) {
        unifiedAnimationFrame.push_back({startVelocityY, startPositionY, (float)unifiedActions[i].deltaTimeToNext, (float)unifiedActions[i].startTime});
    
        newPositionY = startPositionY + startVelocityY*unifiedActions[i].deltaTimeToNext + 0.5*GRAV_ACCELERATION*unifiedActions[i].deltaTimeToNext*unifiedActions[i].deltaTimeToNext;
        newVelocityY = -(startVelocityY + GRAV_ACCELERATION*unifiedActions[i].deltaTimeToNext)*COEFFICIENT_OF_RESTITUTION;

        startPositionY = newPositionY;
        startVelocityY = newVelocityY;

    }
    std::cout<<unifiedActions.size()<<" drop\n";
}

void ballDropAnimation::drawBallDrop(SDL_Window *window, SDL_Renderer *renderer, midiFile &midiObj)
{
    //int index = (int)(midiObj.currentTime/2)%seperateAnimationFrame.size();
    int screenWidth = 2000;
    int screenHeight = 1000;
    SDL_GetWindowSizeInPixels(window, &screenWidth, &screenHeight);
    for (int i=0;i<seperateAnimationFrame.size();i++)
    {
        drawBallDropSeperate(window, renderer, midiObj, i, screenHeight/seperateAnimationFrame.size()*i, screenHeight/seperateAnimationFrame.size());
        SDL_RenderLine(renderer, 0, screenHeight/seperateAnimationFrame.size()*i, screenWidth, screenHeight/seperateAnimationFrame.size()*i);
        //break;
    }
    return;
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
    for (int i=0;i<unifiedActions.size();i++)
    {
        if ((midiObj.currentTime >= unifiedAnimationFrame[i].startTime && midiObj.currentTime <= (unifiedAnimationFrame[i].startTime+unifiedAnimationFrame[i].duration)) || midiObj.currentTime<0)
        {
            float physicTime = midiObj.currentTime - unifiedAnimationFrame[i].startTime;
            float positionY = unifiedAnimationFrame[i].startPositionY + unifiedAnimationFrame[i].startVelocityY*physicTime+0.5*GRAV_ACCELERATION*physicTime*physicTime;
            ballRenderY = positionY;
            SDL_FRect rect = {20, screenHeight/2, 10, 10};
            SDL_RenderRect(renderer, &rect);
            break;
        }
    }


    for (int i=0;i<unifiedActions.size();i++)
    {
        float positionY = screenHeight-unifiedAnimationFrame[i].startPositionY;
        positionY += (screenHeight/2-(screenHeight - ballRenderY));
        if (unifiedAnimationFrame[i].startVelocityY>0)
        {
            positionY+=10;
        } else 
        {
            positionY-=10;    
        }
        SDL_FRect rect = {unifiedActions[i].startTime*HORIZTONAL_VELOCITY-(float)midiObj.currentTime*HORIZTONAL_VELOCITY+20, positionY, 10, 10};
        SDL_RenderRect(renderer, &rect);
    }


}

void ballDropAnimation::drawBallDropSeperate(SDL_Window *window, SDL_Renderer *renderer, midiFile &midiObj, int index, float startY, float height)
{
    int screenWidth = 2000;
    int screenHeight = 1000;
    SDL_GetWindowSizeInPixels(window, &screenWidth, &screenHeight);
    for (int i=0;i<seperateActions[index].size();i++)
    {
        if ((midiObj.currentTime >= seperateAnimationFrame[index][i].startTime && midiObj.currentTime <= (seperateAnimationFrame[index][i].startTime+seperateAnimationFrame[index][i].duration)))
        {
            float physicTime = midiObj.currentTime - seperateAnimationFrame[index][i].startTime;
            float positionY = seperateAnimationFrame[index][i].startPositionY + seperateAnimationFrame[index][i].startVelocityY*physicTime+0.5*GRAV_ACCELERATION*physicTime*physicTime;
            ballRenderY = positionY;
            SDL_FRect rect = {X_OFFSET, startY + height/2, 10, 10};
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, SDL_ALPHA_OPAQUE);
            SDL_RenderRect(renderer, &rect);
            currentBlock[index] = i;
            break;
        }
    }
    
    
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
            positionY-=10*mutliplier;    
        }
        positionY += (height/2-(height - ballRenderY))+startY;
        if (positionX>screenWidth || positionX<0 || positionY+10>startY+height || positionY<startY)
        {
            continue;
        }
        SDL_FRect rect = {positionX, positionY, 10, 10};
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
        if (i<=currentBlock[index])
        {
            SDL_SetRenderDrawColor(renderer, 100, 100, 100, SDL_ALPHA_OPAQUE);
        }
        SDL_RenderFillRect(renderer, &rect);
    }
}
