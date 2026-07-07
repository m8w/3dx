#pragma once
#include "Biquad.h"
#include <cmath>
#include <array>

// One of the 4 quaternion-component voices. Continuously morphs
// sine -> triangle -> saw -> vocoder(formant) and wraps back to sine --
// CAYLEY II's original 3-point morph circle (WAVE + FAN spreading voices
// around it) extended with vocoder as a 4th waypoint, per spec.
struct Voice
{
    double phase = 0.0;
    double curFreq = 110.0;
    double envVal = 1.0;
    int envState = 0;     // 0 idle/sustain, 1 attack, 2 decay
    double lastOutput = 0.0;

    std::array<Biquad, 5> formants;

    void reset()
    {
        phase = 0.0; envVal = 1.0; envState = 0; lastOutput = 0.0;
        for (auto& f : formants) f.reset();
    }

    static double polyBlep(double t, double dt)
    {
        if (t < dt)
        {
            t /= dt;
            return t + t - t * t - 1.0;
        }
        if (t > 1.0 - dt)
        {
            t = (t - 1.0) / dt;
            return t * t + t + t + 1.0;
        }
        return 0.0;
    }

    static double sineWave(double ph) { return std::sin(2.0 * 3.14159265358979323846 * ph); }

    static double triWave(double ph)
    {
        double q = ph + 0.25; q -= std::floor(q);
        return 4.0 * std::abs(q - 0.5) - 1.0;
    }

    static double sawWave(double ph, double dt)
    {
        double naive = 2.0 * ph - 1.0;
        return naive - polyBlep(ph, dt);
    }

    // 5-band formant filter bank driven by a saw carrier; gains blend a
    // fixed vowel-ish curve with live sidechain band envelopes (true-vocoder
    // path), per "formant" and "scMix" params.
    double vocoderWave(double ph, double dt, double sampleRate, const std::array<double, 5>& formantFreqs,
                       const std::array<double, 5>& fixedGains, const std::array<double, 5>& scGains, double scMix)
    {
        double carrier = sawWave(ph, dt);
        double out = 0.0;
        for (int i = 0; i < 5; ++i)
        {
            formants[static_cast<size_t>(i)].setBandpass(formantFreqs[static_cast<size_t>(i)], 8.0, sampleRate);
            double band = formants[static_cast<size_t>(i)].process(carrier);
            double g = fixedGains[static_cast<size_t>(i)] * (1.0 - scMix) + scGains[static_cast<size_t>(i)] * scMix;
            out += band * g;
        }
        return std::tanh(out * 1.6);
    }

    // Advances phase/env by one sample and returns the raw voice signal.
    // fmPhaseOffset: extra phase increment this sample from cross-voice FM.
    double render(double sampleRate, double targetFreq, double glideCoeff, double level,
                  double wavePos, double fmPhaseOffset, bool droneMode, double envAttackTarget,
                  double decayCoeff, const std::array<double, 5>& formantFreqs,
                  const std::array<double, 5>& fixedGains, const std::array<double, 5>& scGains, double scMix)
    {
        curFreq += (targetFreq - curFreq) * glideCoeff;
        double dt = curFreq / sampleRate;

        if (droneMode)
        {
            envVal += (1.0 - envVal) * 0.001;
        }
        else if (envState == 1)
        {
            envVal += (envAttackTarget - envVal) * 0.02;
            if (envVal >= envAttackTarget * 0.999) { envVal = envAttackTarget; envState = 2; }
        }
        else if (envState == 2 && decayCoeff < 1.0)
        {
            envVal *= decayCoeff;
        }

        double pos = wavePos - std::floor(wavePos);
        double scaled = pos * 4.0;
        int segment = static_cast<int>(scaled) & 3;
        double frac = scaled - std::floor(scaled);

        auto genAnchor = [&](int a) -> double
        {
            switch (a)
            {
                case 0: return sineWave(phase);
                case 1: return triWave(phase);
                case 2: return sawWave(phase, dt);
                default: return vocoderWave(phase, dt, sampleRate, formantFreqs, fixedGains, scGains, scMix);
            }
        };

        double s = genAnchor(segment) * (1.0 - frac) + genAnchor((segment + 1) & 3) * frac;
        double out = level * envVal * s;
        lastOutput = out;

        phase += dt + fmPhaseOffset;
        phase -= std::floor(phase);
        return out;
    }
};
