#pragma once
#include <SDL3/SDL.h>
#include <vector>
#include "midiParser.h"
class noteGraph
{
    public:
        noteGraph();
        void renderFrame(SDL_Window *window, SDL_Renderer *renderer, midiFile& midiObj);
    private:
        std::vector<SDL_FRect>rectRenderBuff[17];
};