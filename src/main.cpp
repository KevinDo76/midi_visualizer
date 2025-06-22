#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <iostream>
#include <stdint.h>
#include <cstdlib>
#include "midi_parser.h"
#include <chrono>
#include <thread>
#include <cmath>
int main()
{
    bool appRunning=true;
    const int maxFrameRate = 60;
    const float minFrameTime = (1.0f/maxFrameRate)*1000*1000;
    int screenWidth = 2000;
    int screenHeight = 1000;
    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;
    std::string midiFilePath = "assets/midi/gold.mid";
    setenv("SDL_SOUNDFONTS", "assets/midi/FluidR3_GM.sf2", 1);
    setenv("SDL_AUDIODRIVER", "pulseaudio", 1);

    midiFile midiObj(midiFilePath);

    SDL_SetAppMetadata("Geometrizer", "0.0", "Geometrizer");
    
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    
    if (!SDL_CreateWindowAndRenderer("Geometrizer", 2000, 1000, 0, &window, &renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }


    if (!Mix_Init(MIX_INIT_MID)) {
        SDL_Log("Mix_Init failed: %s", SDL_GetError());
        return 1;
    }

    SDL_AudioSpec spec;
    SDL_zero(spec);  // Zero out the struct
    spec.freq = 44100;
    spec.format = SDL_AUDIO_S16;
    spec.channels = 1;

    if (!Mix_OpenAudio(0, &spec)) {
        SDL_Log("Mix_OpenAudio failed: %s", SDL_GetError());
        return 1;
    }
    
    
    SDL_SetWindowResizable(window, true);

    Mix_Music* music = Mix_LoadMUS(midiFilePath.c_str());
    if (!music) {
        SDL_Log("Failed to load music: %s", SDL_GetError());
        return 1;
    }

    
    
    if (!Mix_PlayMusic(music, 0)) {
        SDL_Log("Failed to play music: %s", SDL_GetError());
    }
    SDL_Event currentEvent;
    std::chrono::time_point lastTime = std::chrono::high_resolution_clock::now();
    std::chrono::time_point startTime = std::chrono::high_resolution_clock::now();
    
    while (appRunning)
    {
        SDL_GetWindowSizeInPixels(window, &screenWidth, &screenHeight);
        double timeDelta = (double)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now()-lastTime).count()/1000;
        double currentTime = (double)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now()-startTime).count()/1000000;
        lastTime = std::chrono::high_resolution_clock::now();
        std::chrono::time_point startFrameTime = std::chrono::high_resolution_clock::now();
        while (SDL_PollEvent(&currentEvent)) {
            if (currentEvent.type == SDL_EVENT_QUIT)
            {
                std::cout<<"Exit requested\n";
                appRunning=false;
            }
        }

        SDL_SetRenderDrawColor(renderer, 20, 20, 20, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
        for (int i=0;i<midiObj.unifiedNotes.size();i++){
            if (currentTime>= midiObj.unifiedNotes[i].startTime && (currentTime - midiObj.unifiedNotes[i].startTime)<screenWidth/100-2) {
                SDL_FRect rect = {(float)std::fmod((float)midiObj.unifiedNotes[i].startTime*100.0, screenWidth),
                                  (127-(float)midiObj.unifiedNotes[i].note)/127.0f*screenHeight,
                                  (float)midiObj.unifiedNotes[i].duration*100,
                                  1000.0f/127.0f
                };
                SDL_SetRenderDrawColor(renderer, (1+midiObj.unifiedNotes[i].channel)*2304%256, (1+midiObj.unifiedNotes[i].channel)*3804%256, (1+midiObj.unifiedNotes[i].channel)*9432%256, SDL_ALPHA_OPAQUE);
                SDL_RenderRect(renderer,&rect);
            }
        }
        SDL_RenderLine(renderer, (float)std::fmod(currentTime*100.0f, screenWidth), 0, (float)std::fmod(currentTime*100.0f, screenWidth), screenHeight);
        SDL_RenderPresent(renderer);
        double frameTime = (double)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now()-startFrameTime).count();
        //std::cout<<"sleeping\n"<<minFrameTime-frameTime<<"\n";
        std::this_thread::sleep_for(std::chrono::microseconds((int)(minFrameTime-frameTime)));
    }


    Mix_HaltMusic();
    SDL_Quit();
    std::cout<<"SDL ended\n";
    return 0;
}