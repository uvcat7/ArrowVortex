#pragma once

namespace Vortex {

// Writes order + 1 coefficients to b and a.
extern void ButterLowPassCoefs(int order, double frequency, double* outB,
                               double* outA);

// Writes order + 1 coefficients to b and a.
extern void ButterHighPassCoefs(int order, double frequency, double* outB,
                                double* outA);

void FilterOrder3(const double* b, const double* a, const short* in, short* out,
                  int size) {
    short maxAmp = 1;
    double Xi, Yi, z0 = 0.0, z1 = 0.0, z2 = 0.0;
    double a1 = a[1], a2 = a[2], a3 = a[3];
    double b0 = b[0], b1 = b[1], b2 = b[2], b3 = b[3];
    for (short *dst = out, *end = dst + size; dst != end; ++in, ++dst) {
        Xi = (double)*in;
        Yi = b0 * Xi + z0;
        z0 = b1 * Xi + z1 - a1 * Yi;
        z1 = b2 * Xi + z2 - a2 * Yi;
        z2 = b3 * Xi - a3 * Yi;
        *dst = (short)Yi;
        maxAmp = max(*dst, maxAmp);
    }
    int scalar = (SHRT_MAX << 16) / (int)maxAmp;
    for (short *dst = out, *end = dst + size; dst != end; ++dst) {
        *dst = (short)clamp((((int)*dst) * scalar) >> 16, SHRT_MIN, SHRT_MAX);
    }
}

void LowPassFilter(short* out, const short* in, int numFrames, double freq) {
    double coefB[4], coefA[4];
    ButterLowPassCoefs(3, freq, coefB, coefA);
    FilterOrder3(coefB, coefA, in, out, numFrames);
}

void HighPassFilter(short* out, const short* in, int numFrames, double freq) {
    double coefB[4], coefA[4];
    ButterHighPassCoefs(3, freq, coefB, coefA);
    FilterOrder3(coefB, coefA, in, out, numFrames);
}

};  // namespace Vortex