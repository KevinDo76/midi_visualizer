#include <SDL3/SDL.h>
#include <iostream>
#include <stdint.h>
#include <cstdlib>
#include "midi_parser.h"
#include <chrono>
#include <thread>
#include <sstream>
#include <cmath>
#include <fluidsynth.h>


int main()
{
    bool appRunning=true;
    const int maxFrameRate = 120;
    const float minFrameTime = (1.0f/maxFrameRate)*1000*1000;
    int screenWidth = 2000;
    int screenHeight = 1000;
    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;
    std::string midiFilePath = "assets/midi/nevada.mid";
    midiFile midiObj(midiFilePath);


    SDL_SetAppMetadata("Midi Visualizer", "0.0", "Midi Visualizer");
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    
    if (!SDL_CreateWindowAndRenderer("Midi Visualizer", 2000, 1000, 0, &window, &renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    
    SDL_SetWindowResizable(window, true);
    
    
    std::vector<SDL_FRect>rectRenderBuff[17];
    for (int i=0;i<17;i++)
    {
        rectRenderBuff[i].reserve(100);
    }
    SDL_Event currentEvent;
    
    fluid_player_play(midiObj.player);
    std::chrono::time_point startTime = std::chrono::high_resolution_clock::now();
    std::chrono::time_point lastTime = std::chrono::high_resolution_clock::now();
    uint32_t lastTick = 0;
    while (appRunning)
    {
        SDL_GetWindowSizeInPixels(window, &screenWidth, &screenHeight);
        midiObj.updateCurrentTime();


        double timeDelta = (double)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now()-lastTime).count()/1000;
        lastTime = std::chrono::high_resolution_clock::now();
        std::chrono::time_point startFrameTime = std::chrono::high_resolution_clock::now();
        while (SDL_PollEvent(&currentEvent)) {
            if (currentEvent.type == SDL_EVENT_QUIT)
            {
                std::cout<<"Exit requested\n";
                appRunning=false;
            }
        }

        SDL_SetRenderDrawColor(renderer, 5, 5, 5, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);

        for (int i=0;i<midiObj.unifiedNotes.size();i++){
            float positionX = (float)midiObj.unifiedNotes[i].startTime*60-(midiObj.currentTime*60);
            SDL_FRect rect = {positionX,
                (127-(float)midiObj.unifiedNotes[i].note)/127.0f*screenHeight,
                (float)midiObj.unifiedNotes[i].duration*60,
                1000.0f/127.0f
            };

            if (positionX<screenWidth && positionX>20) {
                rectRenderBuff[midiObj.unifiedNotes[i].channel].push_back(rect);
            } else if (positionX>=screenWidth) {
                break;
            }

            if (positionX<=20 && positionX>-midiObj.unifiedNotes[i].duration*60) {
                rectRenderBuff[16].push_back(rect);
            }
            
        }

        for (int i=0;i<16;i++)
        {
            if (rectRenderBuff[i].size()>0)
            {
                SDL_SetRenderDrawColor(renderer, (1+i)*2304%256, (1+i)*3804%256, (1+i)*9432%256, SDL_ALPHA_OPAQUE);
                SDL_RenderRects(renderer, &rectRenderBuff[i][0], rectRenderBuff[i].size()); // delightfully illegal move belike
                rectRenderBuff[i].clear();
            }
        }

        if (rectRenderBuff[16].size()>0)
        {
            SDL_SetRenderDrawColor(renderer, 0, 0,255, SDL_ALPHA_OPAQUE);
            SDL_RenderRects(renderer, &rectRenderBuff[16][0], rectRenderBuff[16].size()); // delightfully illegal move belike
            rectRenderBuff[16].clear();    
        }

        SDL_RenderLine(renderer, 20,0,20,screenHeight);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);


        std::stringstream fpsDebug;
        fpsDebug<<1/timeDelta*1000<<"FPS";
        SDL_RenderDebugText(renderer, 0,0,fpsDebug.str().c_str());
        SDL_RenderPresent(renderer);


        double frameTime = (double)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now()-startFrameTime).count();
        std::this_thread::sleep_for(std::chrono::microseconds((int)(minFrameTime-frameTime)));
    }


    SDL_Quit();
    std::cout<<"SDL ended\n";
    return 0;
}