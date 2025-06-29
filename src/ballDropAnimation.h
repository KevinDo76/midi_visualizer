#pragma once
#include <vector>
#include <SDL3/SDL.h>
#include "midiParser.h"
struct actionList {
    double startTime;
    double deltaTimeToNext;
    uint32_t startTick;
    int combinedNoteCount;
    int note;
    uint32_t track;
    uint32_t program;
    uint32_t channel;
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
    void drawBallDrop(SDL_Window* window, SDL_Renderer *renderer, midiFile &midiObj, float timeDelta);
private:
    void drawBallDropSeperate(SDL_Window* window, SDL_Renderer *renderer, midiFile &midiObj, int index, float startY, float height, float timeDelta);
    std::vector<actionList> unifiedActions;
    std::vector<std::vector<actionList>> seperateActions;
    std::vector<actionMetaData> unifiedAnimationFrame;
    std::vector<std::vector<actionMetaData>> seperateAnimationFrame;
    std::vector<int>currentBlock;
    std::vector<std::vector<SDL_FPoint>> particles;
    int ballRenderY;
};