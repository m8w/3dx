#pragma once
#include <vector>
#include <algorithm>
#include <cmath>

// Simple circular delay line with linear-interpolated read, sample-accurate
// write. Backs the shift-chain feedback delay and the haunt "tape" delay.
struct DelayLine
{
    std::vector<double> buf;
    int writePos = 0;
    double sampleRate = 44100.0;

    void prepare(double maxSeconds, double sr)
    {
        sampleRate = sr;
        buf.assign(static_cast<size_t>(std::ceil(maxSeconds * sr)) + 4, 0.0);
        writePos = 0;
    }

    void reset() { std::fill(buf.begin(), buf.end(), 0.0); writePos = 0; }

    void push(double x)
    {
        buf[static_cast<size_t>(writePos)] = x;
        writePos = (writePos + 1) % static_cast<int>(buf.size());
    }

    double read(double delaySeconds) const
    {
        double delaySamples = std::clamp(delaySeconds * sampleRate, 0.0,
                                          static_cast<double>(buf.size()) - 2.0);
        double readPos = static_cast<double>(writePos) - delaySamples;
        int n = static_cast<int>(buf.size());
        while (readPos < 0) readPos += n;
        int i0 = static_cast<int>(readPos) % n;
        int i1 = (i0 + 1) % n;
        double frac = readPos - std::floor(readPos);
        return buf[static_cast<size_t>(i0)] * (1.0 - frac) + buf[static_cast<size_t>(i1)] * frac;
    }
};
