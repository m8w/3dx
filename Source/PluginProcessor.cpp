#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>

namespace
{
    // Two vowel-ish formant tables the FORMANT knob morphs between.
    constexpr std::array<double, 5> kVowelA { 700.0, 1220.0, 2600.0, 3300.0, 4950.0 };  // "ah"
    constexpr std::array<double, 5> kVowelB { 270.0, 2300.0, 3000.0, 3700.0, 4950.0 };  // "ee"
    constexpr std::array<double, 5> kFormantGains { 1.0, 0.7, 0.5, 0.35, 0.25 };
    constexpr std::array<double, 4> kPanL { 0.90, 0.65, 0.35, 0.10 };
    constexpr std::array<double, 4> kPanR { 0.10, 0.35, 0.65, 0.90 };
}

QuaternionAudioProcessor::QuaternionAudioProcessor()
    : AudioProcessor(BusesProperties()
                          .withInput("Sidechain", juce::AudioChannelSet::stereo(), false)
                          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMS", Params::createLayout())
{
    cacheParamPointers();
}

void QuaternionAudioProcessor::cacheParamPointers()
{
    pWave = apvts.getRawParameterValue(Params::wave);
    pFan = apvts.getRawParameterValue(Params::fan);
    pFormant = apvts.getRawParameterValue(Params::formant);
    pScMix = apvts.getRawParameterValue(Params::scMix);
    pRate = apvts.getRawParameterValue(Params::rate);
    pRange = apvts.getRawParameterValue(Params::range);
    pGlide = apvts.getRawParameterValue(Params::glide);
    pGate = apvts.getRawParameterValue(Params::gate);
    pEnv = apvts.getRawParameterValue(Params::env);
    pPattern = apvts.getRawParameterValue(Params::pattern);
    pSpin = apvts.getRawParameterValue(Params::spin);
    pDrift = apvts.getRawParameterValue(Params::drift);
    pDepth = apvts.getRawParameterValue(Params::depth);
    pAlgMode = apvts.getRawParameterValue(Params::algMode);
    pFmAmount = apvts.getRawParameterValue(Params::fmAmount);
    pShiftHz = apvts.getRawParameterValue(Params::shiftHz);
    pBalance = apvts.getRawParameterValue(Params::balance);
    pFeedback = apvts.getRawParameterValue(Params::feedback);
    pFbTime = apvts.getRawParameterValue(Params::fbTime);
    pShiftMix = apvts.getRawParameterValue(Params::shiftMix);
    pHaunt = apvts.getRawParameterValue(Params::haunt);
    pOutput = apvts.getRawParameterValue(Params::output);
}

void QuaternionAudioProcessor::prepareToPlay(double sr, int)
{
    sampleRate = sr;
    for (auto& v : voices) v.reset();
    for (auto& s : shifter) s.prepare(sr);
    for (auto& g : ghostTape) g.prepare(sr);
    haunt.prepare(sr);
    for (auto& f : scBandFilters) f.reset();
    scEnv.fill(0.0);

    stepCounter = 0;
    stepLenSamples = std::max<long>(64, static_cast<long>(sr / std::max(0.02, static_cast<double>(*pRate))));
    noteEnv = 0.0;
    attackCoeff = 1.0 - std::exp(-1.0 / (0.005 * sr));
    releaseCoeff = std::exp(-1.0 / (0.15 * sr));
    yPhi = wPhi = 0.0;
}

bool QuaternionAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    auto sc = layouts.getChannelSet(true, 0);
    if (!sc.isDisabled() && sc != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void QuaternionAudioProcessor::handleMidiMessage(const juce::MidiMessage& msg)
{
    if (msg.isNoteOn())
    {
        int note = msg.getNoteNumber();
        heldNotes.erase(std::remove(heldNotes.begin(), heldNotes.end(), note), heldNotes.end());
        heldNotes.push_back(note);
        currentMidiNote = note;
        rootFreq = midiToHz(note);
        noteActive = true;
    }
    else if (msg.isNoteOff())
    {
        int note = msg.getNoteNumber();
        heldNotes.erase(std::remove(heldNotes.begin(), heldNotes.end(), note), heldNotes.end());
        if (!heldNotes.empty())
        {
            currentMidiNote = heldNotes.back();
            rootFreq = midiToHz(currentMidiNote);
            noteActive = true;
        }
        else
        {
            noteActive = false;
        }
    }
}

void QuaternionAudioProcessor::renderSample(float* outL, float* outR, float scLf, float scRf)
{
    double waveP = *pWave, fanP = *pFan, formantP = *pFormant, scMixP = *pScMix;
    double rateP = *pRate, rangeP = *pRange, glideP = *pGlide, gateP = *pGate, envAmtP = *pEnv;
    auto pattern = static_cast<Sequencer::Pattern>(static_cast<int>(*pPattern));
    double spinP = *pSpin, driftP = *pDrift, depthP = *pDepth;
    auto algMode = static_cast<AlgMode>(static_cast<int>(*pAlgMode));
    double fmAmtP = *pFmAmount;
    double shiftHzP = *pShiftHz, balP = *pBalance, fbP = *pFeedback, fbTimeP = *pFbTime, shiftMixP = *pShiftMix;
    double hauntP = *pHaunt, outP = *pOutput;

    // -- sequencer clock --
    if (++stepCounter >= stepLenSamples)
    {
        stepCounter = 0;
        bool triggered[4] { false, false, false, false };
        seq.step(pattern, static_cast<float>(rangeP), static_cast<float>(gateP), static_cast<float>(envAmtP), rng, triggered);
        for (int k = 0; k < 4; ++k)
            if (triggered[k]) voices[static_cast<size_t>(k)].envState = 1;
        stepLenSamples = std::max<long>(64, static_cast<long>(sampleRate / std::max(0.02, rateP)));
    }

    bool droneMode = (pattern == Sequencer::Drone) || (envAmtP <= 0.01);
    double glideCoeff = std::pow(10.0, -1.0 - 3.5 * glideP);
    double decayC = 1.0 - envAmtP * 0.002;

    // -- formant frequencies (vowel morph) + sidechain band envelopes --
    std::array<double, 5> formantFreqs {};
    for (int b = 0; b < 5; ++b)
        formantFreqs[static_cast<size_t>(b)] = kVowelA[static_cast<size_t>(b)] * (1.0 - formantP) + kVowelB[static_cast<size_t>(b)] * formantP;

    double scMono = 0.5 * (static_cast<double>(scLf) + static_cast<double>(scRf));
    std::array<double, 5> scGains {};
    for (int b = 0; b < 5; ++b)
    {
        auto& f = scBandFilters[static_cast<size_t>(b)];
        f.setBandpass(formantFreqs[static_cast<size_t>(b)], 6.0, sampleRate);
        double band = f.process(scMono);
        double rect = std::abs(band);
        double& env = scEnv[static_cast<size_t>(b)];
        env += (rect - env) * (rect > env ? 0.05 : 0.002);
        scGains[static_cast<size_t>(b)] = std::clamp(env * 4.0, 0.0, 1.0);
    }

    // -- 4 voices --
    double X[4];
    double fLimit = sampleRate * 0.45;
    for (int k = 0; k < 4; ++k)
    {
        double h = static_cast<double>(std::max(1, seq.targetH[static_cast<size_t>(k)]));
        double targetFreq = rootFreq * h;
        while (targetFreq > fLimit) targetFreq *= 0.5;

        double wavePos = waveP + fanP * static_cast<double>(k) / 3.0;
        double fmMod = fmAmtP * fmAmtP * 0.02 * voices[static_cast<size_t>((k + 3) % 4)].lastOutput;
        double level = 0.85 * std::pow(h, -0.55);

        X[k] = voices[static_cast<size_t>(k)].render(sampleRate, targetFreq, glideCoeff, level, wavePos, fmMod,
                                                       droneMode, 1.0, decayC, formantFreqs, kFormantGains, scGains, scMixP);
    }

    // -- rotor + quaternion algebra --
    bool frozen = haunt.stasisActive();
    if (!frozen) yPhi += 2.0 * juce::MathConstants<double>::pi * spinP / sampleRate;
    wPhi += 2.0 * juce::MathConstants<double>::pi * (spinP * 0.618 + 0.11) / sampleRate;

    if (driftP > 0.0)
    {
        std::uniform_real_distribution<double> uni(0.0, 1.0);
        for (int i = 0; i < 3; ++i)
        {
            axis[static_cast<size_t>(i)] += driftP * 0.06 * (uni(rng) - 0.5);
            waxis[static_cast<size_t>(i)] += driftP * 0.06 * (uni(rng) - 0.5);
        }
        normalize3(axis);
        normalize3(waxis);
    }

    Quat Y { std::cos(yPhi), std::sin(yPhi) * axis[0], std::sin(yPhi) * axis[1], std::sin(yPhi) * axis[2] };
    Quat W { std::cos(wPhi), std::sin(wPhi) * waxis[0], std::sin(wPhi) * waxis[1], std::sin(wPhi) * waxis[2] };

    double Z[4];
    applyAlgebra(algMode, X, Y, W, Z);
    haunt.applyAndRecord(Z);

    // -- depth mix + equal-power pan across 4 voices --
    double L = 0.0, R = 0.0;
    for (int k = 0; k < 4; ++k)
    {
        double v = X[static_cast<size_t>(k)] * (1.0 - depthP) + Z[static_cast<size_t>(k)] * depthP;
        L += v * kPanL[static_cast<size_t>(k)];
        R += v * kPanR[static_cast<size_t>(k)];
    }
    L *= 0.35; R *= 0.35;

    // -- overall note envelope (silence when no MIDI note is held) --
    if (noteActive) noteEnv += (1.0 - noteEnv) * attackCoeff;
    else noteEnv *= releaseCoeff;
    L *= noteEnv; R *= noteEnv;

    // -- Bode frequency shifter, independent per channel --
    double shiftedL = shifter[0].process(L, shiftHzP, balP, fbP, fbTimeP, shiftMixP);
    double shiftedR = shifter[1].process(R, shiftHzP, balP, fbP, fbTimeP, shiftMixP);

    // -- haunt: ghost tape resurfaces past output, feeding back into the mix --
    auto fired = haunt.maybeFire(hauntP, sampleRate, rng);
    if (fired.kind == 1)
    {
        ghostTape[0].trigger(fired.backSec, fired.warpSec, fired.durationSec, fired.bits, fired.filterType, fired.filterFreq, hauntP);
        ghostTape[1].trigger(fired.backSec, fired.warpSec, fired.durationSec, fired.bits, fired.filterType, fired.filterFreq, hauntP);
    }
    double ghostL = ghostTape[0].process(shiftedL);
    double ghostR = ghostTape[1].process(shiftedR);

    double sumL = (shiftedL + ghostL) * outP;
    double sumR = (shiftedR + ghostR) * outP;

    *outL = static_cast<float>(softLimit(sumL));
    *outR = static_cast<float>(softLimit(sumR));

    // -- scope data for the editor's goniometer/spectrum (lock-free ring) --
    int wi = scope.writeIndex.load(std::memory_order_relaxed);
    scope.compA[static_cast<size_t>(wi)] = static_cast<float>(Z[0]);
    scope.compB[static_cast<size_t>(wi)] = static_cast<float>(Z[1]);
    scope.mono[static_cast<size_t>(wi)] = static_cast<float>((sumL + sumR) * 0.5);
    scope.writeIndex.store((wi + 1) % kScopeSize, std::memory_order_relaxed);
}

void QuaternionAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto numSamples = buffer.getNumSamples();

    auto mainOut = getBusBuffer(buffer, false, 0);
    auto* outL = mainOut.getWritePointer(0);
    auto* outR = mainOut.getNumChannels() > 1 ? mainOut.getWritePointer(1) : outL;

    auto scBuffer = getBusBuffer(buffer, true, 0);
    bool scActive = scBuffer.getNumChannels() >= 2;
    const float* scL = scActive ? scBuffer.getReadPointer(0) : nullptr;
    const float* scR = scActive ? scBuffer.getReadPointer(1) : nullptr;

    auto midiIt = midiMessages.cbegin();
    auto midiEnd = midiMessages.cend();

    for (int i = 0; i < numSamples; ++i)
    {
        while (midiIt != midiEnd && (*midiIt).samplePosition <= i)
        {
            handleMidiMessage((*midiIt).getMessage());
            ++midiIt;
        }
        renderSample(&outL[i], &outR[i], scL ? scL[i] : 0.0f, scR ? scR[i] : 0.0f);
    }

    midiMessages.clear();
}

int QuaternionAudioProcessor::getNumPrograms()
{
    return static_cast<int>(Presets::factoryPresets().size());
}

const juce::String QuaternionAudioProcessor::getProgramName(int index)
{
    const auto& presets = Presets::factoryPresets();
    if (index < 0 || index >= static_cast<int>(presets.size()))
        return {};
    return presets[static_cast<size_t>(index)].name;
}

void QuaternionAudioProcessor::setCurrentProgram(int index)
{
    const auto& presets = Presets::factoryPresets();
    if (index < 0 || index >= static_cast<int>(presets.size()))
        return;

    currentProgram = index;
    const auto& preset = presets[static_cast<size_t>(index)];
    const auto& ids = Presets::paramIds();

    for (size_t i = 0; i < ids.size(); ++i)
    {
        if (auto* param = apvts.getParameter(ids[i]))
        {
            float norm = param->convertTo0to1(preset.values[i]);
            param->setValueNotifyingHost(norm);
        }
    }

    updateHostDisplay(juce::AudioProcessorListener::ChangeDetails().withProgramChanged(true));
}

juce::AudioProcessorEditor* QuaternionAudioProcessor::createEditor()
{
    return new QuaternionAudioProcessorEditor(*this);
}

void QuaternionAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void QuaternionAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

// This creates new instances of the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new QuaternionAudioProcessor();
}
