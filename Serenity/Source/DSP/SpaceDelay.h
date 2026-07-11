#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

// A small feedback delay network for the SPACE control: three
// non-commensurate delay lines, each with its own damping (one-pole
// lowpass) inside the feedback loop, panned apart and summed into a
// stereo wet bus. Same idea as serenity.html's three DelayNodes, ported
// to fixed-size ring buffers for the plugin.
struct SpaceTap
{
    std::vector<double> buf;
    int writePos = 0;
    double lpState = 0.0;
    double delaySamples = 0.0;
    double feedback = 0.4;
    double dampCoeff = 0.3;
    double panL = 0.7, panR = 0.7;

    void prepare(double delaySeconds, double sampleRate, double fb, double damp, double pL, double pR)
    {
        buf.assign(static_cast<size_t>(delaySeconds * sampleRate) + 4, 0.0);
        writePos = 0;
        lpState = 0.0;
        delaySamples = delaySeconds * sampleRate;
        feedback = fb;
        dampCoeff = damp;
        panL = pL;
        panR = pR;
    }

    void process(double input, double* outL, double* outR)
    {
        int n = static_cast<int>(buf.size());
        int readPos = writePos - static_cast<int>(delaySamples);
        while (readPos < 0)
            readPos += n;
        double delayed = buf[static_cast<size_t>(readPos % n)];

        lpState += (delayed - lpState) * dampCoeff;
        double fed = input + lpState * feedback;

        buf[static_cast<size_t>(writePos)] = fed;
        writePos = (writePos + 1) % n;

        *outL = lpState * panL;
        *outR = lpState * panR;
    }
};

struct SpaceDelay
{
    std::array<SpaceTap, 3> taps;

    void prepare(double sampleRate)
    {
        taps[0].prepare(0.29, sampleRate, 0.42, 0.35, 0.85, 0.35);
        taps[1].prepare(0.43, sampleRate, 0.38, 0.30, 0.35, 0.85);
        taps[2].prepare(0.61, sampleRate, 0.34, 0.25, 0.6, 0.6);
    }

    void process(double inL, double inR, double* outL, double* outR)
    {
        double mono = 0.5 * (inL + inR);
        double l = 0.0, r = 0.0;
        for (auto& t : taps)
        {
            double tl, tr;
            t.process(mono, &tl, &tr);
            l += tl;
            r += tr;
        }
        *outL = l;
        *outR = r;
    }
};
