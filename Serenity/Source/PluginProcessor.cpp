#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>

namespace
{
    // Five-degree scale tables (semitones within the octave) shared by the
    // pad chord and the chime picker. DORIAN/LYDIAN are trimmed to their
    // most characteristic five degrees so pads stay consonant.
    constexpr std::array<std::array<int, 5>, 4> kScaleTables { {
        { 0, 2, 4, 7, 9 },  // MAJOR PENT
        { 0, 3, 5, 7, 10 }, // MINOR PENT
        { 0, 2, 3, 7, 9 },  // DORIAN
        { 0, 2, 4, 6, 7 },  // LYDIAN
    } };

    // Fixed octave spread across the 5 pad voices (bass pair / root / high pair).
    constexpr std::array<int, 5> kOctaveShift { -12, -12, 0, 12, 12 };
    constexpr std::array<int, 3> kChimeOctaves { 12, 24, 36 };
}

SerenityAudioProcessor::SerenityAudioProcessor()
    : apvts(*this, nullptr, "PARAMS", Params::createLayout())
{
    cacheParamPointers();
}

void SerenityAudioProcessor::cacheParamPointers()
{
    pDensity = apvts.getRawParameterValue(Params::density);
    pSpace = apvts.getRawParameterValue(Params::space);
    pBrightness = apvts.getRawParameterValue(Params::brightness);
    pDrift = apvts.getRawParameterValue(Params::drift);
    pScale = apvts.getRawParameterValue(Params::scale);
    pOutput = apvts.getRawParameterValue(Params::output);
}

void SerenityAudioProcessor::prepareToPlay(double sr, int)
{
    sampleRate = sr;

    for (auto& v : pads)
    {
        v.reset();
        v.curFreq = v.targetFreq = rootFreq;
    }
    chimes.prepare(sr, rng);
    space.prepare(sr);
    toneL.reset();
    toneR.reset();

    driftSemis = 0.0;
    driftCounterSamples = static_cast<long>(sr * 2.0);
    levelAccum = 0.0f;
    levelCount = 0;
}

bool SerenityAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void SerenityAudioProcessor::handleMidiMessage(const juce::MidiMessage& msg)
{
    if (msg.isNoteOn())
        rootFreq = midiToHz(msg.getNoteNumber());
}

void SerenityAudioProcessor::renderSample(float* outL, float* outR)
{
    double densityP = *pDensity, spaceP = *pSpace, brightP = *pBrightness, driftP = *pDrift, outP = *pOutput;
    auto scaleIdx = static_cast<size_t>(juce::jlimit(0, 3, static_cast<int>(*pScale)));
    const auto& degrees = kScaleTables[scaleIdx];

    // -- slow root drift, random walk bounded by DRIFT --
    if (--driftCounterSamples <= 0)
    {
        std::uniform_real_distribution<double> uniStep(-1.0, 1.0);
        double maxWander = 5.0 * driftP;
        driftSemis += uniStep(rng) * (0.6 + driftP);
        driftSemis = std::clamp(driftSemis, -maxWander, maxWander);
        std::uniform_real_distribution<double> uniInt(4.0, 9.0);
        driftCounterSamples = static_cast<long>(uniInt(rng) * sampleRate);
    }

    // -- 5 pad voices, own random swell clocks, ~3.5s glide toward drift --
    double glideCoeff = 1.0 / (3.5 * sampleRate);
    double padL = 0.0, padR = 0.0;
    for (int i = 0; i < 5; ++i)
    {
        pads[static_cast<size_t>(i)].targetFreq =
            rootFreq * std::pow(2.0, (degrees[static_cast<size_t>(i)] + kOctaveShift[static_cast<size_t>(i)] + driftSemis) / 12.0);
        double vl, vr;
        pads[static_cast<size_t>(i)].render(sampleRate, glideCoeff, rng, &vl, &vr);
        padL += vl;
        padR += vr;
    }

    double cutoff = 300.0 + brightP * brightP * 6000.0;
    toneL.setCutoff(cutoff, sampleRate);
    toneR.setCutoff(cutoff, sampleRate);
    padL = toneL.process(padL);
    padR = toneR.process(padR);

    // -- random chime scheduler --
    if (chimes.tick(densityP, sampleRate, rng))
    {
        std::uniform_int_distribution<int> degPick(0, 4);
        std::uniform_int_distribution<int> octPick(0, 2);
        double freq = rootFreq * std::pow(2.0, (degrees[static_cast<size_t>(degPick(rng))]
                                                  + kChimeOctaves[static_cast<size_t>(octPick(rng))] + driftSemis) / 12.0);
        chimes.trigger(freq, sampleRate, rng);
    }
    double chimeL, chimeR;
    chimes.render(sampleRate, &chimeL, &chimeR);

    double dryL = padL + chimeL;
    double dryR = padR + chimeR;

    double wetL, wetR;
    space.process(dryL, dryR, &wetL, &wetR);

    double mixL = dryL * (1.0 - spaceP * 0.6) + wetL * spaceP;
    double mixR = dryR * (1.0 - spaceP * 0.6) + wetR * spaceP;

    double sampleOutL = std::tanh(mixL * outP * 1.1);
    double sampleOutR = std::tanh(mixR * outP * 1.1);
    *outL = static_cast<float>(sampleOutL);
    *outR = static_cast<float>(sampleOutR);

    levelAccum += static_cast<float>(std::abs(sampleOutL) + std::abs(sampleOutR)) * 0.5f;
    ++levelCount;
}

void SerenityAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto numSamples = buffer.getNumSamples();

    auto* outL = buffer.getWritePointer(0);
    auto* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : outL;

    auto midiIt = midiMessages.cbegin();
    auto midiEnd = midiMessages.cend();

    for (int i = 0; i < numSamples; ++i)
    {
        while (midiIt != midiEnd && (*midiIt).samplePosition <= i)
        {
            handleMidiMessage((*midiIt).getMessage());
            ++midiIt;
        }
        renderSample(&outL[i], &outR[i]);
    }

    midiMessages.clear();

    if (levelCount > 0)
    {
        outputLevel.store(levelAccum / static_cast<float>(levelCount), std::memory_order_relaxed);
        levelAccum = 0.0f;
        levelCount = 0;
    }
}

juce::AudioProcessorEditor* SerenityAudioProcessor::createEditor()
{
    return new SerenityAudioProcessorEditor(*this);
}

void SerenityAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    xml->setAttribute("themeIndex", themeIndex.load());
    copyXmlToBinary(*xml, destData);
}

void SerenityAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName(apvts.state.getType()))
    {
        themeIndex.store(xml->getIntAttribute("themeIndex", 0));
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
    }
}

// This creates new instances of the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SerenityAudioProcessor();
}
