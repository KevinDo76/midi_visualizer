#include <print>
#include "midiParser.h"
#include "ballDropAnimation.h"
#include <iostream>
#include <cmath>
#include <sstream>
#include <SDL3_ttf/SDL_ttf.h>
#define HORIZTONAL_VELOCITY 100
#define GRAV_ACCELERATION -100
#define COEFFICIENT_OF_RESTITUTION 1.1f
#define X_OFFSET 500
ballDropAnimation::ballDropAnimation(midiFile &midiFile, SDL_Renderer *_renderer, TTF_Font* font)
    : renderer(_renderer)
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
                seperateActions[j].push_back({midiFile.unifiedNotes[i].startTime, 0, midiFile.unifiedNotes[i].startTick, 1, midiFile.unifiedNotes[i].note, midiFile.unifiedNotes[i].track, midiFile.unifiedNotes[i].program, midiFile.unifiedNotes[i].channel});
                found = true;
                break;
            }
            if (std::abs(seperateActions[j].back().startTime - midiFile.unifiedNotes[i].startTime)<0.1 || midiFile.unifiedNotes[i].program !=seperateActions[j].back().program || midiFile.unifiedNotes[i].track !=seperateActions[j].back().track)
            {
                continue;
            } else {
                seperateActions[j].push_back({midiFile.unifiedNotes[i].startTime, 0, midiFile.unifiedNotes[i].startTick, 1, midiFile.unifiedNotes[i].note, midiFile.unifiedNotes[i].track, midiFile.unifiedNotes[i].program, midiFile.unifiedNotes[i].channel});
                found = true;
                break;
            }
        }
        if (!found) 
        {
            seperateActions.push_back({});
            seperateAnimationFrame.push_back({});
            currentBlock.push_back(0);
            particles.push_back({});
            seperateActions.back().push_back({midiFile.unifiedNotes[i].startTime, 0, midiFile.unifiedNotes[i].startTick, 1, midiFile.unifiedNotes[i].note, midiFile.unifiedNotes[i].track, midiFile.unifiedNotes[i].program, midiFile.unifiedNotes[i].channel});
            ballLineActive.push_back(false);
        }
    }

    for (int i=0;i<seperateActions.size();i++)
    {

        float startVelocityY = -50;
        float startPositionY = 900;

        float newPositionY = startPositionY + startVelocityY*1 + 0.5*GRAV_ACCELERATION*1;
        float newVelocityY = -(startVelocityY + GRAV_ACCELERATION*1*COEFFICIENT_OF_RESTITUTION);

        for (int j=0;j<seperateActions[i].size();j++) {
            float deltaTimeToNext = 0;
            if (seperateActions[i].size()>1&&j<seperateActions[i].size()-1)
            {
                deltaTimeToNext = seperateActions[i][j+1].startTime-seperateActions[i][j].startTime; 
            }
            seperateAnimationFrame[i].push_back({startVelocityY, startPositionY, deltaTimeToNext, (float)seperateActions[i][j].startTime});
            
            newPositionY = startPositionY + startVelocityY*deltaTimeToNext + 0.5*GRAV_ACCELERATION*deltaTimeToNext*deltaTimeToNext;
            newVelocityY = std::clamp(-(startVelocityY + GRAV_ACCELERATION*deltaTimeToNext)*COEFFICIENT_OF_RESTITUTION, -150.0f, 150.0f);

            startPositionY = newPositionY;
            startVelocityY = newVelocityY;
        }
    }

    for (int i=0;i<seperateAnimationFrame.size();i++)
    {
        SDL_Color textColor = {255, 255, 255, 255};  // White

        SDL_Surface *textSurface = TTF_RenderText_Solid(font, midiFile.getInstrumentName(seperateActions[i][0].program, seperateActions[i][0].channel).c_str(), 0, textColor);
        if (!textSurface) {
            SDL_Log("Text render failed: %s", SDL_GetError());
        }
        

        SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
        textTextures.push_back(textTexture);
        SDL_DestroySurface(textSurface);  // Free surface after creating texture
    }

    std::cout<<seperateActions.size()<<" drop\n";
}

ballDropAnimation::~ballDropAnimation()
{
    for (int i=0;i<textTextures.size();i++)
    {
        SDL_DestroyTexture(textTextures[i]);
    }
}

void ballDropAnimation::drawBallDrop(SDL_Window *window, midiFile &midiObj, float timeDelta)
{
    int screenWidth = 2000;
    int screenHeight = 1000;
    SDL_GetWindowSizeInPixels(window, &screenWidth, &screenHeight);
    for (int i=0;i<seperateAnimationFrame.size();i++)
    {
        drawBallDropSeperate(window, midiObj, i, screenHeight/seperateAnimationFrame.size()*i, screenHeight/seperateAnimationFrame.size(), timeDelta);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
        SDL_RenderLine(renderer, 0, screenHeight/seperateAnimationFrame.size()*i, screenWidth, screenHeight/seperateAnimationFrame.size()*i);
        if (!ballLineActive[i])
        {
            continue;
        }
        SDL_FRect destRect = { 0.0f, (float)screenHeight/seperateAnimationFrame.size()*i, 0.0f, 0.0f };
        SDL_GetTextureSize(textTextures[i], &destRect.w, &destRect.h);
        SDL_RenderTexture(renderer, textTextures[i], NULL, &destRect);
    }
}

void ballDropAnimation::drawBallDropSeperate(SDL_Window *window, midiFile &midiObj, int index, float startY, float height, float timeDelta)
{
    int screenWidth = 2000;
    int screenHeight = 1000;
    SDL_GetWindowSizeInPixels(window, &screenWidth, &screenHeight);
    for (int i=0;i<seperateActions[index].size();i++)
    {
        if ((midiObj.currentTime >= seperateAnimationFrame[index][i].startTime && midiObj.currentTime <= (seperateAnimationFrame[index][i].startTime+seperateAnimationFrame[index][i].duration)) || (midiObj.currentTime >= seperateAnimationFrame[index][i].startTime && i==seperateActions[index].size()-1))
        {
            ballLineActive[index] = true;
            float physicTime = midiObj.currentTime - seperateAnimationFrame[index][i].startTime;
            float positionY = seperateAnimationFrame[index][i].startPositionY + seperateAnimationFrame[index][i].startVelocityY*physicTime+0.5*GRAV_ACCELERATION*physicTime*physicTime;
            ballRenderY = positionY;

            SDL_FPoint particle = {X_OFFSET, height-positionY+2.5f};
            particles[index].push_back(particle);
            if (particles[index].size()>200)
            {
                particles[index].erase(particles[index].begin());
            }
            SDL_FRect rect = {X_OFFSET, startY + height/2, 10, 10};
            SDL_SetRenderDrawColor(renderer, 255, 165, 0, SDL_ALPHA_OPAQUE);
            SDL_RenderFillRect(renderer, &rect);
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
        if (particles[index][i].x>screenWidth || particles[index][i].x<0 || positionY>startY+height || positionY<startY)
        {
            continue;
        }
        SDL_FPoint particle = {particles[index][i].x, positionY};
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderPoint(renderer, particle.x, particle.y);
    }

    
    for (int i=0;i<seperateActions[index].size();i++)
    {
        if (!ballLineActive[index])
        {
            continue;
        }
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
        float x = (float)seperateActions[index][i].note/127;
        if (i<=currentBlock[index]) {
            SDL_SetRenderDrawColor(renderer, 0, 200, 200, SDL_ALPHA_OPAQUE);
        } else {
            SDL_SetRenderDrawColor(renderer, std::sin(2*3.1415*x)*127+128, std::sin(2*3.1415*x+2*3.1415/3)*127+128, sin(2*3.1415*x+4*3.1415/3)*127+128, SDL_ALPHA_OPAQUE);
        }
        SDL_RenderFillRect(renderer, &rect);
    }
}
