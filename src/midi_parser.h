#pragma once
#include <string>
#include <fstream>
#include <vector>

class midiEvent;
class midiTrack;
class midiNote;
class playingNote;

class midiFile
{
public:
    enum midiEventName : uint8_t
    {
        noteOff = 0x80,
        noteOn = 0x90,
        afterTouch = 0xA0,
        controlChange = 0xB0,
        programChange = 0xC0,
        channelPressure = 0xD0,
        pitchBend = 0xE0,
        systemExclusive = 0xF0
    };

    enum MetaEventName : uint8_t
	{
		Sequence = 0x00,
		Text = 0x01,
		Copyright = 0x02,
		TrackName = 0x03,
		InstrumentName = 0x04,
		Lyrics = 0x05,
		Marker = 0x06,
		CuePoint = 0x07,
		ChannelPrefix = 0x20,
		EndOfTrack = 0x2F,
		SetTempo = 0x51,
		SMPTEOffset = 0x54,
		TimeSignature = 0x58,
		KeySignature = 0x59,
		SequencerSpecific = 0x7F,
	};
    midiFile(std::string filePath);

    uint32_t readVariableAmount(std::ifstream& inputMidi);
    std::string readString(std::ifstream &inputMidi, uint32_t length);
    std::vector<midiTrack> midiTracks;
    std::vector<midiEvent> unifiedEvents;
    std::vector<midiNote> unifiedNotes;
    std::ifstream inputMidi;

    uint32_t timeSignatureNumerator;
    uint32_t timeSignatureDenominator;
    uint32_t clocksPerTick;
    uint32_t _32per24Clocks;
private:
    uint32_t Tempo = 0;
	uint32_t BPM = 0;
    
};

class midiNote
{
    public:
        midiNote(uint8_t note, uint8_t velocity, double startTime, double duration, uint8_t channel);
        uint8_t note;
	    uint8_t velocity;
        uint8_t channel;
	    double startTime;
	    double duration;
    private:
};

class playingNote
{
    public:
        playingNote();
        bool playing;
        double startTime;
    private:
};

class midiTrack
{
    public:
        midiTrack();
        std::string trackName;
        std::vector<midiEvent> midiEvents;
    private:
};

class midiEvent
{
    public:
        midiEvent(midiFile::midiEventName, uint8_t noteIndex, uint8_t noteVelocity, uint32_t tickTime, uint8_t noteChannel, uint32_t sumTickTime, uint32_t Tempo);

        midiFile::midiEventName type;
        uint8_t noteIndex;
        uint8_t noteVelocity;
        uint8_t noteChannel;
        uint32_t tickTime;
        uint32_t sumTickTime;
        double sumSecondTime;
        uint32_t Tempo;

    private:

};

