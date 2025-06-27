#include <SDL3/SDL.h>
#include <iostream>
#include <stdint.h>
#include <cstdlib>
#include "midiParser.h"
#include <chrono>
#include <thread>
#include <sstream>
#include <cmath>
#include <fluidsynth.h>
#include "noteGraph.h"
#include "ballLaunchAnimation.h"

int main()
{
    const int maxFrameRate = 120;
    const float minFrameTime = (1.0f/maxFrameRate)*1000*1000;
    const std::string midiFilePath = "assets/midi/nevada.mid";

    bool appRunning=true;
    int screenWidth = 2000;
    int screenHeight = 1000;

    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;
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

    midiFile midiObj(midiFilePath);
    noteGraph noteGraphObj;
    ballLaunchAnimation ballAnimation(window, midiObj);

    
    SDL_Event currentEvent;


    std::chrono::time_point startTime = std::chrono::high_resolution_clock::now();
    std::chrono::time_point lastTime = std::chrono::high_resolution_clock::now();
    uint32_t lastTick = 0;
    midiObj.currentTime = ballAnimation.expectedStartTime;
    midiObj.startPlayback();
    while (appRunning)
    {
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
        SDL_GetWindowSizeInPixels(window, &screenWidth, &screenHeight);
        SDL_SetRenderDrawColor(renderer, 5, 5, 5, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(renderer);

        ballAnimation.drawBalls(window, renderer, midiObj);
        noteGraphObj.renderFrame(window, renderer, midiObj);

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