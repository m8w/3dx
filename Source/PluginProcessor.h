#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Params.h"
#include "DSP/Quaternion.h"
#include "DSP/Voice.h"
#include "DSP/Sequencer.h"
#include "DSP/HilbertShifter.h"
#include "DSP/Haunt.h"
#include <array>
#include <vector>
#include <random>
#include <atomic>

// QUATERNION -- a 4-voice instrument descended from two Web Audio prototypes:
// CAYLEY II's octonion harmonic sequencer (reduced from 8 voices to 4, so
// each voice is exactly one quaternion component -- Hamilton product mixing
// literally adds and subtracts the four voices' signals every sample) and
// SHIFT's Bode frequency shifter (ported bit-for-bit as the output stage).
// Each voice continuously morphs sine -> triangle -> saw -> vocoder/formant,
// and a circular FM matrix cross-modulates the four voices' phases -- the
// two "adding/subtracting" and "frequency modulation" mechanisms requested
// on top of the original design. See README.md for the full control map.
class QuaternionAudioProcessor : public juce::AudioProcessor
{
public:
    QuaternionAudioProcessor();
    ~QuaternionAudioProcessor() override = default;

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
    double getTailLengthSeconds() const override { return 12.0; }

    int getNumPrograms() override { return 2; }
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // UI-only setting (not a host parameter): index into Theme.h's themes(),
    // persisted alongside apvts state so the chosen look survives reload.
    int getThemeIndex() const { return themeIndex.load(); }
    void setThemeIndex(int i) { themeIndex.store(i); }

    // Read by the editor for the goniometer / spectrum displays.
    static constexpr int kScopeSize = 1024;
    struct ScopeData
    {
        std::array<float, kScopeSize> compA {}; // quaternion component 0 (goniometer X)
        std::array<float, kScopeSize> compB {}; // quaternion component 1 (goniometer Y)
        std::array<float, kScopeSize> mono {};  // post-shift mono sum, for spectrum
        std::atomic<int> writeIndex { 0 };
    };
    ScopeData scope;

private:
    void cacheParamPointers();
    void handleMidiMessage(const juce::MidiMessage& msg);
    void renderSample(float* outL, float* outR, float scL, float scR);
    static double midiToHz(int note) { return 440.0 * std::pow(2.0, (note - 69) / 12.0); }
    static double softLimit(double x) { return std::tanh(x * 0.9) * 1.05; }

    double sampleRate = 44100.0;
    int currentProgram = 0; // 0 = Default, 1 = Ambient Serenity (see PluginProcessor.cpp)
    std::atomic<int> themeIndex { 0 };

    std::atomic<float> *pWave, *pFan, *pFormant, *pScMix, *pRate, *pRange, *pGlide, *pGate, *pEnv,
        *pPattern, *pSpin, *pDrift, *pDepth, *pAlgMode, *pFmAmount, *pShiftHz, *pBalance, *pFeedback,
        *pFbTime, *pShiftMix, *pHaunt, *pOutput;

    std::array<Voice, 4> voices;
    Sequencer seq;
    AlgebraHaunt haunt;
    std::array<GhostTape, 2> ghostTape;
    std::array<HilbertShifter, 2> shifter;

    std::array<Biquad, 5> scBandFilters;
    std::array<double, 5> scEnv { 0, 0, 0, 0, 0 };

    std::mt19937 rng { std::random_device{}() };

    std::vector<int> heldNotes;
    int currentMidiNote = -1;
    bool noteActive = false;
    double rootFreq = 220.0;
    double noteEnv = 0.0;
    double attackCoeff = 0.1, releaseCoeff = 0.99;

    double yPhi = 0.0, wPhi = 0.0;
    std::array<double, 3> axis { 0.6, -0.5, 0.4 };
    std::array<double, 3> waxis { -0.3, 0.7, 0.5 };

    long stepCounter = 0;
    long stepLenSamples = 4096;

    static void normalize3(std::array<double, 3>& v)
    {
        double n = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
        if (n > 1.0e-9) { v[0] /= n; v[1] /= n; v[2] /= n; }
        else { v = { 1, 0, 0 }; }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(QuaternionAudioProcessor)
};
