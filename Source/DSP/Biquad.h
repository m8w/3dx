#pragma once
#include <cmath>

// Small RBJ biquad, Direct Form I. Used for formant bandpasses, sidechain
// envelope-follower bands, and the feedback loop's HPF/LPF -- the native
// equivalents of the original's stock BiquadFilterNodes.
struct Biquad
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    double x1 = 0, x2 = 0, y1 = 0, y2 = 0;

    inline double process(double x)
    {
        double y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
        x2 = x1; x1 = x;
        y2 = y1; y1 = y;
        return y;
    }

    void reset() { x1 = x2 = y1 = y2 = 0; }

    void setBandpass(double freq, double q, double sampleRate)
    {
        double w0 = 2.0 * pi() * freq / sampleRate;
        double alpha = std::sin(w0) / (2.0 * q);
        double cosw0 = std::cos(w0);
        double a0 = 1 + alpha;
        b0 = alpha / a0;
        b1 = 0;
        b2 = -alpha / a0;
        a1 = (-2 * cosw0) / a0;
        a2 = (1 - alpha) / a0;
    }

    void setHighpass(double freq, double q, double sampleRate)
    {
        double w0 = 2.0 * pi() * freq / sampleRate;
        double alpha = std::sin(w0) / (2.0 * q);
        double cosw0 = std::cos(w0);
        double a0 = 1 + alpha;
        b0 = ((1 + cosw0) / 2) / a0;
        b1 = (-(1 + cosw0)) / a0;
        b2 = ((1 + cosw0) / 2) / a0;
        a1 = (-2 * cosw0) / a0;
        a2 = (1 - alpha) / a0;
    }

    void setLowpass(double freq, double q, double sampleRate)
    {
        double w0 = 2.0 * pi() * freq / sampleRate;
        double alpha = std::sin(w0) / (2.0 * q);
        double cosw0 = std::cos(w0);
        double a0 = 1 + alpha;
        b0 = ((1 - cosw0) / 2) / a0;
        b1 = (1 - cosw0) / a0;
        b2 = ((1 - cosw0) / 2) / a0;
        a1 = (-2 * cosw0) / a0;
        a2 = (1 - alpha) / a0;
    }

    static constexpr double pi() { return 3.14159265358979323846; }
};
