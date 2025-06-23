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

void quiet_log_handler(int level, const char* message, void* data) {}

int main()
{
    bool appRunning=true;
    const int maxFrameRate = 120;
    const float minFrameTime = (1.0f/maxFrameRate)*1000*1000;
    int screenWidth = 2000;
    int screenHeight = 1000;
    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;
    std::string midiFilePath = "assets/midi/another love.mid";

    midiFile midiObj(midiFilePath);

    fluid_set_log_function(FLUID_PANIC, quiet_log_handler, nullptr);
    fluid_set_log_function(FLUID_ERR, quiet_log_handler, nullptr);
    fluid_set_log_function(FLUID_WARN, quiet_log_handler, nullptr);
    fluid_set_log_function(FLUID_INFO, quiet_log_handler, nullptr);
    fluid_set_log_function(FLUID_DBG, quiet_log_handler, nullptr);
    fluid_settings_t* settings = new_fluid_settings();
    fluid_synth_t* synth = new_fluid_synth(settings);
    std::string soundFont = "assets/midi/FluidR3_GM.sf2";
    if (fluid_synth_sfload(synth, soundFont.c_str(), 1) == FLUID_FAILED) {
        std::cerr << "Failed to load SoundFont: " << soundFont << "\n";
        return 1;
    }
    fluid_settings_setstr(settings, "audio.driver", "pulseaudio");
    fluid_audio_driver_t* adriver = new_fluid_audio_driver(settings, synth);
    fluid_player_t* player = new_fluid_player(synth);
    if (fluid_player_add(player, midiFilePath.c_str()) != FLUID_OK) {
        std::cerr << "Failed to load MIDI file\n";
        return 1;
    }
    fluid_settings_setnum(settings, "synth.gain", 2);

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
    
    
    std::vector<SDL_FRect>rectRenderBuff[16];
    for (int i=0;i<16;i++)
    {
        rectRenderBuff[i].reserve(100);
    }
    SDL_Event currentEvent;
    
    fluid_player_play(player);
    std::chrono::time_point startTime = std::chrono::high_resolution_clock::now();
    std::chrono::time_point lastTime = std::chrono::high_resolution_clock::now();
    double currentTime = 0;
    uint32_t lastTick = 0;
    while (appRunning)
    {
        SDL_GetWindowSizeInPixels(window, &screenWidth, &screenHeight);
        
        uint32_t currentTick = fluid_player_get_current_tick(player);
        uint32_t tickDelta = currentTick - lastTick;
        lastTick = currentTick;
        currentTime+=(tickDelta * fluid_player_get_midi_tempo(player)) / (midiObj.division * 1000000.0);
        double timeDelta = (double)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now()-lastTime).count()/1000;
        //double currentTime = (double)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now()-startTime).count()/1000000;
        std::cout<<tickDelta<<"\n";
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
            //if (currentTime>= midiObj.unifiedNotes[i].startTime && (currentTime - midiObj.unifiedNotes[i].startTime)<screenWidth/100-2) {
            if (currentTime>=midiObj.unifiedNotes[i].startTime && (currentTime - midiObj.unifiedNotes[i].startTime)<screenWidth/60-2) {
                SDL_FRect rect = {(float)std::fmod((float)midiObj.unifiedNotes[i].startTime*60, screenWidth),
                                  (127-(float)midiObj.unifiedNotes[i].note)/127.0f*screenHeight,
                                  (float)midiObj.unifiedNotes[i].duration*60,
                                  1000.0f/127.0f
                };
                rectRenderBuff[midiObj.unifiedNotes[i].channel].push_back(rect);
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

        std::stringstream fpsDebug;
        fpsDebug<<1/timeDelta*1000<<"FPS";
        SDL_RenderDebugText(renderer, 0,0,fpsDebug.str().c_str());
        SDL_RenderLine(renderer, (float)std::fmod(currentTime*60.0f, screenWidth), 0, (float)std::fmod(currentTime*60.0f, screenWidth), screenHeight);
        SDL_RenderPresent(renderer);

        double frameTime = (double)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now()-startFrameTime).count();
        std::this_thread::sleep_for(std::chrono::microseconds((int)(minFrameTime-frameTime)));
    }


    SDL_Quit();
    std::cout<<"SDL ended\n";
    return 0;
}