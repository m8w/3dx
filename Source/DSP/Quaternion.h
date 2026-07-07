#pragma once
#include <cmath>
#include <array>

// Minimal quaternion algebra used to cross-mix the 4 voices, the direct
// descendant of CAYLEY II's 8-component octonion multiply (Z = Y*X), sized
// down to 4 so each voice is exactly one component. Unlike octonions,
// quaternions are associative, so CASCADE/ASSOC (which exploited octonion
// non-associativity) doesn't carry over -- CONJUGATE (Y*X*Y^-1, the textbook
// quaternion rotation formula) replaces it with something quaternion-native:
// it rotates the 3 "imaginary" voices around the spin axis while the first
// voice (the scalar part) passes through untouched.
struct Quat
{
    double w = 1, x = 0, y = 0, z = 0;

    static Quat fromArray(const double* a) { return { a[0], a[1], a[2], a[3] }; }
    void toArray(double* a) const { a[0] = w; a[1] = x; a[2] = y; a[3] = z; }

    Quat conjugate() const { return { w, -x, -y, -z }; }

    Quat operator*(const Quat& q) const
    {
        return {
            w * q.w - x * q.x - y * q.y - z * q.z,
            w * q.x + x * q.w + y * q.z - z * q.y,
            w * q.y - x * q.z + y * q.w + z * q.x,
            w * q.z + x * q.y - y * q.x + z * q.w
        };
    }

    void normalize()
    {
        double n = std::sqrt(w * w + x * x + y * y + z * z);
        if (n > 1.0e-9) { w /= n; x /= n; y /= n; z /= n; }
        else { w = 1; x = y = z = 0; }
    }
};

enum class AlgMode { Rotate = 0, Cascade, Conjugate };

// X is the raw 4 voice signals, Y/W are unit rotors (spinning oscillator +
// secondary rotor for CASCADE). Result written to Z.
inline void applyAlgebra(AlgMode mode, const double* X, const Quat& Y, const Quat& W, double* Z)
{
    Quat xq = Quat::fromArray(X);
    Quat result;
    if (mode == AlgMode::Rotate)
    {
        result = Y * xq;
    }
    else if (mode == AlgMode::Cascade)
    {
        result = W * (Y * xq);
    }
    else // Conjugate
    {
        Quat yInv = Y.conjugate(); // Y is unit norm, conjugate == inverse
        result = (Y * xq) * yInv;
    }
    result.toArray(Z);
}
