#pragma once
#include "Biquad.h"
#include "DelayLine.h"
#include <array>
#include <cmath>

// Bode frequency shifter -- a faithful native port of SHIFT's DSP core.
//
// Where the HTML version routed Niemitalo's allpass sections through
// generic Web Audio BiquadFilterNodes pinned at fs/4 (a workaround for
// Web Audio having no raw allpass primitive), this implements the exact
// z^-2 allpass transfer function directly:
//   H(z) = (z^-2 - a^2) / (1 - a^2 z^-2)
// which is sample-rate independent and needs no RBJ Q juggling.
// One HilbertShifter instance runs per audio channel (matches the original,
// where per-channel BiquadFilterNodes gave true independent stereo shift).
class HilbertShifter
{
public:
    void prepare(double sampleRateIn)
    {
        sampleRate = sampleRateIn;
        for (auto& s : chainA) s.reset();
        for (auto& s : chainB) s.reset();
        d1 = 0.0;
        loPhase = 0.0;
        fbHP.setHighpass(28.0, 0.707, sampleRate);
        fbLP.setLowpass(13500.0, 0.707, sampleRate);
        fbHP.reset(); fbLP.reset();
        fbDelay.prepare(1.0, sampleRate);
    }

    void reset()
    {
        for (auto& s : chainA) s.reset();
        for (auto& s : chainB) s.reset();
        d1 = 0.0;
        fbHP.reset(); fbLP.reset();
        fbDelay.reset();
    }

    // shiftHz: Delta frequency. bal: 0=down .. 0.5=ring-mod .. 1=up.
    // fb/fbTimeSec: feedback amount and delay time. mixAmt: dry/wet.
    double process(double input, double shiftHz, double bal, double fb, double fbTimeSec, double mixAmt)
    {
        double sum = input + fbDelay.read(fbTimeSec) * fbGainSmoothed(fb);

        // -- Hilbert pair: chain A -> 1-sample delay = I, chain B = Q --
        double sA = sum;
        for (auto& s : chainA) sA = s.process(sA);
        // exact one-sample delay compensating chain B's extra group delay
        double I = d1;
        d1 = sA;
        double Q = sum;
        for (auto& s : chainB) Q = s.process(Q);

        // -- quadrature LO, phase-locked cos/sin --
        loPhase += shiftHz / sampleRate;
        loPhase -= std::floor(loPhase);
        double c = std::cos(2.0 * pi * loPhase);
        double s = std::sin(2.0 * pi * loPhase);

        double up = I * c - Q * s;
        double down = I * c + Q * s;

        double upG = std::sin(bal * pi / 2.0);
        double dnG = std::cos(bal * pi / 2.0);
        double wet = up * upG + down * dnG;

        // -- feedback loop: wet -> HPF -> LPF -> delay --
        double f = fbHP.process(wet);
        f = fbLP.process(f);
        fbDelay.push(f);

        double dryG = std::cos(mixAmt * pi / 2.0);
        double wetG = std::sin(mixAmt * pi / 2.0);
        return input * dryG + wet * wetG;
    }

private:
    static constexpr double pi = 3.14159265358979323846;

    // Niemitalo Hilbert-pair allpass coefficients (chain A carries +1 sample delay)
    static constexpr double A_COEF[4] = { 0.6923877778065359, 0.9360654322959357,
                                           0.9882295226860673, 0.9987488452737823 };
    static constexpr double B_COEF[4] = { 0.4021921162426, 0.8561710882420,
                                           0.9722909545651, 0.9952884791278 };

    struct AllpassZ2
    {
        double a2 = 0.0, x1 = 0, x2 = 0, y1 = 0, y2 = 0;
        double process(double x)
        {
            // y[n] = a2*(y[n-2] - x[n]) + x[n-2]
            double y = a2 * (y2 - x) + x2;
            x2 = x1; x1 = x;
            y2 = y1; y1 = y;
            return y;
        }
        void reset() { x1 = x2 = y1 = y2 = 0; }
    };

    static std::array<AllpassZ2, 4> makeChain(const double* coefs)
    {
        std::array<AllpassZ2, 4> c;
        for (size_t i = 0; i < 4; ++i) c[i].a2 = coefs[i] * coefs[i];
        return c;
    }

    std::array<AllpassZ2, 4> chainA = makeChain(A_COEF);
    std::array<AllpassZ2, 4> chainB = makeChain(B_COEF);
    double d1 = 0.0;
    double loPhase = 0.0;
    double sampleRate = 44100.0;

    Biquad fbHP, fbLP;
    DelayLine fbDelay;

    static double fbGainSmoothed(double fb) { return fb * 0.9; }
};
