#pragma once
#include <vector>
#include <SDL3/SDL.h>
#include "midiParser.h"
struct actionList {
    double startTime;
    double deltaTimeToNext;
    uint32_t startTick;
    int combinedNoteCount;
};

struct actionMetaData {
    float startVelocityY;
    float startPositionY;
    float duration;
    float startTime;
};

class ballDropAnimation
{
public:
    ballDropAnimation(midiFile& midiFile);
    void drawBallDrop(SDL_Window* window, SDL_Renderer *renderer, midiFile &midiObj);
private:
    void drawBallDropSeperate(SDL_Window* window, SDL_Renderer *renderer, midiFile &midiObj, int index, float startY, float height);
    std::vector<actionList> unifiedActions;
    std::vector<std::vector<actionList>> seperateActions;
    std::vector<actionMetaData> unifiedAnimationFrame;
    std::vector<std::vector<actionMetaData>> seperateAnimationFrame;
    std::vector<int>currentBlock;
    int ballRenderY;
};