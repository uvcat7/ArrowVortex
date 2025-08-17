#include <Editor/Aubio.h>

#include <string.h>
#include <math.h>
#include <stdlib.h>

// double to float conversion.
#pragma warning(disable : 4244)

namespace Vortex {

/* fvec */

fvec_t *new_fvec(uint_t length) {
    fvec_t *s;
    if (static_cast<sint_t>(length) <= 0) {
        return nullptr;
    }
    s = AUBIO_NEW(fvec_t);
    s->length = length;
    s->data = AUBIO_ARRAY(smpl_t, s->length);
    return s;
}

void del_fvec(fvec_t *s) {
    AUBIO_FREE(s->data);
    AUBIO_FREE(s);
}

void fvec_zeros(fvec_t *s) { memset(s->data, 0, s->length * sizeof(smpl_t)); }

/* cvec */

cvec_t *new_cvec(uint_t length) {
    cvec_t *s;
    if (static_cast<sint_t>(length) <= 0) {
        return nullptr;
    }
    s = AUBIO_NEW(cvec_t);
    s->length = length / 2 + 1;
    s->norm = AUBIO_ARRAY(smpl_t, s->length);
    s->phas = AUBIO_ARRAY(smpl_t, s->length);
    return s;
}

void del_cvec(cvec_t *s) {
    AUBIO_FREE(s->norm);
    AUBIO_FREE(s->phas);
    AUBIO_FREE(s);
}

/* lvec */

lvec_t *new_lvec(uint_t length) {
    lvec_t *s;
    if (static_cast<sint_t>(length) <= 0) {
        return nullptr;
    }
    s = AUBIO_NEW(lvec_t);
    s->length = length;
    s->data = AUBIO_ARRAY(lsmp_t, s->length);
    return s;
}

void del_lvec(lvec_t *s) {
    AUBIO_FREE(s->data);
    AUBIO_FREE(s);
}

void lvec_zeros(lvec_t *s) { memset(s->data, 0, s->length * sizeof(lsmp_t)); }

/* MISC */

uint_t fvec_set_window(fvec_t *win, aubio_window_type window_type) {
    smpl_t *w = win->data;
    uint_t i, size = win->length;

    switch (window_type) {
        case aubio_win_rectangle:
            for (i = 0; i < size; i++) w[i] = 0.5;
            break;
        case aubio_win_hamming:
            for (i = 0; i < size; i++)
                w[i] = 0.54 - 0.46 * COS(TWO_PI * i / (size));
            break;
        case aubio_win_hanning:
            for (i = 0; i < size; i++)
                w[i] = 0.5 - (0.5 * COS(TWO_PI * i / (size)));
            break;
        case aubio_win_hanningz:
            for (i = 0; i < size; i++)
                w[i] = 0.5 * (1.0 - COS(TWO_PI * i / (size)));
            break;
        case aubio_win_blackman:
            for (i = 0; i < size; i++)
                w[i] = 0.42 - 0.50 * COS(TWO_PI * i / (size - 1.0)) +
                       0.08 * COS(2.0 * TWO_PI * i / (size - 1.0));
            break;
        case aubio_win_blackman_harris:
            for (i = 0; i < size; i++)
                w[i] = 0.35875 - 0.48829 * COS(TWO_PI * i / (size - 1.0)) +
                       0.14128 * COS(2.0 * TWO_PI * i / (size - 1.0)) -
                       0.01168 * COS(3.0 * TWO_PI * i / (size - 1.0));
            break;
        case aubio_win_gaussian: {
            lsmp_t a, b, c = 0.5;
            uint_t n;
            for (n = 0; n < size; n++) {
                a = (n - c * (size - 1)) / (SQR(c) * (size - 1));
                b = -c * SQR(a);
                w[n] = EXP(b);
            }
        } break;
        case aubio_win_welch:
            for (i = 0; i < size; i++)
                w[i] = 1.0 - SQR((2. * i - size) / (size + 1.0));
            break;
        case aubio_win_parzen:
            for (i = 0; i < size; i++)
                w[i] = 1.0 - ABS((2. * i - size) / (size + 1.0));
            break;
        default:
            break;
    }
    return 0;
}

fvec_t *new_aubio_window(aubio_window_type window_type, uint_t length) {
    fvec_t *win = new_fvec(length);
    uint_t err;
    if (win == nullptr) {
        return nullptr;
    }
    err = fvec_set_window(win, window_type);
    if (err != 0) {
        del_fvec(win);
        return nullptr;
    }
    return win;
}

smpl_t fvec_median(fvec_t *input) {
    uint_t n = input->length;
    smpl_t *arr = input->data;
    uint_t low, high;
    uint_t median;
    uint_t middle, ll, hh;

    int numOuter = 0, numInner = 0;

    low = 0;
    high = n - 1;
    median = (low + high) / 2;
    for (;;) {
        ++numOuter;

        if (high <= low) /* One element only */
            return arr[median];

        if (high == low + 1) { /* Two elements only */
            if (arr[low] > arr[high]) ELEM_SWAP(arr[low], arr[high]);
            return arr[median];
        }

        /* Find median of low, middle and high items; swap into position low */
        middle = (low + high) / 2;
        if (arr[middle] > arr[high]) ELEM_SWAP(arr[middle], arr[high]);
        if (arr[low] > arr[high]) ELEM_SWAP(arr[low], arr[high]);
        if (arr[middle] > arr[low]) ELEM_SWAP(arr[middle], arr[low]);

        /* Swap low item (now in position middle) into position (low+1) */
        ELEM_SWAP(arr[middle], arr[low + 1]);

        /* Nibble from each end towards middle, swapping items when stuck */
        ll = low + 1;
        hh = high;
        for (;;) {
            do ll++;
            while (arr[low] > arr[ll]);
            do hh--;
            while (arr[hh] > arr[low]);

            if (hh < ll) break;

            ++numInner;

            ELEM_SWAP(arr[ll], arr[hh]);
        }

        /* Swap middle item (in position low) back into correct position */
        ELEM_SWAP(arr[low], arr[hh]);

        /* Re-set active partition */
        if (hh <= median) low = ll;
        if (hh >= median) high = hh - 1;
    }
}

uint_t fvec_peakpick(fvec_t *onset, uint_t pos) {
    uint_t tmp = 0;
    tmp = (onset->data[pos] > onset->data[pos - 1] &&
           onset->data[pos] > onset->data[pos + 1] && onset->data[pos] > 0.);
    return tmp;
}

smpl_t aubio_unwrap2pi(smpl_t phase) {
    /* mod(phase+pi,-2pi)+pi */
    return phase + TWO_PI * (1. + FLOOR(-(phase + PI) / TWO_PI));
}

smpl_t fvec_max(fvec_t *s) {
    uint_t j;
    smpl_t tmp = 0.0;
    for (j = 0; j < s->length; j++) {
        tmp = (tmp > s->data[j]) ? tmp : s->data[j];
    }
    return tmp;
}

smpl_t fvec_min(fvec_t *s) {
    uint_t j;
    smpl_t tmp = s->data[0];
    for (j = 0; j < s->length; j++) {
        tmp = (tmp < s->data[j]) ? tmp : s->data[j];
    }
    return tmp;
}

void fvec_shift(fvec_t *s) {
    uint_t j;
    for (j = 0; j < s->length / 2; j++) {
        ELEM_SWAP(s->data[j], s->data[j + s->length / 2]);
    }
}

smpl_t fvec_mean(fvec_t *s) {
    uint_t j;
    smpl_t tmp = 0.0;
    for (j = 0; j < s->length; j++) {
        tmp += s->data[j];
    }
    return tmp / static_cast<smpl_t>(s->length);
}

smpl_t fvec_quadratic_peak_pos(fvec_t *x, uint_t pos) {
    smpl_t s0, s1, s2;
    uint_t x0, x2;
    if (pos == 0 || pos == x->length - 1) return pos;
    x0 = (pos < 1) ? pos : pos - 1;
    x2 = (pos + 1 < x->length) ? pos + 1 : pos;
    if (x0 == pos) return (x->data[pos] <= x->data[x2]) ? pos : x2;
    if (x2 == pos) return (x->data[pos] <= x->data[x0]) ? pos : x0;
    s0 = x->data[x0];
    s1 = x->data[pos];
    s2 = x->data[x2];
    return pos + 0.5 * (s0 - s2) / (s0 - 2. * s1 + s2);
}

smpl_t aubio_level_lin(fvec_t *f) {
    smpl_t energy = 0.;
    uint_t j;
    for (j = 0; j < f->length; j++) {
        energy += SQR(f->data[j]);
    }
    return energy / f->length;
}

smpl_t aubio_db_spl(fvec_t *o) { return 10. * LOG10(aubio_level_lin(o)); }

uint_t aubio_silence_detection(fvec_t *o, smpl_t threshold) {
    return (aubio_db_spl(o) < threshold);
}

};  // namespace Vortex
