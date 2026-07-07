#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

// Parameter IDs for QUATERNION -- the VST descendant of SHIFT + CAYLEY II.
// See README.md for how each control maps back to the two original HTML toys.
namespace Params
{
    static constexpr const char* wave      = "wave";
    static constexpr const char* fan       = "fan";
    static constexpr const char* formant   = "formant";
    static constexpr const char* scMix     = "scMix";

    static constexpr const char* rate      = "rate";
    static constexpr const char* range     = "range";
    static constexpr const char* glide     = "glide";
    static constexpr const char* gate      = "gate";
    static constexpr const char* env       = "env";
    static constexpr const char* pattern   = "pattern";

    static constexpr const char* spin      = "spin";
    static constexpr const char* drift     = "drift";
    static constexpr const char* depth     = "depth";
    static constexpr const char* algMode   = "algMode";
    static constexpr const char* fmAmount  = "fmAmount";

    static constexpr const char* shiftHz   = "shiftHz";
    static constexpr const char* balance   = "balance";
    static constexpr const char* feedback  = "feedback";
    static constexpr const char* fbTime    = "fbTime";
    static constexpr const char* shiftMix  = "shiftMix";

    static constexpr const char* haunt     = "haunt";
    static constexpr const char* output    = "output";

    inline juce::StringArray patternChoices()
    {
        return { "DRONE", "LADDER", "WALK", "PRIME", "FRACTAL" };
    }
    inline juce::StringArray algModeChoices()
    {
        return { "ROTATE", "CASCADE", "CONJUGATE" };
    }

    inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
    {
        using P = juce::AudioParameterFloat;
        using NR = juce::NormalisableRange<float>;
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

        auto logRange = [](float lo, float hi)
        {
            NR r(lo, hi);
            r.setSkewForCentre(std::sqrt(lo * hi));
            return r;
        };

        p.push_back(std::make_unique<P>(juce::ParameterID{ wave, 1 },     "Wave",       NR(0.f, 1.f), 0.0f));
        p.push_back(std::make_unique<P>(juce::ParameterID{ fan, 1 },      "Fan",        NR(0.f, 1.f), 0.5f));
        p.push_back(std::make_unique<P>(juce::ParameterID{ formant, 1 },  "Formant",    NR(0.f, 1.f), 0.3f));
        p.push_back(std::make_unique<P>(juce::ParameterID{ scMix, 1 },    "SC Mix",     NR(0.f, 1.f), 0.0f));

        p.push_back(std::make_unique<P>(juce::ParameterID{ rate, 1 },    "Rate",   logRange(0.02f, 20.f), 1.2f));
        p.push_back(std::make_unique<P>(juce::ParameterID{ range, 1 },   "Range",  NR(2.f, 16.f, 1.f), 12.f));
        p.push_back(std::make_unique<P>(juce::ParameterID{ glide, 1 },   "Glide",  NR(0.f, 1.f), 0.35f));
        p.push_back(std::make_unique<P>(juce::ParameterID{ gate, 1 },    "Gate",   NR(0.f, 1.f), 0.8f));
        p.push_back(std::make_unique<P>(juce::ParameterID{ env, 1 },     "Env",    NR(0.f, 1.f), 0.4f));
        p.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{ pattern, 1 }, "Pattern", patternChoices(), 1));

        p.push_back(std::make_unique<P>(juce::ParameterID{ spin, 1 },   "Spin",  logRange(0.02f, 500.f), 3.f));
        p.push_back(std::make_unique<P>(juce::ParameterID{ drift, 1 },  "Drift", NR(0.f, 1.f), 0.3f));
        p.push_back(std::make_unique<P>(juce::ParameterID{ depth, 1 },  "Depth", NR(0.f, 1.f), 1.f));
        p.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{ algMode, 1 }, "Algebra", algModeChoices(), 0));
        p.push_back(std::make_unique<P>(juce::ParameterID{ fmAmount, 1 }, "FM Amount", NR(0.f, 1.f), 0.0f));

        p.push_back(std::make_unique<P>(juce::ParameterID{ shiftHz, 1 },  "Shift",     logRange(0.1f, 2000.f), 1.7f));
        p.push_back(std::make_unique<P>(juce::ParameterID{ balance, 1 }, "Balance",   NR(0.f, 1.f), 1.f));
        p.push_back(std::make_unique<P>(juce::ParameterID{ feedback, 1 },"Feedback",  NR(0.f, 1.f), 0.35f));
        p.push_back(std::make_unique<P>(juce::ParameterID{ fbTime, 1 },  "FB Time",   logRange(0.003f, 0.8f), 0.19f));
        p.push_back(std::make_unique<P>(juce::ParameterID{ shiftMix, 1 },"Shift Mix", NR(0.f, 1.f), 0.6f));

        p.push_back(std::make_unique<P>(juce::ParameterID{ haunt, 1 },  "Haunt",  NR(0.f, 1.f), 0.5f));
        p.push_back(std::make_unique<P>(juce::ParameterID{ output, 1 }, "Output", NR(0.f, 1.f), 0.7f));

        return { p.begin(), p.end() };
    }
}
