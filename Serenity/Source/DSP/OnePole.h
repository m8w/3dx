#pragma once
#include <cmath>

// One-pole lowpass used for the BRIGHTNESS control on the pad bus and for
// damping inside the space delay taps.
struct OnePoleLP
{
    double state = 0.0;
    double coeff = 0.5;

    void setCutoff(double freqHz, double sampleRate)
    {
        double x = std::exp(-2.0 * 3.14159265358979323846 * freqHz / sampleRate);
        coeff = 1.0 - x;
    }

    double process(double in)
    {
        state += (in - state) * coeff;
        return state;
    }

    void reset() { state = 0.0; }
};
