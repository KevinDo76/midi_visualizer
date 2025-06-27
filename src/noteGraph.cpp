#include "noteGraph.h"

noteGraph::noteGraph()
{
    for (int i=0;i<17;i++)
    {
        rectRenderBuff[i].reserve(100);
    }
}

void noteGraph::renderFrame(SDL_Window *window, SDL_Renderer *renderer, midiFile& midiObj)
{
    int screenWidth = 2000;
    int screenHeight = 1000;
    SDL_GetWindowSizeInPixels(window, &screenWidth, &screenHeight);
    for (int i=0;i<midiObj.unifiedNotes.size();i++){
            float positionX = (float)midiObj.unifiedNotes[i].startTime*60-(midiObj.currentTime*60)+20;
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
}
