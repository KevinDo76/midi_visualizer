#include <SDL3/SDL.h>
#include <fluidsynth.h>
#include <iostream>
#include <stdint.h>
#include <cstdlib>
#include "midiParser.h"
#include <chrono>
#include <thread>
#include <sstream>
#include <cmath>
#include "noteGraph.h"
#include "ballLaunchAnimation.h"
#include "ballDropAnimation.h"
#include "SDL3_ttf/SDL_ttf.h"
int main(int argc, char* argv[])
{
    const int maxFrameRate = 120;
    const float minFrameTime = (1.0f/maxFrameRate)*1000*1000;
    //if no path was given, default 
    std::string midiFilePath = "assets/midi/badapple.mid";
    if (argc>1)
    {
        midiFilePath = argv[1];
    }

    bool startAudio = false;
    bool appRunning=true;
    int screenWidth = 2100;
    int screenHeight = (float)screenWidth/(3.0/2.0);

    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;
    SDL_SetAppMetadata("Midi Visualizer", "0.0", "Midi Visualizer");
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    
    if (!SDL_CreateWindowAndRenderer("Midi Visualizer", screenWidth, screenHeight, 0, &window, &renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SetWindowResizable(window, false);

    if (!TTF_Init()) {
        SDL_Log("Couldn't initialise SDL_ttf: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    TTF_Font* font = TTF_OpenFont("assets/fonts/Silkscreen/Silkscreen-Bold.ttf", 18);
    if (!font) {
        SDL_Log("Couldn't open font: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_Color textColor = {255, 255, 255, 255};  // White
    SDL_Surface *startTextSurface = TTF_RenderText_Solid(font, "Press enter to start!", 0, textColor);
    SDL_Texture *startTextTexture = SDL_CreateTextureFromSurface(renderer, startTextSurface);

    midiFile midiObj(midiFilePath);
    noteGraph noteGraphObj;
    ballLaunchAnimation ballAnimation(window, midiObj);
    ballDropAnimation ballDrop(midiObj, renderer, font);
    
    SDL_Event currentEvent;


    std::chrono::time_point startTime = std::chrono::high_resolution_clock::now();
    std::chrono::time_point lastTime = std::chrono::high_resolution_clock::now();
    uint32_t lastTick = 0;
    //midiObj.currentTime = ballAnimation.expectedStartTime;
    midiObj.currentTime = -1;
    while (appRunning)
    {
        if (startAudio)
        {
            midiObj.updateCurrentTime();
        }

        double timeDelta = (double)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now()-lastTime).count()/1000;
        lastTime = std::chrono::high_resolution_clock::now();
        std::chrono::time_point startFrameTime = std::chrono::high_resolution_clock::now();
        
        while (SDL_PollEvent(&currentEvent)) {

            switch (currentEvent.type)
            {
            case SDL_EVENT_QUIT:
                std::cout<<"Exit requested\n";
                appRunning=false;
                break;
            case SDL_EVENT_KEY_DOWN:
                if (currentEvent.key.scancode==0x28)
                {
                    startAudio = true;
                    midiObj.startPlayback();
                }
                break;
            }
        }
        SDL_GetWindowSizeInPixels(window, &screenWidth, &screenHeight);
        SDL_SetRenderDrawColor(renderer, 5, 5, 5, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(renderer);

        //ballAnimation.drawBalls(window, renderer, midiObj);
        //noteGraphObj.renderFrame(window, renderer, midiObj);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
        std::stringstream fpsDebug;
        fpsDebug<<1/timeDelta*1000<<"FPS";
        //SDL_RenderDebugText(renderer, 100,0,fpsDebug.str().c_str());

        if (midiObj.currentTime == -1)
        {
            SDL_FRect destRect = { 0.0f, 0.0f, 0.0f, 0.0f };
            SDL_GetTextureSize(startTextTexture, &destRect.w, &destRect.h);
            SDL_RenderTexture(renderer, startTextTexture, NULL, &destRect);
        } else {
            ballDrop.drawBallDrop(window, midiObj, timeDelta);

        }

        SDL_RenderPresent(renderer);


        double frameTime = (double)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now()-startFrameTime).count();
        std::this_thread::sleep_for(std::chrono::microseconds((int)(minFrameTime-frameTime)));
    }

    SDL_DestroySurface(startTextSurface);
    SDL_DestroyTexture(startTextTexture);
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_Quit();
    std::cout<<"SDL ended\n";
    return 0;
}