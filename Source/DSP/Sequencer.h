#pragma once
#include <array>
#include <random>
#include <algorithm>
#include <cmath>

// Harmonic-series pattern engine, ported from CAYLEY II's seqStep(), sized
// to 4 voices instead of 8. Same five patterns: DRONE / LADDER / WALK /
// PRIME / FRACTAL, walking integer harmonic numbers assigned to each voice.
struct Sequencer
{
    enum Pattern { Drone = 0, Ladder, Walk, Prime, Fractal };

    long stepIdx = 0;
    std::array<int, 4> walkH { 1, 2, 3, 4 };
    std::array<int, 4> targetH { 1, 2, 3, 4 };

    static const std::array<int, 12>& primes()
    {
        static const std::array<int, 12> p { 1, 2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31 };
        return p;
    }

    static int nPrimesUpTo(int R)
    {
        int n = 0;
        for (int v : primes()) if (v <= R) ++n;
        return std::max(2, n);
    }

    static int ruler(int n) { return n & (-n); } // largest power of 2 dividing n

    void step(Pattern pat, float rangeF, float gate, float envAmt, std::mt19937& rng, bool* triggeredOut)
    {
        long t = stepIdx++;
        int R = std::max(2, static_cast<int>(std::round(rangeF)));
        std::uniform_real_distribution<double> uni(0.0, 1.0);
        std::uniform_int_distribution<int> tri(0, 2);

        for (int k = 0; k < 4; ++k)
        {
            int h;
            if (pat == Ladder)
            {
                h = ((k * 2 + static_cast<int>(t)) % R) + 1;
            }
            else if (pat == Walk)
            {
                walkH[static_cast<size_t>(k)] += tri(rng) - 1;
                walkH[static_cast<size_t>(k)] = std::clamp(walkH[static_cast<size_t>(k)], 1, R);
                h = walkH[static_cast<size_t>(k)];
            }
            else if (pat == Prime)
            {
                int n = nPrimesUpTo(R);
                h = primes()[static_cast<size_t>((k + static_cast<int>(t)) % n)];
            }
            else if (pat == Fractal)
            {
                h = ruler(static_cast<int>(t) + k + 1);
                if (((static_cast<int>(t) + k) >> 2) & 1) h *= 3;
                while (h > R) h >>= 1;
                if (h < 1) h = 1;
            }
            else
            {
                h = std::min(k + 1, R);
            }
            targetH[static_cast<size_t>(k)] = h;

            triggeredOut[k] = (pat != Drone) && (envAmt > 0.01f) && (uni(rng) < gate);
        }
    }
};
