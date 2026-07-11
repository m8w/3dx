#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

// Parameter IDs for SERENITY -- a free-running ambient pad + random-chime
// generator. See README.md for the control map.
namespace Params
{
    static constexpr const char* density    = "density";
    static constexpr const char* space      = "space";
    static constexpr const char* brightness = "brightness";
    static constexpr const char* drift      = "drift";
    static constexpr const char* scale      = "scale";
    static constexpr const char* output     = "output";

    inline juce::StringArray scaleChoices()
    {
        return { "MAJOR PENT", "MINOR PENT", "DORIAN", "LYDIAN" };
    }

    inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
    {
        using P = juce::AudioParameterFloat;
        using NR = juce::NormalisableRange<float>;
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

        p.push_back(std::make_unique<P>(juce::ParameterID{ density, 1 },    "Density",    NR(0.f, 1.f), 0.35f));
        p.push_back(std::make_unique<P>(juce::ParameterID{ space, 1 },      "Space",      NR(0.f, 1.f), 0.55f));
        p.push_back(std::make_unique<P>(juce::ParameterID{ brightness, 1 }, "Brightness", NR(0.f, 1.f), 0.5f));
        p.push_back(std::make_unique<P>(juce::ParameterID{ drift, 1 },      "Drift",      NR(0.f, 1.f), 0.4f));
        p.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{ scale, 1 }, "Scale", scaleChoices(), 0));
        p.push_back(std::make_unique<P>(juce::ParameterID{ output, 1 },     "Output",     NR(0.f, 1.f), 0.7f));

        return { p.begin(), p.end() };
    }
}
