#pragma once
#include <vector>
#include <SDL3/SDL.h>
#include "midiParser.h"
#include <SDL3_ttf/SDL_ttf.h>
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
    ballDropAnimation(midiFile& midiFile, SDL_Renderer *renderer, TTF_Font*);
    ~ballDropAnimation();
    void drawBallDrop(SDL_Window* window, midiFile &midiObj, float timeDelta);
private:
    void drawBallDropSeperate(SDL_Window* window, midiFile &midiObj, int index, float startY, float height, float timeDelta);
    std::vector<actionList> unifiedActions;
    std::vector<std::vector<actionList>> seperateActions;
    std::vector<actionMetaData> unifiedAnimationFrame;
    std::vector<std::vector<actionMetaData>> seperateAnimationFrame;
    std::vector<int>currentBlock;
    std::vector<std::vector<SDL_FPoint>> particles;
    std::vector<SDL_Texture* >textTextures;
    std::vector<bool> ballLineActive;
    SDL_Renderer *renderer;
    int ballRenderY;
};