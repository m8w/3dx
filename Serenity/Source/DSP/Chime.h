#pragma once
#include <array>
#include <cmath>
#include <random>

namespace SerenityDSP
{
    constexpr double kPi = 3.14159265358979323846;
}

// A short two-oscillator FM "bell": sine carrier + sine modulator into the
// carrier's frequency, fast attack / exponential decay, random pan.
struct ChimeVoice
{
    bool active = false;
    double carrierPhase = 0.0, modPhase = 0.0;
    double carrierFreq = 440.0, modFreq = 440.0, modDepth = 0.0;
    double env = 0.0, attackTarget = 0.15, decayCoeff = 0.999;
    double pan = 0.0;
    int attackSamplesLeft = 0;

    void trigger(double freq, double sampleRate, std::mt19937& rng)
    {
        std::uniform_real_distribution<double> uniMod(0.3, 0.8);
        std::uniform_real_distribution<double> uniPeak(0.10, 0.22);
        std::uniform_real_distribution<double> uniDecay(1.4, 3.2);
        std::uniform_real_distribution<double> uniPan(-0.85, 0.85);

        carrierFreq = freq;
        modFreq = freq * 2.01;
        modDepth = freq * uniMod(rng);
        carrierPhase = 0.0;
        modPhase = 0.0;
        env = 0.0;
        attackTarget = uniPeak(rng);
        attackSamplesLeft = std::max(1, static_cast<int>(0.006 * sampleRate));
        double decaySec = uniDecay(rng);
        decayCoeff = std::exp(-1.0 / (decaySec * sampleRate * 0.25));
        pan = uniPan(rng);
        active = true;
    }

    void render(double sampleRate, double* outL, double* outR)
    {
        if (!active)
        {
            *outL = 0.0;
            *outR = 0.0;
            return;
        }

        double modOut = std::sin(2.0 * SerenityDSP::kPi * modPhase);
        double freqMod = carrierFreq + modDepth * modOut;
        double carrierOut = std::sin(2.0 * SerenityDSP::kPi * carrierPhase);

        if (attackSamplesLeft > 0)
        {
            env += (attackTarget - env) / static_cast<double>(attackSamplesLeft);
            --attackSamplesLeft;
        }
        else
        {
            env *= decayCoeff;
            if (env < 0.0002)
                active = false;
        }

        double s = carrierOut * env;
        modPhase += modFreq / sampleRate;
        modPhase -= std::floor(modPhase);
        carrierPhase += freqMod / sampleRate;
        carrierPhase -= std::floor(carrierPhase);

        double l = std::sqrt(0.5 * (1.0 - pan));
        double r = std::sqrt(0.5 * (1.0 + pan));
        *outL = s * l;
        *outR = s * r;
    }
};

// Pool of chime voices plus the random scheduling clock. DENSITY (0..1)
// sets the average wait between random scale-degree bell hits.
struct ChimeBank
{
    static constexpr int kMaxVoices = 6;
    std::array<ChimeVoice, kMaxVoices> voices;
    long samplesUntilNext = 1;

    void prepare(double sampleRate, std::mt19937& rng)
    {
        for (auto& v : voices)
            v.active = false;
        armNext(0.35, sampleRate, rng);
    }

    void armNext(double density, double sampleRate, std::mt19937& rng)
    {
        double avgWaitSec = 16.0 - density * 14.5; // density 0 -> ~16s, 1 -> ~1.5s
        std::uniform_real_distribution<double> uniWait(avgWaitSec * 0.4, avgWaitSec * 1.6);
        samplesUntilNext = static_cast<long>(std::max(0.2, uniWait(rng)) * sampleRate);
    }

    void trigger(double freq, double sampleRate, std::mt19937& rng)
    {
        for (auto& v : voices)
        {
            if (!v.active)
            {
                v.trigger(freq, sampleRate, rng);
                return;
            }
        }
        voices[0].trigger(freq, sampleRate, rng); // pool full: steal the first slot
    }

    // Advances the clock by one sample; returns true when a new chime is due.
    bool tick(double density, double sampleRate, std::mt19937& rng)
    {
        if (--samplesUntilNext <= 0)
        {
            armNext(density, sampleRate, rng);
            return true;
        }
        return false;
    }

    void render(double sampleRate, double* outL, double* outR)
    {
        double l = 0.0, r = 0.0;
        for (auto& v : voices)
        {
            if (!v.active)
                continue;
            double vl, vr;
            v.render(sampleRate, &vl, &vr);
            l += vl;
            r += vr;
        }
        *outL = l;
        *outR = r;
    }
};
