//credits:
    //was heavily relied on
        //https://www.youtube.com/watch?v=040BKtnDdg0&t=2075s - javidx9's video on MIDI
    //A healthy dose of
        //https://ccrma.stanford.edu/~craig/14q/midifile/MidiFileFormat.html
    //andd some suggestion from ChatGPT
#include "midiParser.h"
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
    : inputMidi(filePath, std::ios::binary), timerStart(false)
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
                midiTracks[trackI].midiEvents.push_back({midiEventName::noteOff, noteIndex, noteVelocity, timeDelta, channel, tickSum, Tempo, trackI});
                
                if (noteIndex>127) {std::cout<<"high "<<eventCount<<" "<<(int)noteIndex<<"track: "<<trackI<<"\n";}
            } else if (eventType == midiEventName::noteOn) {
                uint8_t channel = status&0x0F;
                uint8_t noteIndex = inputMidi.get();
                uint8_t noteVelocity = inputMidi.get();
                if (noteVelocity > 0)
                {
                    midiTracks[trackI].midiEvents.push_back({midiEventName::noteOn, noteIndex, noteVelocity, timeDelta, channel, tickSum, Tempo, trackI});
                } else {
                    midiTracks[trackI].midiEvents.push_back({midiEventName::noteOff, noteIndex, noteVelocity, timeDelta, channel, tickSum, Tempo, trackI});
                }

                if (noteIndex>127) {std::cout<<"high "<<eventCount<<(int)noteIndex<<"track: "<<trackI<<"\n";}
            } else if (eventType == midiEventName::afterTouch) {
                uint8_t channel = status&0x0F;
                uint8_t noteIndex = inputMidi.get();
                uint8_t noteVelocity = inputMidi.get();
                midiTracks[trackI].midiEvents.emplace_back(midiEventName::afterTouch, noteIndex, noteVelocity, timeDelta, channel, tickSum, Tempo, trackI);
            } else if (eventType == midiEventName::controlChange) {
                uint8_t channel = status&0x0F;
                uint8_t noteIndex = inputMidi.get();
                uint8_t noteVelocity = inputMidi.get();
                midiTracks[trackI].midiEvents.emplace_back(midiEventName::controlChange, noteIndex, noteVelocity, timeDelta, channel, tickSum, Tempo, trackI);
            } else if (eventType == midiEventName::programChange) {
                uint8_t channel = status&0x0F;
                uint8_t programID = inputMidi.get();
                midiTracks[trackI].midiEvents.emplace_back(midiEventName::programChange, programID, 0, timeDelta, channel, tickSum, Tempo, trackI);
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
                    std::cout<<"Track: "<<trackI<<" DataFrame: "<<eventCount<<"\n";
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
                            midiTracks[trackI].midiEvents.emplace_back(midiEventName::systemExclusive, 0, 0, timeDelta, 0, tickSum, Tempo, trackI);
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

    int concurrentNotes = 0;
    int concurrentNoteMax = 0;
    std::array<uint32_t, 16>currentChannelProgram;
    for (int i=0;i<unifiedEvents.size();i++)
    {
        if (unifiedEvents[i].type==midiFile::midiEventName::noteOn)
        {
            concurrentNotes++;
            if (concurrentNotes>concurrentNoteMax) { concurrentNoteMax = concurrentNotes;}
            noteStateArray[unifiedEvents[i].noteChannel][unifiedEvents[i].noteIndex].push_back({});
            noteStateArray[unifiedEvents[i].noteChannel][unifiedEvents[i].noteIndex].back().startTime = unifiedEvents[i].sumSecondTime;
            noteStateArray[unifiedEvents[i].noteChannel][unifiedEvents[i].noteIndex].back().startTick = unifiedEvents[i].sumTickTime;
        } else if (unifiedEvents[i].type==midiFile::midiEventName::noteOff) {
            if (noteStateArray[unifiedEvents[i].noteChannel][unifiedEvents[i].noteIndex].size()>0)
            {
                concurrentNotes--;
                double noteStartTime = noteStateArray[unifiedEvents[i].noteChannel][unifiedEvents[i].noteIndex][0].startTime;
                uint32_t noteStartTick = noteStateArray[unifiedEvents[i].noteChannel][unifiedEvents[i].noteIndex][0].startTick;
                unifiedNotes.push_back({unifiedEvents[i].noteIndex, unifiedEvents[i].noteVelocity, noteStartTime, unifiedEvents[i].sumSecondTime - noteStartTime, unifiedEvents[i].noteChannel, unifiedEvents[i].sumTickTime, unifiedEvents[i].sumTickTime-noteStartTick, unifiedEvents[i].track, currentChannelProgram[unifiedEvents[i].noteChannel]});
                noteStateArray[unifiedEvents[i].noteChannel][unifiedEvents[i].noteIndex].erase(noteStateArray[unifiedEvents[i].noteChannel][unifiedEvents[i].noteIndex].begin());
            } else {
                std::cout<<"Attempted to turn off an already off note! "<<i<<"\n";
            }
        } else if (unifiedEvents[i].type == midiFile::midiEventName::programChange) {
            currentChannelProgram[unifiedEvents[i].noteChannel] = unifiedEvents[i].noteIndex;
        }
    }

    std::cout<<"Concurrent notes: "<<concurrentNoteMax<<"\n";


    std::sort(unifiedNotes.begin(), unifiedNotes.end(), [](midiNote a, midiNote b)
    {
        return a.startTime<b.startTime;
    });

    std::cout<<"Midi parsed\n";
}

void midiFile::updateCurrentTime()
{
    if (fluid_player_get_status(player)==FLUID_PLAYER_READY && currentTime>=0 && timerStart)
    {
        fluid_player_play(player);
    } else if (currentTime<0 && timerStart)
    {
        std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
        double timeDelta = (double)std::chrono::duration_cast<std::chrono::microseconds>(now - lastTime).count()/1000000.0;
        lastTime = now;
        currentTime+=timeDelta;
        return;
    }
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

void midiFile::startPlayback()
{
    lastTime = std::chrono::high_resolution_clock::now();
    timerStart=true;       
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
    //fluid_set_log_function(FLUID_ERR, quiet_log_handler, nullptr);
    //fluid_set_log_function(FLUID_WARN, quiet_log_handler, nullptr);
    //fluid_set_log_function(FLUID_INFO, quiet_log_handler, nullptr);
    //fluid_set_log_function(FLUID_DBG, quiet_log_handler, nullptr);
    settings = new_fluid_settings();
    fluid_settings_setstr(settings, "audio.driver", "pulseaudio");
    fluid_settings_setnum(settings, "synth.sample-rate", 44100.0); 
    fluid_settings_setnum(settings, "synth.gain", 2);

    synth = new_fluid_synth(settings);
    
    std::string soundFont = SF2_FILE_PATH;
    if (fluid_synth_sfload(synth, soundFont.c_str(), 1) == FLUID_FAILED) {
        std::cerr << "Failed to load SoundFont: " << soundFont << "\n";
        throw std::invalid_argument("sf2 file not found");
    }
    driver = new_fluid_audio_driver(settings, synth);
    player = new_fluid_player(synth);
    if (fluid_player_add(player, midiPath.c_str()) != FLUID_OK) {
        std::cerr << "Failed to load MIDI file\n";
        throw std::invalid_argument("midi file not found");
    }
    
}

midiEvent::midiEvent(midiFile::midiEventName _type, uint8_t _noteIndex, uint8_t _noteVelocity, uint32_t _tickTime, uint8_t _noteChannel, uint32_t _sumTickTime, uint32_t _Tempo, uint32_t _track)
    : type(_type), noteIndex(_noteIndex), noteVelocity(_noteVelocity), tickTime(_tickTime), noteChannel(_noteChannel), sumTickTime(_sumTickTime), Tempo(_Tempo), sumSecondTime(0), metaType(midiFile::MetaEventName::Sequence), track(_track)
{

}

midiTrack::midiTrack()
    : trackName("MIDI Track")
{
}

midiNote::midiNote(uint8_t _note, uint8_t _velocity, double _startTime, double _duration, uint8_t _channel, uint32_t _startTick, uint32_t _durationTick,  uint32_t _track, uint32_t _program)
    : note(_note), velocity(_velocity), startTime(_startTime), duration(_duration), channel(_channel), startTick(_startTick), durationTick(_durationTick), track(_track), program(_program)
{

}

playingNote::playingNote()
    : startTime(0)
{
}

//chatgpt clutched with this one lol, would've hurt to type this out manually

std::string midiFile::getInstrumentName(int programID, int channel, int note) {
    if (channel == 9) { // MIDI channels are 0-indexed, so 9 = channel 10
        switch (note) {
            case 35: return "Acoustic Bass Drum";
            case 36: return "Electric Bass Drum";
            case 37: return "Side Stick";
            case 38: return "Acoustic Snare";
            case 39: return "Hand Clap";
            case 40: return "Electric Snare";
            case 41: return "Low Floor Tom";
            case 42: return "Closed Hi-hat";
            case 43: return "High Floor Tom";
            case 44: return "Pedal Hi-hat";
            case 45: return "Low Tom";
            case 46: return "Open Hi-hat";
            case 47: return "Low-Mid Tom";
            case 48: return "High-Mid Tom";
            case 49: return "Crash Cymbal 1";
            case 50: return "High Tom";
            case 51: return "Ride Cymbal 1";
            case 52: return "Chinese Cymbal";
            case 53: return "Ride Bell";
            case 54: return "Tambourine";
            case 55: return "Splash Cymbal";
            case 56: return "Cowbell";
            case 57: return "Crash Cymbal 2";
            case 58: return "Vibraslap";
            case 59: return "Ride Cymbal 2";
            case 60: return "High Bongo";
            case 61: return "Low Bongo";
            case 62: return "Mute High Conga";
            case 63: return "Open High Conga";
            case 64: return "Low Conga";
            case 65: return "High Timbale";
            case 66: return "Low Timbale";
            case 67: return "High Agogô";
            case 68: return "Low Agogô";
            case 69: return "Cabasa";
            case 70: return "Maracas";
            case 71: return "Short Whistle";
            case 72: return "Long Whistle";
            case 73: return "Short Güiro";
            case 74: return "Long Güiro";
            case 75: return "Claves";
            case 76: return "High Woodblock";
            case 77: return "Low Woodblock";
            case 78: return "Mute Cuíca";
            case 79: return "Open Cuíca";
            case 80: return "Mute Triangle";
            case 81: return "Open Triangle";
            default: return "Unknown Percussion";
        }
    }

    // Otherwise, use melodic instrument program ID lookup
    return getInstrumentName(programID);
}

std::string midiFile::getInstrumentName(int programID) {
    programID++;
    switch (programID) {
        case 1: return "Acoustic Grand Piano";
        case 2: return "Bright Acoustic Piano";
        case 3: return "Electric Grand Piano";
        case 4: return "Honky-tonk Piano";
        case 5: return "Electric Piano 1";
        case 6: return "Electric Piano 2";
        case 7: return "Harpsichord";
        case 8: return "Clavinet";

        case 9: return "Celesta";
        case 10: return "Glockenspiel";
        case 11: return "Music Box";
        case 12: return "Vibraphone";
        case 13: return "Marimba";
        case 14: return "Xylophone";
        case 15: return "Tubular Bells";
        case 16: return "Dulcimer";

        case 17: return "Drawbar Organ";
        case 18: return "Percussive Organ";
        case 19: return "Rock Organ";
        case 20: return "Church Organ";
        case 21: return "Reed Organ";
        case 22: return "Accordion";
        case 23: return "Harmonica";
        case 24: return "Bandoneon";

        case 25: return "Acoustic Guitar (nylon)";
        case 26: return "Acoustic Guitar (steel)";
        case 27: return "Electric Guitar (jazz)";
        case 28: return "Electric Guitar (clean)";
        case 29: return "Electric Guitar (muted)";
        case 30: return "Electric Guitar (overdrive)";
        case 31: return "Electric Guitar (distortion)";
        case 32: return "Electric Guitar (harmonics)";

        case 33: return "Acoustic Bass";
        case 34: return "Electric Bass (finger)";
        case 35: return "Electric Bass (picked)";
        case 36: return "Fretless Bass";
        case 37: return "Slap Bass 1";
        case 38: return "Slap Bass 2";
        case 39: return "Synth Bass 1";
        case 40: return "Synth Bass 2";

        case 41: return "Violin";
        case 42: return "Viola";
        case 43: return "Cello";
        case 44: return "Contrabass";
        case 45: return "Tremolo Strings";
        case 46: return "Pizzicato Strings";
        case 47: return "Orchestral Harp";
        case 48: return "Timpani";

        case 49: return "String Ensemble 1";
        case 50: return "String Ensemble 2";
        case 51: return "Synth Strings 1";
        case 52: return "Synth Strings 2";
        case 53: return "Choir Aahs";
        case 54: return "Voice Oohs";
        case 55: return "Synth Voice";
        case 56: return "Orchestra Hit";

        case 57: return "Trumpet";
        case 58: return "Trombone";
        case 59: return "Tuba";
        case 60: return "Muted Trumpet";
        case 61: return "French Horn";
        case 62: return "Brass Section";
        case 63: return "Synth Brass 1";
        case 64: return "Synth Brass 2";

        case 65: return "Soprano Sax";
        case 66: return "Alto Sax";
        case 67: return "Tenor Sax";
        case 68: return "Baritone Sax";
        case 69: return "Oboe";
        case 70: return "English Horn";
        case 71: return "Bassoon";
        case 72: return "Clarinet";

        case 73: return "Piccolo";
        case 74: return "Flute";
        case 75: return "Recorder";
        case 76: return "Pan Flute";
        case 77: return "Blown Bottle";
        case 78: return "Shakuhachi";
        case 79: return "Whistle";
        case 80: return "Ocarina";

        case 81: return "Lead 1 (square)";
        case 82: return "Lead 2 (sawtooth)";
        case 83: return "Lead 3 (calliope)";
        case 84: return "Lead 4 (chiff)";
        case 85: return "Lead 5 (charang)";
        case 86: return "Lead 6 (voice)";
        case 87: return "Lead 7 (fifths)";
        case 88: return "Lead 8 (bass + lead)";

        case 89: return "Pad 1 (new age)";
        case 90: return "Pad 2 (warm)";
        case 91: return "Pad 3 (polysynth)";
        case 92: return "Pad 4 (choir)";
        case 93: return "Pad 5 (bowed)";
        case 94: return "Pad 6 (metallic)";
        case 95: return "Pad 7 (halo)";
        case 96: return "Pad 8 (sweep)";

        case 97: return "FX 1 (rain)";
        case 98: return "FX 2 (soundtrack)";
        case 99: return "FX 3 (crystal)";
        case 100: return "FX 4 (atmosphere)";
        case 101: return "FX 5 (brightness)";
        case 102: return "FX 6 (goblins)";
        case 103: return "FX 7 (echoes)";
        case 104: return "FX 8 (sci-fi)";

        case 105: return "Sitar";
        case 106: return "Banjo";
        case 107: return "Shamisen";
        case 108: return "Koto";
        case 109: return "Kalimba";
        case 110: return "Bag Pipe";
        case 111: return "Fiddle";
        case 112: return "Shanai";

        case 113: return "Tinkle Bell";
        case 114: return "Agogo";
        case 115: return "Steel Drums";
        case 116: return "Woodblock";
        case 117: return "Taiko Drum";
        case 118: return "Melodic Tom";
        case 119: return "Synth Drum";
        case 120: return "Reverse Cymbal";

        case 121: return "Guitar Fret Noise";
        case 122: return "Breath Noise";
        case 123: return "Seashore";
        case 124: return "Bird Tweet";
        case 125: return "Telephone Ring";
        case 126: return "Helicopter";
        case 127: return "Applause";
        case 128: return "Gunshot";

        default: return "Unknown Instrument";
    }
}

std::string midiFile::getInstrumentName(int programID, int channel)
{
    if (channel == 9)
    {
        return "Percussion Instrument";
    }
    return getInstrumentName(programID);
}
