//credits:
    //was heavily relied on
        //https://www.youtube.com/watch?v=040BKtnDdg0&t=2075s - javidx9's video on MIDI
    //A healthy dose of
        //https://ccrma.stanford.edu/~craig/14q/midifile/MidiFileFormat.html
    //andd some suggestion from ChatGPT
#include "midi_parser.h"
#include <iostream>
#include <cstdint>
#include <byteswap.h>
#include <bit>
#include <array>
#include <algorithm>
#include <fluidsynth.h>
#include <exception>

#define SF2_FILE_PATH "/usr/share/soundfonts/FluidR3_GM.sf2"

midiFile::midiFile(std::string filePath)
    : inputMidi(filePath, std::ios::binary)
{
    resetCurrentTimeCounter();
    timeSignatureNumerator = 4;
    timeSignatureDenominator = 4;
    clocksPerTick = 24;
    _32per24Clocks = 8;
    Tempo = 500000;

    fluidsynthInit(filePath);
    
    uint32_t midiMagicNumber = 0;
    uint32_t headerLength = 0;
    uint16_t format = 0;
    uint16_t trackCount = 0;
    division = 0;
    inputMidi.read((char *)&midiMagicNumber, 4);
    inputMidi.read((char *)&headerLength, 4);
    inputMidi.read((char *)&format, 2);
    inputMidi.read((char *)&trackCount, 2);
    inputMidi.read((char *)&division, 2);
    
    headerLength = std::byteswap(headerLength);
    format = std::byteswap(format);
    trackCount = std::byteswap(trackCount);
    division = std::byteswap(division);

    if ((division & 0x8000)) {
        throw std::exception(); // invalid timing proto
    }

    if (midiMagicNumber == 0x6468544d)
    {
        std::cout<<"Midi header detected\n";
    } else {
        throw std::invalid_argument("Provided file is not MIDI");
    }
    std::cout<<"Format: "<<format<<" Track Count: "<<trackCount<<" Division: "<<division<<"\n";   
    
    for (int trackI=0;trackI<trackCount;trackI++)
    {
        midiTracks.emplace_back();
        bool endOfTrack = 0;
        uint32_t tickSum = 0; 
        uint8_t previousStatus = 0;
        std::string trackMagicNumber;
        uint32_t trackLength = 0;
        int eventCount=0;

        trackMagicNumber=readString(inputMidi, 4);
        std::cout<<"------"<<trackMagicNumber<<"------"<<"\n";

        inputMidi.read((char *)&trackLength, 4);
        trackLength=std::byteswap(trackLength);
        
        while (!inputMidi.eof() && !endOfTrack)
        {
            uint32_t timeDelta = readVariableAmount(inputMidi);
            tickSum+=timeDelta;
            uint8_t status = inputMidi.get();
            eventCount++;
            if (status<0x80)
            {
                status = previousStatus;
                inputMidi.seekg(-1, std::ios_base::cur);
            } else {
                previousStatus = status;
            }
            
            uint8_t eventType = status&0xF0;
            if (eventType == midiEventName::noteOff)
            {
                uint8_t channel = status&0x0F;
                uint8_t noteIndex = inputMidi.get();
                uint8_t noteVelocity = inputMidi.get();
                midiTracks[trackI].midiEvents.push_back({midiEventName::noteOff, noteIndex, noteVelocity, timeDelta, channel, tickSum, Tempo});
                
                if (noteIndex>127) {std::cout<<"high "<<eventCount<<" "<<(int)noteIndex<<"track: "<<trackI<<"\n";}
            } else if (eventType == midiEventName::noteOn) {
                uint8_t channel = status&0x0F;
                uint8_t noteIndex = inputMidi.get();
                uint8_t noteVelocity = inputMidi.get();
                if (noteVelocity > 0)
                {
                    midiTracks[trackI].midiEvents.push_back({midiEventName::noteOn, noteIndex, noteVelocity, timeDelta, channel, tickSum, Tempo});
                } else {
                    midiTracks[trackI].midiEvents.push_back({midiEventName::noteOff, noteIndex, noteVelocity, timeDelta, channel, tickSum, Tempo});
                }

                if (noteIndex>127) {std::cout<<"high "<<eventCount<<(int)noteIndex<<"track: "<<trackI<<"\n";}
            } else if (eventType == midiEventName::afterTouch) {
                uint8_t channel = status&0x0F;
                uint8_t noteIndex = inputMidi.get();
                uint8_t noteVelocity = inputMidi.get();
                midiTracks[trackI].midiEvents.emplace_back(midiEventName::afterTouch, noteIndex, noteVelocity, timeDelta, channel, tickSum, Tempo);
            } else if (eventType == midiEventName::controlChange) {
                uint8_t channel = status&0x0F;
                uint8_t noteIndex = inputMidi.get();
                uint8_t noteVelocity = inputMidi.get();
                midiTracks[trackI].midiEvents.emplace_back(midiEventName::controlChange, noteIndex, noteVelocity, timeDelta, channel, tickSum, Tempo);
            } else if (eventType == midiEventName::programChange) {
                uint8_t channel = status&0x0F;
                uint8_t programID = inputMidi.get();
                midiTracks[trackI].midiEvents.emplace_back(midiEventName::programChange, programID, 0, timeDelta, channel, tickSum, Tempo);
            } else if (eventType == midiEventName::channelPressure) {
                uint8_t channel = status&0x0F;
                uint8_t programID = inputMidi.get();
            } else if (eventType == midiEventName::pitchBend) {
                uint8_t channel = status&0x0F;
                uint8_t nLS7B = inputMidi.get();
                uint8_t nMS7B = inputMidi.get();
            } else if (eventType == midiEventName::systemExclusive) {
                previousStatus = 0;
                if (status == 0xF0)
                {
                    std::cout<<"Sys exclusive begin: "<<readString(inputMidi,readVariableAmount(inputMidi))<<"\n";
                }

                if (status == 0xF7)
                {
                    std::cout<<"Sys exclusive end: "<<readString(inputMidi,readVariableAmount(inputMidi))<<"\n";
                }
                if (status == 0xFF)
                {
                    uint8_t metaType = inputMidi.get();
                    uint32_t metaLength = readVariableAmount(inputMidi);

                    switch (metaType)
						{
						case Sequence:
							std::cout << "Sequence Number: " << inputMidi.get() << inputMidi.get() << std::endl;
							break;
						case Text:
                            std::cout<<metaLength<<":Data text\n";
							std::cout << "Text: " << readString(inputMidi, metaLength) << std::endl;
							break;
						case Copyright:
							std::cout << "Copyright: " << readString(inputMidi, metaLength) << std::endl;
							break;
						case TrackName:
                            midiTracks[trackI].trackName = readString(inputMidi, metaLength);
							std::cout << "Track Name: " << midiTracks[trackI].trackName << std::endl;							
							break;
						case InstrumentName:
							std::cout << "Instrument Name: " << readString(inputMidi, metaLength) << std::endl;
							break;
						case Lyrics:
						    //std::cout << "Lyrics: " << readString(inputMidi, metaLength) << std::endl;
                            readString(inputMidi, metaLength);
							break;
						case Marker:
							std::cout << "Marker: " << readString(inputMidi, metaLength) << std::endl;
							break;
						case CuePoint:
							std::cout << "Cue: " << readString(inputMidi, metaLength) << std::endl;
							break;
						case ChannelPrefix:
							std::cout << "Prefix: " << inputMidi.get() << std::endl;
							break;
						case EndOfTrack:
                            std::cout<<"------Track Ended------\n";
							endOfTrack = true;
							break;
						case SetTempo:
							// Tempo is in microseconds per quarter note	
                            Tempo = 0;
                            (Tempo |= (inputMidi.get() << 16));
                            (Tempo |= (inputMidi.get() << 8));
                            (Tempo |= (inputMidi.get() << 0));
       
                            std::cout << "Tempo: " << Tempo << tickSum << std::endl;
                            midiTracks[trackI].midiEvents.emplace_back(midiEventName::systemExclusive, 0, 0, timeDelta, 0, tickSum, Tempo);
                            midiTracks[trackI].midiEvents.back().metaType = midiFile::MetaEventName::SetTempo;
							break;
						case SMPTEOffset:
							std::cout << "SMPTE: H:" << inputMidi.get() << " M:" << inputMidi.get() << " S:" << inputMidi.get() << " FR:" << inputMidi.get() << " FF:" << inputMidi.get() << std::endl;
							break;
						case TimeSignature:
                            timeSignatureNumerator = inputMidi.get();
                            timeSignatureDenominator = (2 << inputMidi.get());
                            clocksPerTick = inputMidi.get();
                            _32per24Clocks = inputMidi.get();

							std::cout << "Time Signature: " << timeSignatureNumerator << "/" << timeSignatureDenominator << std::endl;
							std::cout << "ClocksPerTick: " << clocksPerTick << std::endl;
							std::cout << "32per24Clocks: " << _32per24Clocks << std::endl;
							break;
						case KeySignature:
							std::cout << "Key Signature: " << inputMidi.get() << std::endl;
							std::cout << "Minor Key: " << inputMidi.get() << std::endl;
							break;
						case SequencerSpecific:
							std::cout << "Sequencer Specific: " << readString(inputMidi, metaLength) << std::endl;
							break;
						default:
							std::cout << "Unrecognised MetaEvent: " << std::hex <<(int)metaType << std::dec << std::endl;
                            for (int i=0;i<metaLength;i++)
                            {
                                inputMidi.get();
                            }
						}
                }
            } else {
                std::cout<<"Unknown event! "<<std::hex<<(int)eventType<<std::dec<<"\n";
            }
        }
    }

    for (int trackI=0;trackI<midiTracks.size();trackI++)
    {
        for (int i=0;i<midiTracks[trackI].midiEvents.size();i++)
        {
            unifiedEvents.push_back(midiTracks[trackI].midiEvents[i]);
        }
    }

    std::sort(unifiedEvents.begin(), unifiedEvents.end(), [](midiEvent a, midiEvent b)
    {
        return a.sumTickTime<b.sumTickTime;
    });

    std::array<std::array<std::vector<playingNote>, 128>, 16> noteStateArray;

    for (int i=0;i<unifiedEvents.size();i++)
    {
        if (unifiedEvents[i].type == midiFile::midiEventName::systemExclusive && unifiedEvents[i].metaType == midiFile::MetaEventName::SetTempo)
        {
            Tempo = unifiedEvents[i].Tempo;
        } else 
        {
            unifiedEvents[i].Tempo = Tempo;
        }
    }

    //tick delta must be recalculated, this was a pain to figure out.
    uint32_t lastTick=0;
    for (int i=0;i<unifiedEvents.size();i++)
    {
        unifiedEvents[i].tickTime = unifiedEvents[i].sumTickTime-lastTick;
        lastTick = unifiedEvents[i].sumTickTime;
    }

    double secondSum = 0;
    for (int i=0;i<unifiedEvents.size();i++)
    {
        secondSum+=((double)unifiedEvents[i].tickTime * unifiedEvents[i].Tempo) / (division * 1000000.0);
        unifiedEvents[i].sumSecondTime = secondSum;
    }

    for (int i=0;i<unifiedEvents.size();i++)
    {
        if (unifiedEvents[i].type==midiFile::midiEventName::noteOn)
        {
            noteStateArray[unifiedEvents[i].noteChannel][unifiedEvents[i].noteIndex].push_back({});
            noteStateArray[unifiedEvents[i].noteChannel][unifiedEvents[i].noteIndex][noteStateArray[unifiedEvents[i].noteChannel][unifiedEvents[i].noteIndex].size()-1].startTime = unifiedEvents[i].sumSecondTime;
            noteStateArray[unifiedEvents[i].noteChannel][unifiedEvents[i].noteIndex][noteStateArray[unifiedEvents[i].noteChannel][unifiedEvents[i].noteIndex].size()-1].startTick = unifiedEvents[i].sumTickTime;
        } else if (unifiedEvents[i].type==midiFile::midiEventName::noteOff) {
            if (noteStateArray[unifiedEvents[i].noteChannel][unifiedEvents[i].noteIndex].size()>0)
            {
                double noteStartTime = noteStateArray[unifiedEvents[i].noteChannel][unifiedEvents[i].noteIndex][0].startTime;
                uint32_t noteStartTick = noteStateArray[unifiedEvents[i].noteChannel][unifiedEvents[i].noteIndex][0].startTick;
                unifiedNotes.push_back({unifiedEvents[i].noteIndex, unifiedEvents[i].noteVelocity, noteStartTime, unifiedEvents[i].sumSecondTime - noteStartTime, unifiedEvents[i].noteChannel, unifiedEvents[i].sumTickTime, unifiedEvents[i].sumTickTime-noteStartTick});
                noteStateArray[unifiedEvents[i].noteChannel][unifiedEvents[i].noteIndex].erase(noteStateArray[unifiedEvents[i].noteChannel][unifiedEvents[i].noteIndex].begin());
                
            } else {
                std::cout<<"Attempted to turn off an already off note! "<<i<<"\n";
            }
        }
    }


    std::sort(unifiedNotes.begin(), unifiedNotes.end(), [](midiNote a, midiNote b)
    {
        return a.startTime<b.startTime;
    });

    std::cout<<"Midi parsed\n";
}

void midiFile::updateCurrentTime()
{
    uint32_t currentTick = fluid_player_get_current_tick(player);
    uint32_t tickDelta = currentTick - lastTick;
    lastTick = currentTick;
    currentTime+=(tickDelta * fluid_player_get_midi_tempo(player)) / (division * 1000000.0);
}

void midiFile::resetCurrentTimeCounter()
{
    currentTime = 0;
    lastTick = 0;
}

uint32_t midiFile::readVariableAmount(std::ifstream &inputMidi)
{
    uint32_t returnVal = 0;
    uint8_t currentByte = 0;

    do {
        inputMidi.read((char*)&currentByte, 1);
        returnVal = (returnVal<<7) | (currentByte&0x7F);
    } while (currentByte&0x80);
    return returnVal;

}

std::string midiFile::readString(std::ifstream &inputMidi, uint32_t length)
{
    std::string result;
    for (int i=0;i<length;i++)
    {
        if (inputMidi.eof())
        {
            return result;
        }
        result+=inputMidi.get();
    }
    return result;
}

void quiet_log_handler(int level, const char* message, void* data) {}
void midiFile::fluidsynthInit(std::string midiPath)
{
    fluid_set_log_function(FLUID_PANIC, quiet_log_handler, nullptr);
    fluid_set_log_function(FLUID_ERR, quiet_log_handler, nullptr);
    fluid_set_log_function(FLUID_WARN, quiet_log_handler, nullptr);
    fluid_set_log_function(FLUID_INFO, quiet_log_handler, nullptr);
    fluid_set_log_function(FLUID_DBG, quiet_log_handler, nullptr);
    settings = new_fluid_settings();
    synth = new_fluid_synth(settings);

    std::string soundFont = SF2_FILE_PATH;
    if (fluid_synth_sfload(synth, soundFont.c_str(), 1) == FLUID_FAILED) {
        std::cerr << "Failed to load SoundFont: " << soundFont << "\n";
        throw std::invalid_argument("sf2 file not found");
    }
    fluid_settings_setstr(settings, "audio.driver", "pulseaudio");
    driver = new_fluid_audio_driver(settings, synth);
    player = new_fluid_player(synth);
    if (fluid_player_add(player, midiPath.c_str()) != FLUID_OK) {
        std::cerr << "Failed to load MIDI file\n";
        throw std::invalid_argument("midi file not found");
    }
    fluid_settings_setnum(settings, "synth.gain", 2);
}

midiEvent::midiEvent(midiFile::midiEventName _type, uint8_t _noteIndex, uint8_t _noteVelocity, uint32_t _tickTime, uint8_t _noteChannel, uint32_t _sumTickTime, uint32_t _Tempo)
    : type(_type), noteIndex(_noteIndex), noteVelocity(_noteVelocity), tickTime(_tickTime), noteChannel(_noteChannel), sumTickTime(_sumTickTime), Tempo(_Tempo), sumSecondTime(0), metaType(midiFile::MetaEventName::Sequence)
{

}

midiTrack::midiTrack()
    : trackName("MIDI Track")
{
}

midiNote::midiNote(uint8_t _note, uint8_t _velocity, double _startTime, double _duration, uint8_t _channel, uint32_t _startTick, uint32_t _durationTick)
    : note(_note), velocity(_velocity), startTime(_startTime), duration(_duration), channel(_channel), startTick(_startTick), durationTick(_durationTick)
{

}

playingNote::playingNote()
    : startTime(0)
{
}


