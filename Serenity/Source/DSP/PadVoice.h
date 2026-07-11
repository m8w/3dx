#pragma once
#include <cmath>
#include <random>

// One ambient pad voice: a triangle oscillator whose amplitude swells and
// fades on its own random-period clock (Eno "Music for Airports" style),
// plus a slowly drifting stereo pan. targetFreq is set externally each
// sample (root * scale degree * octave offset, including root drift).
struct PadVoice
{
    double phase = 0.0;
    double curFreq = 110.0;
    double targetFreq = 110.0;

    double envVal = 0.0;
    double envTarget = 0.1;
    long swellSamplesLeft = 1;

    double pan = 0.0;
    double panTarget = 0.0;

    void reset()
    {
        phase = 0.0;
        envVal = 0.0;
        swellSamplesLeft = 1;
    }

    static double triWave(double ph)
    {
        double q = ph + 0.25;
        q -= std::floor(q);
        return 4.0 * std::abs(q - 0.5) - 1.0;
    }

    void rearm(std::mt19937& rng, double sampleRate)
    {
        std::uniform_real_distribution<double> uniAmp(0.03, 0.22);
        std::uniform_real_distribution<double> uniDur(4.0, 14.0);
        std::uniform_real_distribution<double> uniPan(-0.7, 0.7);
        envTarget = uniAmp(rng);
        swellSamplesLeft = static_cast<long>(uniDur(rng) * sampleRate);
        panTarget = uniPan(rng);
    }

    // Advances one sample and writes the stereo contribution into outL/outR.
    void render(double sampleRate, double glideCoeff, std::mt19937& rng, double* outL, double* outR)
    {
        curFreq += (targetFreq - curFreq) * glideCoeff;
        double dt = curFreq / sampleRate;

        double envRate = 1.0 / (0.5 * sampleRate);
        envVal += (envTarget - envVal) * envRate;

        if (--swellSamplesLeft <= 0)
            rearm(rng, sampleRate);

        pan += (panTarget - pan) / (2.0 * sampleRate);

        double s = triWave(phase) * envVal;
        phase += dt;
        phase -= std::floor(phase);

        double l = std::sqrt(0.5 * (1.0 - pan));
        double r = std::sqrt(0.5 * (1.0 + pan));
        *outL = s * l;
        *outR = s * r;
    }
};
