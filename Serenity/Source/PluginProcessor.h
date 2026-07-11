#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Params.h"
#include "DSP/PadVoice.h"
#include "DSP/Chime.h"
#include "DSP/SpaceDelay.h"
#include "DSP/OnePole.h"
#include <array>
#include <atomic>
#include <random>

// SERENITY -- a free-running ambient instrument, the calmer counterpart to
// Quaternion. Unlike Quaternion it does not need a held MIDI note: five
// pad voices on a slowly root-drifting pentatonic/modal chord swell and
// fade on independent random clocks (Eno "Music for Airports" style),
// while a random scheduler fires short FM bell "chimes" at random scale
// degrees and pan. An incoming MIDI note re-centers the root but the
// plugin is audible the moment it's loaded. See README.md for the full
// control map.
class SerenityAudioProcessor : public juce::AudioProcessor
{
public:
    SerenityAudioProcessor();
    ~SerenityAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    using AudioProcessor::processBlock;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // Read by the editor for the breathing-orb visual.
    std::atomic<float> outputLevel { 0.0f };

private:
    void cacheParamPointers();
    void handleMidiMessage(const juce::MidiMessage& msg);
    void renderSample(float* outL, float* outR);
    static double midiToHz(int note) { return 440.0 * std::pow(2.0, (note - 69) / 12.0); }

    double sampleRate = 44100.0;

    std::atomic<float> *pDensity, *pSpace, *pBrightness, *pDrift, *pScale, *pOutput;

    std::array<PadVoice, 5> pads;
    ChimeBank chimes;
    SpaceDelay space;
    OnePoleLP toneL, toneR;

    std::mt19937 rng { std::random_device{}() };

    double rootFreq = 110.0; // A2
    double driftSemis = 0.0;
    long driftCounterSamples = 1;

    float levelAccum = 0.0f;
    int levelCount = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SerenityAudioProcessor)
};
