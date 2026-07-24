#pragma once
#include "Quaternion.h"
#include "Biquad.h"
#include "DelayLine.h"
#include <array>
#include <vector>
#include <random>
#include <cmath>
#include <algorithm>

// CAYLEY II's "memory" intrusions, ported to 4 components. Four kinds of
// haunting, chosen at random at an interval set by the HAUNT knob:
//   ghost   - a past output resurfaces through a warping tape delay + bit
//             crush + roaming bandpass, feeding back into the mix
//   memory  - a stored snapshot of Z multiplies the present via quaternion
//             product (the algebra literally haunting itself)
//   swap    - two voice components trade places, one negated
//   stasis  - the rotor freezes while everything else keeps moving
struct AlgebraHaunt
{
    static constexpr int HIST_N = 2048;

    int intrude = 0;
    long intrudeEnd = 0;
    double intrudeEnv = 0.0;
    int swapA = 1, swapB = 2;
    std::array<double, 4> M { 1, 0, 0, 0 };
    long sampleCount = 0;
    long nextIntrSample = 0;

    std::vector<std::array<double, 4>> hist;
    int hIdx = 0, hEvery = 256, hCtr = 0;

    void prepare(double sampleRate)
    {
        hist.assign(HIST_N, { 0, 0, 0, 0 });
        hIdx = 0; hCtr = 0; sampleCount = 0; intrude = 0; intrudeEnv = 0;
        nextIntrSample = static_cast<long>(4.0 * sampleRate);
    }

    bool stasisActive() const { return intrude == 3 && sampleCount < intrudeEnd; }

    // Mutates Z in place with any active intrusion, records rolling history.
    void applyAndRecord(double* Z)
    {
        if (intrude != 0 && sampleCount < intrudeEnd)
        {
            if (intrudeEnv < 1.0) intrudeEnv += 0.0006;
            applyIntrudeEffect(Z, intrudeEnv);
        }
        else if (intrude != 0 && sampleCount >= intrudeEnd)
        {
            if (intrudeEnv > 0.0) intrudeEnv -= 0.0008;
            else intrude = 0;
            if ((intrude == 1 || intrude == 2) && intrudeEnv > 0.0)
                applyIntrudeEffect(Z, intrudeEnv);
        }

        if (++hCtr >= hEvery)
        {
            hCtr = 0;
            hist[static_cast<size_t>(hIdx)] = { Z[0], Z[1], Z[2], Z[3] };
            hIdx = (hIdx + 1) % HIST_N;
        }
        ++sampleCount;
    }

    struct FireResult
    {
        int kind = 0; // 0 none, 1 ghost, 2 memory, 3 swap, 4 stasis
        double durationSec = 0, backSec = 0, warpSec = 0;
        int bits = 8, filterType = 0;
        double filterFreq = 800;
    };

    // Call once per block (or every N samples) with haunt01 in [0,1]. Fires
    // at most one intrusion per call when due.
    template <typename RNG>
    FireResult maybeFire(double haunt01, double sampleRate, RNG& rng)
    {
        FireResult r;
        if (haunt01 <= 0.01 || sampleCount < nextIntrSample) return r;

        std::uniform_real_distribution<double> uni(0.0, 1.0);
        double rr = uni(rng);
        double meanSec = std::max(0.5, 24.0 - 21.0 * haunt01);
        double interval = meanSec * (0.35 - std::log(1.0 - uni(rng) * 0.95));
        nextIntrSample = sampleCount + static_cast<long>(interval * sampleRate);

        if (rr < 0.38)
        {
            r.kind = 1;
            r.durationSec = 0.6 + uni(rng) * 2.2;
            r.backSec = 0.5 + uni(rng) * 9.0;
            r.warpSec = (uni(rng) - 0.5) * 1.8;
            r.bits = 3 + static_cast<int>(uni(rng) * 7);
            r.filterType = static_cast<int>(uni(rng) * 3);
            r.filterFreq = 300 + uni(rng) * 2700;
        }
        else if (rr < 0.63)
        {
            r.kind = 2;
            r.durationSec = 0.8 + uni(rng) * 2.5;
            int stepsBack = 200 + static_cast<int>(uni(rng) * (HIST_N - 300));
            int idx = ((hIdx - stepsBack) % HIST_N + HIST_N) % HIST_N;
            double s = 0;
            for (int k = 0; k < 4; ++k) { M[static_cast<size_t>(k)] = hist[static_cast<size_t>(idx)][static_cast<size_t>(k)]; s += M[static_cast<size_t>(k)] * M[static_cast<size_t>(k)]; }
            s = std::sqrt(s); if (s < 1e-9) s = 1.0;
            for (auto& v : M) v /= s;
            intrude = 1; // memory path
            intrudeEnv = 0.0;
            intrudeEnd = sampleCount + static_cast<long>(r.durationSec * sampleRate);
        }
        else if (rr < 0.84)
        {
            r.kind = 3;
            r.durationSec = 0.5 + uni(rng) * 1.5;
            int a = 1 + static_cast<int>(uni(rng) * 3);
            int b = a; while (b == a) b = 1 + static_cast<int>(uni(rng) * 3);
            swapA = a; swapB = b;
            intrude = 2;
            intrudeEnv = 0.0;
            intrudeEnd = sampleCount + static_cast<long>(r.durationSec * sampleRate);
        }
        else
        {
            r.kind = 4;
            r.durationSec = 1.0 + uni(rng) * 2.5;
            intrude = 3;
            intrudeEnv = 0.0;
            intrudeEnd = sampleCount + static_cast<long>(r.durationSec * sampleRate);
        }
        return r;
    }

private:
    void applyIntrudeEffect(double* Z, double env)
    {
        if (intrude == 1)
        {
            Quat m = Quat::fromArray(M.data());
            Quat z = Quat::fromArray(Z);
            Quat t = m * z;
            double ta[4]; t.toArray(ta);
            for (int k = 0; k < 4; ++k) Z[k] = Z[k] * (1 - env) + ta[k] * env;
        }
        else if (intrude == 2)
        {
            double a = Z[swapA], b = Z[swapB];
            Z[swapA] = Z[swapA] * (1 - env) + (-b) * env;
            Z[swapB] = Z[swapB] * (1 - env) + a * env;
        }
    }
};

// Per-channel ghost tape: feedback delay -> bit crush -> roaming filter,
// summed back into the mix. Run one instance per audio channel.
struct GhostTape
{
    DelayLine delay;
    Biquad filt;
    double sampleRate = 44100.0;
    double delayTimeCur = 4.0, delayTimeTarget = 4.0, delayRampPerSample = 0.0;
    long delayRampSamplesLeft = 0;
    double crushBits = 8.0;
    double ghostGain = 0.0, ghostGainTarget = 0.0;
    long holdUntilSample = 0;
    long sampleCount = 0;

    void prepare(double sr)
    {
        sampleRate = sr;
        delay.prepare(12.0, sr);
        filt.reset();
        ghostGain = 0; ghostGainTarget = 0;
        sampleCount = 0;
    }

    void trigger(double backSec, double warpSec, double durSec, int bits, int filterType, double freq, double haunt01)
    {
        delayTimeCur = std::clamp(backSec, 0.05, 11.0);
        delayTimeTarget = std::clamp(backSec + warpSec, 0.05, 11.0);
        delayRampSamplesLeft = std::max<long>(1, static_cast<long>(durSec * sampleRate));
        delayRampPerSample = (delayTimeTarget - delayTimeCur) / static_cast<double>(delayRampSamplesLeft);
        crushBits = static_cast<double>(bits);
        if (filterType == 0) filt.setBandpass(freq, 1.1, sampleRate);
        else if (filterType == 1) filt.setHighpass(freq, 0.9, sampleRate);
        else filt.setLowpass(freq, 0.9, sampleRate);
        ghostGainTarget = 0.55 + 0.25 * haunt01;
        holdUntilSample = sampleCount + static_cast<long>(durSec * sampleRate);
    }

    double crush(double x) const
    {
        double lv = std::pow(2.0, crushBits);
        return std::round(x * lv) / lv;
    }

    double process(double mixInput)
    {
        delay.push(mixInput);
        double d = delay.read(delayTimeCur);
        if (delayRampSamplesLeft > 0) { delayTimeCur += delayRampPerSample; --delayRampSamplesLeft; }

        if (sampleCount >= holdUntilSample) ghostGainTarget = 0.0;
        ghostGain += (ghostGainTarget - ghostGain) * (ghostGainTarget < ghostGain ? 0.00005 : 0.002);

        double c = crush(d);
        double f = filt.process(c);
        ++sampleCount;
        return f * ghostGain;
    }
};
