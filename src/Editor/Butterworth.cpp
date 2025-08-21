/*
 *                           COPYRIGHT
 *
 * liir - Recursive digital filter functions
 * Copyright (C) 2007 Exstrom Laboratories LLC
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * A copy of the GNU General Public License is available on the internet at:
 *
 * http://www.gnu.org/copyleft/gpl.html
 *
 * or you can write to:
 *
 * The Free Software Foundation, Inc.
 * 675 Mass Ave
 * Cambridge, MA 02139, USA
 *
 * You can contact Exstrom Laboratories LLC via Email at:
 *
 * stefan(AT)exstrom.com
 *
 * or you can write to:
 *
 * Exstrom Laboratories LLC
 * P.O. Box 7651
 * Longmont, CO 80501, USA
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <immintrin.h>
#include <mmintrin.h>

#include <vector>

#define M_PI 3.14159265358

static double *binomial_mult(int n, double *p);
static double *trinomial_mult(int n, double *b, double *c);

static double *dcof_bwlp(int n, double fcf);
static double *dcof_bwhp(int n, double fcf);
static double *dcof_bwbp(int n, double f1f, double f2f);
static double *dcof_bwbs(int n, double f1f, double f2f);

static int *ccof_bwlp(int n);
static int *ccof_bwhp(int n);
static int *ccof_bwbp(int n);
static double *ccof_bwbs(int n, double f1f, double f2f);

static double sf_bwlp(int n, double fcf);
static double sf_bwhp(int n, double fcf);
static double sf_bwbp(int n, double f1f, double f2f);
static double sf_bwbs(int n, double f1f, double f2f);

/**********************************************************************
binomial_mult - multiplies a series of binomials together and returns
the coefficients of the resulting polynomial.

The multiplication has the following form:

(x+p[0])*(x+p[1])*...*(x+p[n-1])

The p[i] coefficients are assumed to be complex and are passed to the
function as a pointer to an array of doubles of length 2n.

The resulting polynomial has the following form:

x^n + a[0]*x^n-1 + a[1]*x^n-2 + ... +a[n-2]*x + a[n-1]

The a[i] coefficients can in general be complex but should in most
cases turn out to be real. The a[i] coefficients are returned by the
function as a pointer to an array of doubles of length 2n. Storage
for the array is allocated by the function and should be freed by the
calling program when no longer needed.

Function arguments:

n  -  The number of binomials to multiply
p  -  Pointer to an array of doubles where p[2i] (i=0...n-1) is
assumed to be the real part of the coefficient of the ith binomial
and p[2i+1] is assumed to be the imaginary part. The overall size
of the array is then 2n.
*/

double *binomial_mult(int n, double *p) {
    int i, j;
    double *a;

    a = static_cast<double *>(calloc(2 * n, sizeof(double)));
    if (a == nullptr) return (nullptr);

    for (i = 0; i < n; ++i) {
        for (j = i; j > 0; --j) {
            a[2 * j] +=
                p[2 * i] * a[2 * (j - 1)] - p[2 * i + 1] * a[2 * (j - 1) + 1];
            a[2 * j + 1] +=
                p[2 * i] * a[2 * (j - 1) + 1] + p[2 * i + 1] * a[2 * (j - 1)];
        }
        a[0] += p[2 * i];
        a[1] += p[2 * i + 1];
    }
    return (a);
}

/**********************************************************************
trinomial_mult - multiplies a series of trinomials together and returns
the coefficients of the resulting polynomial.

The multiplication has the following form:

(x^2 + b[0]x + c[0])*(x^2 + b[1]x + c[1])*...*(x^2 + b[n-1]x + c[n-1])

The b[i] and c[i] coefficients are assumed to be complex and are passed
to the function as a pointers to arrays of doubles of length 2n. The real
part of the coefficients are stored in the even numbered elements of the
array and the imaginary parts are stored in the odd numbered elements.

The resulting polynomial has the following form:

x^2n + a[0]*x^2n-1 + a[1]*x^2n-2 + ... +a[2n-2]*x + a[2n-1]

The a[i] coefficients can in general be complex but should in most cases
turn out to be real. The a[i] coefficients are returned by the function as
a pointer to an array of doubles of length 4n. The real and imaginary
parts are stored, respectively, in the even and odd elements of the array.
Storage for the array is allocated by the function and should be freed by
the calling program when no longer needed.

Function arguments:

n  -  The number of trinomials to multiply
b  -  Pointer to an array of doubles of length 2n.
c  -  Pointer to an array of doubles of length 2n.
*/

double *trinomial_mult(int n, double *b, double *c) {
    int i, j;
    double *a;

    a = static_cast<double *>(calloc(4 * n, sizeof(double)));
    if (a == nullptr) return (nullptr);

    a[2] = c[0];
    a[3] = c[1];
    a[0] = b[0];
    a[1] = b[1];

    for (i = 1; i < n; ++i) {
        a[2 * (2 * i + 1)] += c[2 * i] * a[2 * (2 * i - 1)] -
                              c[2 * i + 1] * a[2 * (2 * i - 1) + 1];
        a[2 * (2 * i + 1) + 1] += c[2 * i] * a[2 * (2 * i - 1) + 1] +
                                  c[2 * i + 1] * a[2 * (2 * i - 1)];

        for (j = 2 * i; j > 1; --j) {
            a[2 * j] +=
                b[2 * i] * a[2 * (j - 1)] - b[2 * i + 1] * a[2 * (j - 1) + 1] +
                c[2 * i] * a[2 * (j - 2)] - c[2 * i + 1] * a[2 * (j - 2) + 1];
            a[2 * j + 1] +=
                b[2 * i] * a[2 * (j - 1) + 1] + b[2 * i + 1] * a[2 * (j - 1)] +
                c[2 * i] * a[2 * (j - 2) + 1] + c[2 * i + 1] * a[2 * (j - 2)];
        }

        a[2] += b[2 * i] * a[0] - b[2 * i + 1] * a[1] + c[2 * i];
        a[3] += b[2 * i] * a[1] + b[2 * i + 1] * a[0] + c[2 * i + 1];
        a[0] += b[2 * i];
        a[1] += b[2 * i + 1];
    }

    return (a);
}

/**********************************************************************
dcof_bwlp - calculates the d coefficients for a butterworth lowpass
filter. The coefficients are returned as an array of doubles.

*/

double *dcof_bwlp(int n, double fcf) {
    int k;         // loop variables
    double theta;  // M_PI * fcf / 2.0
    double st;     // sine of theta
    double ct;     // cosine of theta
    double parg;   // pole angle
    double sparg;  // sine of the pole angle
    double cparg;  // cosine of the pole angle
    double a;      // workspace variable
    double *rcof;  // binomial coefficients
    double *dcof;  // dk coefficients

    rcof = static_cast<double *>(calloc(2 * n, sizeof(double)));
    if (rcof == nullptr) return (nullptr);

    theta = M_PI * fcf;
    st = sin(theta);
    ct = cos(theta);

    for (k = 0; k < n; ++k) {
        parg =
            M_PI * static_cast<double>(2 * k + 1) / static_cast<double>(2 * n);
        sparg = sin(parg);
        cparg = cos(parg);
        a = 1.0 + st * sparg;
        rcof[2 * k] = -ct / a;
        rcof[2 * k + 1] = -st * cparg / a;
    }

    dcof = binomial_mult(n, rcof);
    free(rcof);

    dcof[1] = dcof[0];
    dcof[0] = 1.0;
    for (k = 3; k <= n; ++k) dcof[k] = dcof[2 * k - 2];
    return (dcof);
}

/**********************************************************************
dcof_bwhp - calculates the d coefficients for a butterworth highpass
filter. The coefficients are returned as an array of doubles.

*/

double *dcof_bwhp(int n, double fcf) { return (dcof_bwlp(n, fcf)); }

/**********************************************************************
dcof_bwbp - calculates the d coefficients for a butterworth bandpass
filter. The coefficients are returned as an array of doubles.

*/

double *dcof_bwbp(int n, double f1f, double f2f) {
    int k;         // loop variables
    double theta;  // M_PI * (f2f - f1f) / 2.0
    double cp;     // cosine of phi
    double st;     // sine of theta
    double ct;     // cosine of theta
    double s2t;    // sine of 2*theta
    double c2t;    // cosine 0f 2*theta
    double *rcof;  // z^-2 coefficients
    double *tcof;  // z^-1 coefficients
    double *dcof;  // dk coefficients
    double parg;   // pole angle
    double sparg;  // sine of pole angle
    double cparg;  // cosine of pole angle
    double a;      // workspace variables

    cp = cos(M_PI * (f2f + f1f) / 2.0);
    theta = M_PI * (f2f - f1f) / 2.0;
    st = sin(theta);
    ct = cos(theta);
    s2t = 2.0 * st * ct;        // sine of 2*theta
    c2t = 2.0 * ct * ct - 1.0;  // cosine of 2*theta

    rcof = static_cast<double *>(calloc(2 * n, sizeof(double)));
    tcof = static_cast<double *>(calloc(2 * n, sizeof(double)));

    for (k = 0; k < n; ++k) {
        parg =
            M_PI * static_cast<double>(2 * k + 1) / static_cast<double>(2 * n);
        sparg = sin(parg);
        cparg = cos(parg);
        a = 1.0 + s2t * sparg;
        rcof[2 * k] = c2t / a;
        rcof[2 * k + 1] = s2t * cparg / a;
        tcof[2 * k] = -2.0 * cp * (ct + st * sparg) / a;
        tcof[2 * k + 1] = -2.0 * cp * st * cparg / a;
    }

    dcof = trinomial_mult(n, tcof, rcof);
    free(tcof);
    free(rcof);

    dcof[1] = dcof[0];
    dcof[0] = 1.0;
    for (k = 3; k <= 2 * n; ++k) dcof[k] = dcof[2 * k - 2];
    return (dcof);
}

/**********************************************************************
dcof_bwbs - calculates the d coefficients for a butterworth bandstop
filter. The coefficients are returned as an array of doubles.

*/

double *dcof_bwbs(int n, double f1f, double f2f) {
    int k;         // loop variables
    double theta;  // M_PI * (f2f - f1f) / 2.0
    double cp;     // cosine of phi
    double st;     // sine of theta
    double ct;     // cosine of theta
    double s2t;    // sine of 2*theta
    double c2t;    // cosine 0f 2*theta
    double *rcof;  // z^-2 coefficients
    double *tcof;  // z^-1 coefficients
    double *dcof;  // dk coefficients
    double parg;   // pole angle
    double sparg;  // sine of pole angle
    double cparg;  // cosine of pole angle
    double a;      // workspace variables

    cp = cos(M_PI * (f2f + f1f) / 2.0);
    theta = M_PI * (f2f - f1f) / 2.0;
    st = sin(theta);
    ct = cos(theta);
    s2t = 2.0 * st * ct;        // sine of 2*theta
    c2t = 2.0 * ct * ct - 1.0;  // cosine 0f 2*theta

    rcof = static_cast<double *>(calloc(2 * n, sizeof(double)));
    tcof = static_cast<double *>(calloc(2 * n, sizeof(double)));

    for (k = 0; k < n; ++k) {
        parg =
            M_PI * static_cast<double>(2 * k + 1) / static_cast<double>(2 * n);
        sparg = sin(parg);
        cparg = cos(parg);
        a = 1.0 + s2t * sparg;
        rcof[2 * k] = c2t / a;
        rcof[2 * k + 1] = -s2t * cparg / a;
        tcof[2 * k] = -2.0 * cp * (ct + st * sparg) / a;
        tcof[2 * k + 1] = 2.0 * cp * st * cparg / a;
    }

    dcof = trinomial_mult(n, tcof, rcof);
    free(tcof);
    free(rcof);

    dcof[1] = dcof[0];
    dcof[0] = 1.0;
    for (k = 3; k <= 2 * n; ++k) dcof[k] = dcof[2 * k - 2];
    return (dcof);
}

/**********************************************************************
ccof_bwlp - calculates the c coefficients for a butterworth lowpass
filter. The coefficients are returned as an array of integers.

*/

int *ccof_bwlp(int n) {
    int *ccof;
    int m;
    int i;

    ccof = static_cast<int *>(calloc(n + 1, sizeof(int)));
    if (ccof == nullptr) return (nullptr);

    ccof[0] = 1;
    ccof[1] = n;
    m = n / 2;
    for (i = 2; i <= m; ++i) {
        ccof[i] = (n - i + 1) * ccof[i - 1] / i;
        ccof[n - i] = ccof[i];
    }
    ccof[n - 1] = n;
    ccof[n] = 1;

    return (ccof);
}

/**********************************************************************
ccof_bwhp - calculates the c coefficients for a butterworth highpass
filter. The coefficients are returned as an array of integers.

*/

int *ccof_bwhp(int n) {
    int *ccof;
    int i;

    ccof = ccof_bwlp(n);
    if (ccof == nullptr) return (nullptr);

    for (i = 0; i <= n; ++i)
        if (i % 2) ccof[i] = -ccof[i];

    return (ccof);
}

/**********************************************************************
ccof_bwbp - calculates the c coefficients for a butterworth bandpass
filter. The coefficients are returned as an array of integers.

*/

int *ccof_bwbp(int n) {
    int *tcof;
    int *ccof;
    int i;

    ccof = static_cast<int *>(calloc(2 * n + 1, sizeof(int)));
    if (ccof == nullptr) return (nullptr);

    tcof = ccof_bwhp(n);
    if (tcof == nullptr) return (nullptr);

    for (i = 0; i < n; ++i) {
        ccof[2 * i] = tcof[i];
        ccof[2 * i + 1] = 0;
    }
    ccof[2 * n] = tcof[n];

    free(tcof);
    return (ccof);
}

/**********************************************************************
ccof_bwbs - calculates the c coefficients for a butterworth bandstop
filter. The coefficients are returned as an array of integers.

*/

double *ccof_bwbs(int n, double f1f, double f2f) {
    double alpha;
    double *ccof;
    int i, j;

    alpha =
        -2.0 * cos(M_PI * (f2f + f1f) / 2.0) / cos(M_PI * (f2f - f1f) / 2.0);

    ccof = static_cast<double *>(calloc(2 * n + 1, sizeof(double)));

    ccof[0] = 1.0;

    ccof[2] = 1.0;
    ccof[1] = alpha;

    for (i = 1; i < n; ++i) {
        ccof[2 * i + 2] += ccof[2 * i];
        for (j = 2 * i; j > 1; --j)
            ccof[j + 1] += alpha * ccof[j] + ccof[j - 1];

        ccof[2] += alpha * ccof[1] + 1.0;
        ccof[1] += alpha;
    }

    return (ccof);
}

/**********************************************************************
sf_bwlp - calculates the scaling factor for a butterworth lowpass filter.
The scaling factor is what the c coefficients must be multiplied by so
that the filter response has a maximum value of 1.

*/

double sf_bwlp(int n, double fcf) {
    int m, k;       // loop variables
    double omega;   // M_PI * fcf
    double fomega;  // function of omega
    double parg0;   // zeroth pole angle
    double sf;      // scaling factor

    omega = M_PI * fcf;
    fomega = sin(omega);
    parg0 = M_PI / static_cast<double>(2 * n);

    m = n / 2;
    sf = 1.0;
    for (k = 0; k < n / 2; ++k)
        sf *= 1.0 + fomega * sin(static_cast<double>(2 * k + 1) * parg0);

    fomega = sin(omega / 2.0);

    if (n % 2) sf *= fomega + cos(omega / 2.0);
    sf = pow(fomega, n) / sf;

    return (sf);
}

/**********************************************************************
sf_bwhp - calculates the scaling factor for a butterworth highpass filter.
The scaling factor is what the c coefficients must be multiplied by so
that the filter response has a maximum value of 1.

*/

double sf_bwhp(int n, double fcf) {
    int m, k;       // loop variables
    double omega;   // M_PI * fcf
    double fomega;  // function of omega
    double parg0;   // zeroth pole angle
    double sf;      // scaling factor

    omega = M_PI * fcf;
    fomega = sin(omega);
    parg0 = M_PI / static_cast<double>(2 * n);

    m = n / 2;
    sf = 1.0;
    for (k = 0; k < n / 2; ++k)
        sf *= 1.0 + fomega * sin(static_cast<double>(2 * k + 1) * parg0);

    fomega = cos(omega / 2.0);

    if (n % 2) sf *= fomega + sin(omega / 2.0);
    sf = pow(fomega, n) / sf;

    return (sf);
}

/**********************************************************************
sf_bwbp - calculates the scaling factor for a butterworth bandpass filter.
The scaling factor is what the c coefficients must be multiplied by so
that the filter response has a maximum value of 1.

*/

double sf_bwbp(int n, double f1f, double f2f) {
    int k;            // loop variables
    double ctt;       // cotangent of theta
    double sfr, sfi;  // real and imaginary parts of the scaling factor
    double parg;      // pole angle
    double sparg;     // sine of pole angle
    double cparg;     // cosine of pole angle
    double a, b, c;   // workspace variables

    ctt = 1.0 / tan(M_PI * (f2f - f1f) / 2.0);
    sfr = 1.0;
    sfi = 0.0;

    for (k = 0; k < n; ++k) {
        parg =
            M_PI * static_cast<double>(2 * k + 1) / static_cast<double>(2 * n);
        sparg = ctt + sin(parg);
        cparg = cos(parg);
        a = (sfr + sfi) * (sparg - cparg);
        b = sfr * sparg;
        c = -sfi * cparg;
        sfr = b - c;
        sfi = a - b - c;
    }

    return (1.0 / sfr);
}

/**********************************************************************
sf_bwbs - calculates the scaling factor for a butterworth bandstop filter.
The scaling factor is what the c coefficients must be multiplied by so
that the filter response has a maximum value of 1.

*/

double sf_bwbs(int n, double f1f, double f2f) {
    int k;            // loop variables
    double tt;        // tangent of theta
    double sfr, sfi;  // real and imaginary parts of the scaling factor
    double parg;      // pole angle
    double sparg;     // sine of pole angle
    double cparg;     // cosine of pole angle
    double a, b, c;   // workspace variables

    tt = tan(M_PI * (f2f - f1f) / 2.0);
    sfr = 1.0;
    sfi = 0.0;

    for (k = 0; k < n; ++k) {
        parg =
            M_PI * static_cast<double>(2 * k + 1) / static_cast<double>(2 * n);
        sparg = tt + sin(parg);
        cparg = cos(parg);
        a = (sfr + sfi) * (sparg - cparg);
        b = sfr * sparg;
        c = -sfi * cparg;
        sfr = b - c;
        sfi = a - b - c;
    }

    return (1.0 / sfr);
}

namespace Vortex {

void ButterLowPassCoefs(int order, double frequency, double *outB,
                        double *outA) {
    double *dcof = dcof_bwlp(order, frequency);  // 2 * order
    int *ccof = ccof_bwlp(order);                // order + 1
    double sf = sf_bwlp(order, frequency);

    for (int i = 0; i <= order; ++i) outA[i] = dcof[i];
    for (int i = 0; i <= order; ++i)
        outB[i] = static_cast<double>(ccof[i]) * sf;

    free(dcof);
    free(ccof);
}

void ButterHighPassCoefs(int order, double frequency, double *outB,
                         double *outA) {
    double *dcof = dcof_bwhp(order, frequency);  // 2 * order
    int *ccof = ccof_bwhp(order);                // order + 1
    double sf = sf_bwhp(order, frequency);

    for (int i = 0; i <= order; ++i) outA[i] = dcof[i];
    for (int i = 0; i <= order; ++i)
        outB[i] = static_cast<double>(ccof[i]) * sf;

    free(dcof);
    free(ccof);
}

};  // namespace Vortex
