#pragma once
#include "Params.h"
#include <array>
#include <vector>

// Factory presets -- ambient/drone starting points built from the raw
// parameter set. Values are in the same order Params::createLayout()
// declares its parameters, so Preset::values[i] maps to Presets::paramIds()[i].
namespace Presets
{
    inline const std::array<const char*, 22>& paramIds()
    {
        static const std::array<const char*, 22> ids
        {
            Params::wave, Params::fan, Params::formant, Params::scMix,
            Params::rate, Params::range, Params::glide, Params::gate, Params::env, Params::pattern,
            Params::spin, Params::drift, Params::depth, Params::algMode, Params::fmAmount,
            Params::shiftHz, Params::balance, Params::feedback, Params::fbTime, Params::shiftMix,
            Params::haunt, Params::output
        };
        return ids;
    }

    struct Preset
    {
        const char* name;
        std::array<float, 22> values; // wave fan formant scMix | rate range glide gate env pattern
                                       // | spin drift depth algMode fmAmount | shiftHz balance feedback fbTime shiftMix
                                       // | haunt output
    };

    // pattern indices: DRONE=0 LADDER=1 WALK=2 PRIME=3 FRACTAL=4
    // algMode indices: ROTATE=0 CASCADE=1 CONJUGATE=2
    inline const std::vector<Preset>& factoryPresets()
    {
        static const std::vector<Preset> presets
        {
            { "Deep Drone",         { 0.05f, 0.15f, 0.20f, 0.00f,  0.06f, 8.f,  0.90f, 0.00f, 0.10f, 0.f,
                                       0.15f, 0.15f, 0.50f, 0.f, 0.05f,  2.5f,  0.50f, 0.35f, 0.40f, 0.30f,
                                       0.20f, 0.70f } },

            { "Glacial Drift",      { 0.20f, 0.40f, 0.35f, 0.00f,  0.15f, 10.f, 0.85f, 0.15f, 0.20f, 2.f,
                                       0.40f, 0.50f, 0.60f, 1.f, 0.15f,  5.0f,  0.60f, 0.45f, 0.50f, 0.40f,
                                       0.35f, 0.68f } },

            { "Haunted Choir",      { 0.70f, 0.50f, 0.60f, 0.20f,  0.10f, 8.f,  0.60f, 0.20f, 0.30f, 4.f,
                                       1.00f, 0.30f, 0.55f, 2.f, 0.10f,  3.0f,  0.55f, 0.40f, 0.60f, 0.35f,
                                       0.70f, 0.65f } },

            { "Frozen Fractal Bloom", { 0.35f, 0.60f, 0.30f, 0.00f, 0.25f, 14.f, 0.70f, 0.30f, 0.35f, 4.f,
                                       2.50f, 0.35f, 0.70f, 1.f, 0.20f,  7.0f,  0.60f, 0.50f, 0.35f, 0.45f,
                                       0.40f, 0.66f } },

            { "Shimmer Wash",       { 0.10f, 0.25f, 0.20f, 0.00f,  0.08f, 8.f,  0.95f, 0.00f, 0.10f, 0.f,
                                       0.30f, 0.20f, 0.50f, 0.f, 0.08f,  12.0f, 0.65f, 0.65f, 0.18f, 0.60f,
                                       0.15f, 0.62f } },

            { "Ghost Tape Loop",    { 0.15f, 0.30f, 0.25f, 0.00f,  0.05f, 6.f,  0.80f, 0.10f, 0.15f, 2.f,
                                       0.50f, 0.40f, 0.45f, 0.f, 0.10f,  4.0f,  0.50f, 0.30f, 0.70f, 0.30f,
                                       0.85f, 0.60f } },

            { "Quaternion Pad",     { 0.30f, 0.55f, 0.40f, 0.10f,  0.20f, 10.f, 0.75f, 0.25f, 0.30f, 1.f,
                                       1.50f, 0.35f, 0.65f, 1.f, 0.20f,  6.0f,  0.60f, 0.45f, 0.30f, 0.40f,
                                       0.30f, 0.68f } },

            { "Subharmonic Void",   { 0.02f, 0.10f, 0.15f, 0.00f,  0.04f, 16.f, 0.90f, 0.00f, 0.05f, 3.f,
                                       0.05f, 0.10f, 0.40f, 2.f, 0.02f,  0.6f,  0.40f, 0.25f, 0.75f, 0.20f,
                                       0.10f, 0.72f } },
        };
        return presets;
    }
}
