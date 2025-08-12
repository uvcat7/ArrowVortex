#include <Editor/FindOnsets.h>
#include <Editor/Aubio.h>

#include <Core/Utils.h>
#include <Core/Vector.h>
#include <Core/AlignedMemory.h>

#include <System/Thread.h>

#include <math.h>
#include <mutex>

// double to float conversion.
#pragma warning(disable : 4244)

// #define DEBUG_INFO

typedef double real;

namespace Vortex {

extern void rdft(int n, int isgn, smpl_t *a, int *ip, smpl_t *w);

namespace {

// ================================================================================================
// Structures.

/** onsetdetection types */
typedef enum {
    aubio_onset_energy,   /**< energy based */
    aubio_onset_specdiff, /**< spectral diff */
    aubio_onset_hfc,      /**< high frequency content */
    aubio_onset_complex,  /**< complex domain */
    aubio_onset_phase,    /**< phase fast */
    aubio_onset_kl,       /**< Kullback Liebler */
    aubio_onset_mkl,      /**< modified Kullback Liebler */
    aubio_onset_specflux, /**< spectral flux */
    aubio_onset_default = aubio_onset_hfc,
} aubio_specdesc_type;

/** scale object */
struct aubio_scale_t {
    smpl_t ilow;
    smpl_t ihig;
    smpl_t olow;
    smpl_t ohig;
    smpl_t scaler;
    smpl_t irange;
};

/** histogram object */
struct aubio_hist_t {
    fvec_t *hist;
    uint_t nelems;
    fvec_t *cent;
    aubio_scale_t *scaler;
};

/** FFT object */
struct aubio_fft_t {
    uint_t winsize;
    uint_t fft_size;
    smpl_t *in, *out;
    smpl_t *w;
    int *ip;
    fvec_t *compspec;
};

/** Digital filter */
struct aubio_filter_t {
    uint_t order;
    uint_t samplerate;
    lvec_t *a;
    lvec_t *b;
    lvec_t *y;
    lvec_t *x;
};

/** spectral description structure */
struct aubio_specdesc_t {
    aubio_specdesc_type onset_type; /**< onset detection type */
    /** Pointer to aubio_specdesc_<type> function */
    void (*funcpointer)(aubio_specdesc_t *o, cvec_t *fftgrain, fvec_t *onset);
    smpl_t threshold;     /**< minimum norm threshold for phase and specdiff */
    fvec_t *oldmag;       /**< previous norm Vector */
    fvec_t *dev1;         /**< current onset detection measure Vector */
    fvec_t *theta1;       /**< previous phase Vector, one frame behind */
    fvec_t *theta2;       /**< previous phase Vector, two frames behind */
    aubio_hist_t *histog; /**< histogram */
};

/** function pointer to thresholding function */
typedef smpl_t (*aubio_thresholdfn_t)(fvec_t *input);
/** function pointer to peak-picking function */
typedef uint_t (*aubio_pickerfn_t)(fvec_t *input, uint_t pos);

/** peak-picker structure */
struct aubio_peakpicker_t {
    /** thresh: offset threshold [0.033 or 0.01] */
    smpl_t threshold;
    /** win_post: median filter window length (causal part) [8] */
    uint_t win_post;
    /** pre: median filter window (anti-causal part) [post-1] */
    uint_t win_pre;
    /** threshfn: name or handle of fn for computing adaptive threshold [median]
     */
    aubio_thresholdfn_t thresholdfn;
    /** picker: name or handle of fn for picking event times [peakpick] */
    aubio_pickerfn_t pickerfn;
    /** biquad lowpass filter */
    aubio_filter_t *biquad;
    /** original onsets */
    fvec_t *onset_keep;
    /** modified onsets */
    fvec_t *onset_proc;
    /** peak picked window [3] */
    fvec_t *onset_peek;
    /** thresholded function */
    fvec_t *thresholded;
    /** scratch pad for biquad and median */
    fvec_t *scratch;
};

/** phasevocoder object */
struct aubio_pvoc_t {
    uint_t win_s;        /** grain length */
    uint_t hop_s;        /** overlap step */
    aubio_fft_t *fft;    /** fft object */
    fvec_t *data;        /** current input grain, [win_s] frames */
    fvec_t *dataold;     /** memory of past grain, [win_s-hop_s] frames */
    fvec_t *synth;       /** current output grain, [win_s] frames */
    fvec_t *synthold;    /** memory of past grain, [win_s-hop_s] frames */
    fvec_t *w;           /** grain window [win_s] */
    uint_t start;        /** where to start additive synthesis */
    uint_t end;          /** where to end it */
    smpl_t scale;        /** scaling factor for synthesis */
    uint_t end_datasize; /** size of memory to end */
    uint_t hop_datasize; /** size of memory to hop_s */
};

/** onset detection object */
struct aubio_onset_t {
    aubio_pvoc_t *pv;       /**< phase vocoder */
    aubio_specdesc_t *od;   /**< spectral descriptor */
    aubio_peakpicker_t *pp; /**< peak picker */
    cvec_t *fftgrain;       /**< phase vocoder output */
    fvec_t *desc;           /**< spectral description */
    smpl_t silence;         /**< silence threhsold */
    uint_t minioi;          /**< minimum inter onset interval */
    uint_t delay; /**< constant delay, in samples, removed from detected onset
                     times */
    uint_t samplerate;   /**< sampling rate of the input signal */
    uint_t hop_size;     /**< number of samples between two runs */
    uint_t total_frames; /**< total number of frames processed since the
                            beginning */
    uint_t last_onset;   /**< last detected onset location, in frames */
};

// ================================================================================================
// Filter.

static aubio_filter_t *new_aubio_filter(uint_t order) {
    aubio_filter_t *f = AUBIO_NEW(aubio_filter_t);
    f->x = new_lvec(order);
    f->y = new_lvec(order);
    f->a = new_lvec(order);
    f->b = new_lvec(order);
    /* by default, samplerate is not set */
    f->samplerate = 0;
    f->order = order;
    /* set default to identity */
    f->a->data[1] = 1.;
    return f;
}

static void del_aubio_filter(aubio_filter_t *f) {
    del_lvec(f->a);
    del_lvec(f->b);
    del_lvec(f->x);
    del_lvec(f->y);
    AUBIO_FREE(f);
    return;
}

static lvec_t *aubio_filter_get_feedback(aubio_filter_t *f) { return f->a; }

static lvec_t *aubio_filter_get_feedforward(aubio_filter_t *f) { return f->b; }

static uint_t aubio_filter_get_order(aubio_filter_t *f) { return f->order; }

static uint_t aubio_filter_set_biquad(aubio_filter_t *f, lsmp_t b0, lsmp_t b1,
                                      lsmp_t b2, lsmp_t a1, lsmp_t a2) {
    uint_t order = aubio_filter_get_order(f);
    lvec_t *bs = aubio_filter_get_feedforward(f);
    lvec_t *as = aubio_filter_get_feedback(f);

    if (order != 3) {
        AUBIO_ERROR("order of biquad filter must be 3, not %d\n", order);
        return AUBIO_FAIL;
    }
    bs->data[0] = b0;
    bs->data[1] = b1;
    bs->data[2] = b2;
    as->data[0] = 1.;
    as->data[1] = a1;
    as->data[1] = a2;
    return AUBIO_OK;
}

static aubio_filter_t *new_aubio_filter_biquad(lsmp_t b0, lsmp_t b1, lsmp_t b2,
                                               lsmp_t a1, lsmp_t a2) {
    aubio_filter_t *f = new_aubio_filter(3);
    aubio_filter_set_biquad(f, b0, b1, b2, a1, a2);
    return f;
}

static void aubio_filter_do(aubio_filter_t *f, fvec_t *in) {
    uint_t j, l, order = f->order;
    lsmp_t *x = f->x->data;
    lsmp_t *y = f->y->data;
    lsmp_t *a = f->a->data;
    lsmp_t *b = f->b->data;

    for (j = 0; j < in->length; j++) {
        /* new input */
        x[0] = KILL_DENORMAL(in->data[j]);
        y[0] = b[0] * x[0];
        for (l = 1; l < order; l++) {
            y[0] += b[l] * x[l];
            y[0] -= a[l] * y[l];
        }
        /* new output */
        in->data[j] = y[0];
        /* store for next sample */
        for (l = order - 1; l > 0; l--) {
            x[l] = x[l - 1];
            y[l] = y[l - 1];
        }
    }
}

static void aubio_filter_do_reset(aubio_filter_t *f) {
    lvec_zeros(f->x);
    lvec_zeros(f->y);
}

static void aubio_filter_do_filtfilt(aubio_filter_t *f, fvec_t *in,
                                     fvec_t *tmp) {
    uint_t j;
    uint_t length = in->length;
    /* apply filtering */
    aubio_filter_do(f, in);
    aubio_filter_do_reset(f);
    /* mirror */
    for (j = 0; j < length; j++) tmp->data[length - j - 1] = in->data[j];
    /* apply filtering on mirrored */
    aubio_filter_do(f, tmp);
    aubio_filter_do_reset(f);
    /* invert back */
    for (j = 0; j < length; j++) in->data[j] = tmp->data[length - j - 1];
}

// ================================================================================================
// FFT.

static aubio_fft_t *new_aubio_fft(uint_t winsize) {
    aubio_fft_t *s = AUBIO_NEW(aubio_fft_t);
    s->winsize = winsize;
    s->fft_size = winsize / 2 + 1;
    s->compspec = new_fvec(winsize);
    s->in = AUBIO_ARRAY(smpl_t, s->winsize);
    s->out = AUBIO_ARRAY(smpl_t, s->winsize);
    s->ip = AUBIO_ARRAY(int, s->fft_size);
    s->w = AUBIO_ARRAY(smpl_t, s->fft_size);
    s->ip[0] = 0;
    return s;
}

static void del_aubio_fft(aubio_fft_t *s) {
    del_fvec(s->compspec);
    AUBIO_FREE(s->w);
    AUBIO_FREE(s->ip);
    AUBIO_FREE(s->out);
    AUBIO_FREE(s->in);
    AUBIO_FREE(s);
}

static void aubio_fft_do_complex(aubio_fft_t *s, fvec_t *input,
                                 fvec_t *compspec) {
    uint_t i;
    for (i = 0; i < s->winsize; i++) {
        s->in[i] = input->data[i];
    }
    rdft(s->winsize, 1, s->in, s->ip, s->w);
    compspec->data[0] = s->in[0];
    compspec->data[s->winsize / 2] = s->in[1];
    for (i = 1; i < s->fft_size - 1; i++) {
        compspec->data[i] = s->in[2 * i];
        compspec->data[s->winsize - i] = -s->in[2 * i + 1];
    }
}

static void aubio_fft_get_phas(fvec_t *compspec, cvec_t *spectrum) {
    uint_t i;
    if (compspec->data[0] < 0) {
        spectrum->phas[0] = PI;
    } else {
        spectrum->phas[0] = 0.;
    }
    for (i = 1; i < spectrum->length - 1; i++) {
        spectrum->phas[i] =
            ATAN2(compspec->data[compspec->length - i], compspec->data[i]);
    }
    if (compspec->data[compspec->length / 2] < 0) {
        spectrum->phas[spectrum->length - 1] = PI;
    } else {
        spectrum->phas[spectrum->length - 1] = 0.;
    }
}

static void aubio_fft_get_norm(fvec_t *compspec, cvec_t *spectrum) {
    uint_t i = 0;
    spectrum->norm[0] = ABS(compspec->data[0]);
    for (i = 1; i < spectrum->length - 1; i++) {
        spectrum->norm[i] = SQRT(SQR(compspec->data[i]) +
                                 SQR(compspec->data[compspec->length - i]));
    }
    spectrum->norm[spectrum->length - 1] =
        ABS(compspec->data[compspec->length / 2]);
}

static void aubio_fft_do(aubio_fft_t *s, fvec_t *input, cvec_t *spectrum) {
    aubio_fft_do_complex(s, input, s->compspec);
    aubio_fft_get_phas(s->compspec, spectrum);
    aubio_fft_get_norm(s->compspec, spectrum);
}

// ================================================================================================
// Phasevocoder.

static aubio_pvoc_t *new_aubio_pvoc(uint_t win_s, uint_t hop_s) {
    aubio_pvoc_t *pv = AUBIO_NEW(aubio_pvoc_t);

    pv->fft = new_aubio_fft(win_s);

    /* remember old */
    pv->data = new_fvec(win_s);
    pv->synth = new_fvec(win_s);

    /* new input output */
    pv->dataold = new_fvec(win_s - hop_s);
    pv->synthold = new_fvec(win_s - hop_s);
    pv->w = new_aubio_window(aubio_window_type::aubio_win_hanningz, win_s);

    pv->hop_s = hop_s;
    pv->win_s = win_s;

    /* more than 50% overlap, overlap anyway */
    if (win_s < 2 * hop_s) pv->start = 0;
    /* less than 50% overlap, reset latest grain trail */
    else
        pv->start = win_s - hop_s - hop_s;

    if (win_s > hop_s)
        pv->end = win_s - hop_s;
    else
        pv->end = 0;

    pv->end_datasize = pv->end * sizeof(smpl_t);
    pv->hop_datasize = pv->hop_s * sizeof(smpl_t);

    pv->scale = pv->hop_s * 2. / pv->win_s;

    return pv;
}

static void del_aubio_pvoc(aubio_pvoc_t *pv) {
    del_fvec(pv->data);
    del_fvec(pv->synth);
    del_fvec(pv->dataold);
    del_fvec(pv->synthold);
    del_fvec(pv->w);
    del_aubio_fft(pv->fft);
    AUBIO_FREE(pv);
}

static void aubio_pvoc_swapbuffers(aubio_pvoc_t *pv, fvec_t *vnew) {
    /* some convenience pointers */
    smpl_t *data = pv->data->data;
    smpl_t *dataold = pv->dataold->data;
    smpl_t *datanew = vnew->data;
    memcpy(data, dataold, pv->end_datasize);
    data += pv->end;
    memcpy(data, datanew, pv->hop_datasize);
    data -= pv->end;
    data += pv->hop_s;
    memcpy(dataold, data, pv->end_datasize);
}

static void aubio_pvoc_do(aubio_pvoc_t *pv, fvec_t *datanew, cvec_t *fftgrain) {
    /* slide  */
    aubio_pvoc_swapbuffers(pv, datanew);
    /* windowing */

    // Multiply data by weights.
    uint_t length = MIN(pv->data->length, pv->w->length);
    for (uint_t j = 0; j < length; j++) pv->data->data[j] *= pv->w->data[j];

    /* shift */
    fvec_shift(pv->data);
    /* calculate fft */
    aubio_fft_do(pv->fft, pv->data, fftgrain);
}

// ================================================================================================
// Peak picker.

static aubio_peakpicker_t *new_aubio_peakpicker(void) {
    aubio_peakpicker_t *t = AUBIO_NEW(aubio_peakpicker_t);
    t->threshold = 0.1; /* 0.0668; 0.33; 0.082; 0.033; */
    t->win_post = 5;
    t->win_pre = 1;

    t->thresholdfn = (aubio_thresholdfn_t)(fvec_median); /* (fvec_mean); */
    t->pickerfn = (aubio_pickerfn_t)(fvec_peakpick);

    t->scratch = new_fvec(t->win_post + t->win_pre + 1);
    t->onset_keep = new_fvec(t->win_post + t->win_pre + 1);
    t->onset_proc = new_fvec(t->win_post + t->win_pre + 1);
    t->onset_peek = new_fvec(3);
    t->thresholded = new_fvec(1);

    /* cutoff: low-pass filter with cutoff reduced frequency at 0.34
    generated with octave butter function: [b,a] = butter(2, 0.34);
    */
    t->biquad = new_aubio_filter_biquad(0.15998789, 0.31997577, 0.15998789,
                                        -0.59488894, 0.23484048);

    return t;
}

static void del_aubio_peakpicker(aubio_peakpicker_t *p) {
    del_aubio_filter(p->biquad);
    del_fvec(p->onset_keep);
    del_fvec(p->onset_proc);
    del_fvec(p->onset_peek);
    del_fvec(p->thresholded);
    del_fvec(p->scratch);
    AUBIO_FREE(p);
}

static uint_t aubio_peakpicker_set_threshold(aubio_peakpicker_t *p,
                                             smpl_t threshold) {
    p->threshold = threshold;
    return AUBIO_OK;
}

static void aubio_peakpicker_do(aubio_peakpicker_t *p, fvec_t *onset,
                                fvec_t *out) {
    fvec_t *onset_keep = p->onset_keep;
    fvec_t *onset_proc = p->onset_proc;
    fvec_t *onset_peek = p->onset_peek;
    fvec_t *thresholded = p->thresholded;
    fvec_t *scratch = p->scratch;
    smpl_t mean = 0., median = 0.;
    uint_t length = p->win_post + p->win_pre + 1;
    uint_t j = 0;

    /* store onset in onset_keep */
    /* shift all elements but last, then write last */
    for (j = 0; j < length - 1; j++) {
        onset_keep->data[j] = onset_keep->data[j + 1];
        onset_proc->data[j] = onset_keep->data[j];
    }
    onset_keep->data[length - 1] = onset->data[0];
    onset_proc->data[length - 1] = onset->data[0];

    /* filter onset_proc */
    /** \bug filtfilt calculated post+pre times, should be only once !? */
    aubio_filter_do_filtfilt(p->biquad, onset_proc, scratch);

    /* calculate mean and median for onset_proc */
    mean = fvec_mean(onset_proc);
    /* copy to scratch */
    for (j = 0; j < length; j++) scratch->data[j] = onset_proc->data[j];
    median = p->thresholdfn(scratch);

    /* shift peek array */
    for (j = 0; j < 3 - 1; j++) onset_peek->data[j] = onset_peek->data[j + 1];
    /* calculate new tresholded value */
    thresholded->data[0] =
        onset_proc->data[p->win_post] - median - mean * p->threshold;
    onset_peek->data[2] = thresholded->data[0];
    out->data[0] = (p->pickerfn)(onset_peek, 1);
    if (out->data[0]) {
        out->data[0] = fvec_quadratic_peak_pos(onset_peek, 1);
    }
}

// ================================================================================================
// Scale function.

static uint_t aubio_scale_set_limits(aubio_scale_t *s, smpl_t ilow, smpl_t ihig,
                                     smpl_t olow, smpl_t ohig) {
    smpl_t inputrange = ihig - ilow;
    smpl_t outputrange = ohig - olow;
    s->ilow = ilow;
    s->ihig = ihig;
    s->olow = olow;
    s->ohig = ohig;
    if (inputrange == 0) {
        s->scaler = 0.0;
    } else {
        s->scaler = outputrange / inputrange;
        if (inputrange < 0) {
            inputrange = inputrange * -1.0f;
        }
    }
    return AUBIO_OK;
}

static aubio_scale_t *new_aubio_scale(smpl_t ilow, smpl_t ihig, smpl_t olow,
                                      smpl_t ohig) {
    aubio_scale_t *s = AUBIO_NEW(aubio_scale_t);
    aubio_scale_set_limits(s, ilow, ihig, olow, ohig);
    return s;
}

static void del_aubio_scale(aubio_scale_t *s) { AUBIO_FREE(s); }

static void aubio_scale_do(aubio_scale_t *s, fvec_t *input) {
    uint_t j;
    for (j = 0; j < input->length; j++) {
        input->data[j] -= s->ilow;
        input->data[j] *= s->scaler;
        input->data[j] += s->olow;
    }
}

// ================================================================================================
// Histogram function.

static aubio_hist_t *new_aubio_hist(smpl_t flow, smpl_t fhig, uint_t nelems) {
    aubio_hist_t *s = AUBIO_NEW(aubio_hist_t);
    smpl_t step = (fhig - flow) / (smpl_t)(nelems);
    smpl_t accum = step;
    uint_t i;
    s->nelems = nelems;
    s->hist = new_fvec(nelems);
    s->cent = new_fvec(nelems);

    /* use scale to map flow/fhig -> 0/nelems */
    s->scaler = new_aubio_scale(flow, fhig, 0, nelems);
    /* calculate centers now once */
    s->cent->data[0] = flow + 0.5 * step;
    for (i = 1; i < s->nelems; i++, accum += step)
        s->cent->data[i] = s->cent->data[0] + accum;

    return s;
}

static void del_aubio_hist(aubio_hist_t *s) {
    del_fvec(s->hist);
    del_fvec(s->cent);
    del_aubio_scale(s->scaler);
    AUBIO_FREE(s);
}

static void aubio_hist_dyn_notnull(aubio_hist_t *s, fvec_t *input) {
    uint_t i;
    sint_t tmp = 0;
    smpl_t ilow = fvec_min(input);
    smpl_t ihig = fvec_max(input);
    smpl_t step = (ihig - ilow) / (smpl_t)(s->nelems);

    /* readapt */
    aubio_scale_set_limits(s->scaler, ilow, ihig, 0, s->nelems);

    /* recalculate centers */
    s->cent->data[0] = ilow + 0.5f * step;
    for (i = 1; i < s->nelems; i++)
        s->cent->data[i] = s->cent->data[0] + i * step;

    /* scale */
    aubio_scale_do(s->scaler, input);

    /* reset data */
    fvec_zeros(s->hist);
    /* run accum */
    for (i = 0; i < input->length; i++) {
        if (input->data[i] != 0) {
            tmp = (sint_t)FLOOR(input->data[i]);
            if ((tmp >= 0) && (tmp < (sint_t)s->nelems))
                s->hist->data[tmp] += 1;
        }
    }
}

static void aubio_hist_weight(aubio_hist_t *s) {
    uint_t j;
    for (j = 0; j < s->nelems; j++) {
        s->hist->data[j] *= s->cent->data[j];
    }
}

static smpl_t aubio_hist_mean(aubio_hist_t *s) {
    uint_t j;
    smpl_t tmp = 0.0;
    for (j = 0; j < s->nelems; j++) tmp += s->hist->data[j];
    return tmp / (smpl_t)(s->nelems);
}

// ================================================================================================
// Spectral description functions.

/* Energy based onset detection function */
static void aubio_specdesc_energy(aubio_specdesc_t *o UNUSED, cvec_t *fftgrain,
                                  fvec_t *onset) {
    uint_t j;
    onset->data[0] = 0.;
    for (j = 0; j < fftgrain->length; j++) {
        onset->data[0] += SQR(fftgrain->norm[j]);
    }
}

/* High Frequency Content onset detection function */
static void aubio_specdesc_hfc(aubio_specdesc_t *o UNUSED, cvec_t *fftgrain,
                               fvec_t *onset) {
    uint_t j;
    onset->data[0] = 0.;
    for (j = 0; j < fftgrain->length; j++) {
        onset->data[0] += (j + 1) * fftgrain->norm[j];
    }
}

/* Complex Domain Method onset detection function */
static void aubio_specdesc_complex(aubio_specdesc_t *o, cvec_t *fftgrain,
                                   fvec_t *onset) {
    uint_t j;
    uint_t nbins = fftgrain->length;
    onset->data[0] = 0.;
    for (j = 0; j < nbins; j++) {
        // compute the predicted phase
        o->dev1->data[j] = 2. * o->theta1->data[j] - o->theta2->data[j];
        // compute the euclidean distance in the complex domain
        // sqrt ( r_1^2 + r_2^2 - 2 * r_1 * r_2 * \cos ( \phi_1 - \phi_2 ) )
        onset->data[0] +=
            SQRT(ABS(SQR(o->oldmag->data[j]) + SQR(fftgrain->norm[j]) -
                     2. * o->oldmag->data[j] * fftgrain->norm[j] *
                         COS(o->dev1->data[j] - fftgrain->phas[j])));
        /* swap old phase data (need to remember 2 frames behind)*/
        o->theta2->data[j] = o->theta1->data[j];
        o->theta1->data[j] = fftgrain->phas[j];
        /* swap old magnitude data (1 frame is enough) */
        o->oldmag->data[j] = fftgrain->norm[j];
    }
}

/* Phase Based Method onset detection function */
static void aubio_specdesc_phase(aubio_specdesc_t *o, cvec_t *fftgrain,
                                 fvec_t *onset) {
    uint_t j;
    uint_t nbins = fftgrain->length;
    onset->data[0] = 0.0;
    o->dev1->data[0] = 0.;
    for (j = 0; j < nbins; j++) {
        o->dev1->data[j] = aubio_unwrap2pi(
            fftgrain->phas[j] - 2.0 * o->theta1->data[j] + o->theta2->data[j]);
        if (o->threshold < fftgrain->norm[j])
            o->dev1->data[j] = ABS(o->dev1->data[j]);
        else
            o->dev1->data[j] = 0.0;
        /* keep a track of the past frames */
        o->theta2->data[j] = o->theta1->data[j];
        o->theta1->data[j] = fftgrain->phas[j];
    }
    /* apply o->histogram */
    aubio_hist_dyn_notnull(o->histog, o->dev1);
    /* weight it */
    aubio_hist_weight(o->histog);
    /* its mean is the result */
    onset->data[0] = aubio_hist_mean(o->histog);
    // onset->data[0] = fvec_mean(o->dev1);
}

/* Spectral difference method onset detection function */
static void aubio_specdesc_specdiff(aubio_specdesc_t *o, cvec_t *fftgrain,
                                    fvec_t *onset) {
    uint_t j;
    uint_t nbins = fftgrain->length;
    onset->data[0] = 0.0;
    for (j = 0; j < nbins; j++) {
        o->dev1->data[j] =
            SQRT(ABS(SQR(fftgrain->norm[j]) - SQR(o->oldmag->data[j])));
        if (o->threshold < fftgrain->norm[j])
            o->dev1->data[j] = ABS(o->dev1->data[j]);
        else
            o->dev1->data[j] = 0.0;
        o->oldmag->data[j] = fftgrain->norm[j];
    }

    /* apply o->histogram (act somewhat as a low pass on the
     * overall function)*/
    aubio_hist_dyn_notnull(o->histog, o->dev1);
    /* weight it */
    aubio_hist_weight(o->histog);
    /* its mean is the result */
    onset->data[0] = aubio_hist_mean(o->histog);
}

/* Kullback Liebler onset detection function
 * note we use ln(1+Xn/(Xn-1+0.0001)) to avoid
 * negative (1.+) and infinite values (+1.e-10) */
static void aubio_specdesc_kl(aubio_specdesc_t *o, cvec_t *fftgrain,
                              fvec_t *onset) {
    uint_t j;
    onset->data[0] = 0.;
    for (j = 0; j < fftgrain->length; j++) {
        onset->data[0] +=
            fftgrain->norm[j] *
            LOG(1. + fftgrain->norm[j] / (o->oldmag->data[j] + 1.e-1));
        o->oldmag->data[j] = fftgrain->norm[j];
    }
    if (isnan(onset->data[0])) onset->data[0] = 0.;
}

/* Modified Kullback Liebler onset detection function
 * note we use ln(1+Xn/(Xn-1+0.0001)) to avoid
 * negative (1.+) and infinite values (+1.e-10) */
static void aubio_specdesc_mkl(aubio_specdesc_t *o, cvec_t *fftgrain,
                               fvec_t *onset) {
    uint_t j;
    onset->data[0] = 0.;
    for (j = 0; j < fftgrain->length; j++) {
        onset->data[0] +=
            LOG(1. + fftgrain->norm[j] / (o->oldmag->data[j] + 1.e-1));
        o->oldmag->data[j] = fftgrain->norm[j];
    }
    if (isnan(onset->data[0])) onset->data[0] = 0.;
}

/* Spectral flux */
static void aubio_specdesc_specflux(aubio_specdesc_t *o, cvec_t *fftgrain,
                                    fvec_t *onset) {
    uint_t j;
    onset->data[0] = 0.;
    for (j = 0; j < fftgrain->length; j++) {
        if (fftgrain->norm[j] > o->oldmag->data[j])
            onset->data[0] += fftgrain->norm[j] - o->oldmag->data[j];
        o->oldmag->data[j] = fftgrain->norm[j];
    }
}

/* Generic function pointing to the choosen one */
static void aubio_specdesc_do(aubio_specdesc_t *o, cvec_t *fftgrain,
                              fvec_t *onset) {
    o->funcpointer(o, fftgrain, onset);
}

static aubio_specdesc_t *new_aubio_specdesc(const char_t *onset_mode,
                                            uint_t size) {
    aubio_specdesc_t *o = AUBIO_NEW(aubio_specdesc_t);
    uint_t rsize = size / 2 + 1;
    aubio_specdesc_type onset_type;
    if (strcmp(onset_mode, "energy") == 0)
        onset_type = aubio_onset_energy;
    else if (strcmp(onset_mode, "specdiff") == 0)
        onset_type = aubio_onset_specdiff;
    else if (strcmp(onset_mode, "hfc") == 0)
        onset_type = aubio_onset_hfc;
    else if (strcmp(onset_mode, "complexdomain") == 0)
        onset_type = aubio_onset_complex;
    else if (strcmp(onset_mode, "complex") == 0)
        onset_type = aubio_onset_complex;
    else if (strcmp(onset_mode, "phase") == 0)
        onset_type = aubio_onset_phase;
    else if (strcmp(onset_mode, "mkl") == 0)
        onset_type = aubio_onset_mkl;
    else if (strcmp(onset_mode, "kl") == 0)
        onset_type = aubio_onset_kl;
    else if (strcmp(onset_mode, "specflux") == 0)
        onset_type = aubio_onset_specflux;
    else if (strcmp(onset_mode, "default") == 0)
        onset_type = aubio_onset_default;
    else {
        AUBIO_ERR("unknown spectral descriptor type %s, using default.\n",
                  onset_mode);
        onset_type = aubio_onset_default;
    }
    switch (onset_type) {
            /* for both energy and hfc, only fftgrain->norm is required */
        case aubio_onset_energy:
            break;
        case aubio_onset_hfc:
            break;
            /* the other approaches will need some more memory spaces */
        case aubio_onset_complex:
            o->oldmag = new_fvec(rsize);
            o->dev1 = new_fvec(rsize);
            o->theta1 = new_fvec(rsize);
            o->theta2 = new_fvec(rsize);
            break;
        case aubio_onset_phase:
            o->dev1 = new_fvec(rsize);
            o->theta1 = new_fvec(rsize);
            o->theta2 = new_fvec(rsize);
            o->histog = new_aubio_hist(0.0, PI, 10);
            o->threshold = 0.1;
            break;
        case aubio_onset_specdiff:
            o->oldmag = new_fvec(rsize);
            o->dev1 = new_fvec(rsize);
            o->histog = new_aubio_hist(0.0, PI, 10);
            o->threshold = 0.1;
            break;
        case aubio_onset_kl:
        case aubio_onset_mkl:
        case aubio_onset_specflux:
            o->oldmag = new_fvec(rsize);
            break;
        default:
            break;
    }

    switch (onset_type) {
        case aubio_onset_energy:
            o->funcpointer = aubio_specdesc_energy;
            break;
        case aubio_onset_hfc:
            o->funcpointer = aubio_specdesc_hfc;
            break;
        case aubio_onset_complex:
            o->funcpointer = aubio_specdesc_complex;
            break;
        case aubio_onset_phase:
            o->funcpointer = aubio_specdesc_phase;
            break;
        case aubio_onset_specdiff:
            o->funcpointer = aubio_specdesc_specdiff;
            break;
        case aubio_onset_kl:
            o->funcpointer = aubio_specdesc_kl;
            break;
        case aubio_onset_mkl:
            o->funcpointer = aubio_specdesc_mkl;
            break;
        case aubio_onset_specflux:
            o->funcpointer = aubio_specdesc_specflux;
            break;
        default:
            break;
    }
    o->onset_type = onset_type;
    return o;
}

static void del_aubio_specdesc(aubio_specdesc_t *o) {
    switch (o->onset_type) {
        case aubio_onset_energy:
            break;
        case aubio_onset_hfc:
            break;
        case aubio_onset_complex:
            del_fvec(o->oldmag);
            del_fvec(o->dev1);
            del_fvec(o->theta1);
            del_fvec(o->theta2);
            break;
        case aubio_onset_phase:
            del_fvec(o->dev1);
            del_fvec(o->theta1);
            del_fvec(o->theta2);
            del_aubio_hist(o->histog);
            break;
        case aubio_onset_specdiff:
            del_fvec(o->oldmag);
            del_fvec(o->dev1);
            del_aubio_hist(o->histog);
            break;
        case aubio_onset_kl:
        case aubio_onset_mkl:
        case aubio_onset_specflux:
            del_fvec(o->oldmag);
            break;
        default:
            break;
    }
    AUBIO_FREE(o);
}

// ================================================================================================
// Onset detection.

static uint_t aubio_onset_set_delay(aubio_onset_t *o, uint_t delay) {
    o->delay = delay;
    return AUBIO_OK;
}

static uint_t aubio_onset_set_threshold(aubio_onset_t *o, smpl_t threshold) {
    aubio_peakpicker_set_threshold(o->pp, threshold);
    return AUBIO_OK;
}

static uint_t aubio_onset_set_minioi(aubio_onset_t *o, uint_t minioi) {
    o->minioi = minioi;
    return AUBIO_OK;
}

static uint_t aubio_onset_set_minioi_s(aubio_onset_t *o, smpl_t minioi) {
    return aubio_onset_set_minioi(o, (uint_t)(minioi * o->samplerate));
}

static uint_t aubio_onset_set_minioi_ms(aubio_onset_t *o, smpl_t minioi) {
    return aubio_onset_set_minioi_s(o, minioi / 1000.);
}

static uint_t aubio_onset_set_silence(aubio_onset_t *o, smpl_t silence) {
    o->silence = silence;
    return AUBIO_OK;
}

/* Allocate memory for an onset detection */
static aubio_onset_t *new_aubio_onset(const char_t *onset_mode, uint_t buf_size,
                                      uint_t hop_size, uint_t samplerate) {
    aubio_onset_t *o = AUBIO_NEW(aubio_onset_t);
    /* store creation parameters */
    o->samplerate = samplerate;
    o->hop_size = hop_size;

    /* allocate memory */
    o->pv = new_aubio_pvoc(buf_size, o->hop_size);
    o->pp = new_aubio_peakpicker();
    o->od = new_aubio_specdesc(onset_mode, buf_size);
    o->fftgrain = new_cvec(buf_size);
    o->desc = new_fvec(1);

    /* set some default parameter */
    aubio_onset_set_threshold(o, 0.3);
    aubio_onset_set_delay(o, 4.3 * hop_size);
    aubio_onset_set_minioi_ms(o, 20.);
    aubio_onset_set_silence(o, -70.);

    /* initialize internal variables */
    o->last_onset = 0;
    o->total_frames = 0;
    return o;
}

static void del_aubio_onset(aubio_onset_t *o) {
    del_aubio_specdesc(o->od);
    del_aubio_peakpicker(o->pp);
    del_aubio_pvoc(o->pv);
    del_fvec(o->desc);
    del_cvec(o->fftgrain);
    AUBIO_FREE(o);
}

/* execute onset detection function on iput buffer */
static void aubio_onset_do(aubio_onset_t *o, fvec_t *input, fvec_t *onset) {
    smpl_t isonset = 0;
    aubio_pvoc_do(o->pv, input, o->fftgrain);
    aubio_specdesc_do(o->od, o->fftgrain, o->desc);
    aubio_peakpicker_do(o->pp, o->desc, onset);
    isonset = onset->data[0];
    if (isonset > 0.) {
        if (aubio_silence_detection(input, o->silence) == 1) {
            // AUBIO_DBG ("silent onset, not marking as onset\n");
            isonset = 0;
        } else {
            uint_t new_onset =
                o->total_frames + (uint_t)ROUND(isonset * o->hop_size);
            if (o->last_onset + o->minioi < new_onset) {
                // AUBIO_DBG ("accepted detection, marking as onset\n");
                o->last_onset = new_onset;
            } else {
                // AUBIO_DBG ("doubled onset, not marking as onset\n");
                isonset = 0;
            }
        }
    } else {
        // we are at the beginning of the file, and we don't find silence
        if (o->total_frames == 0 &&
            aubio_silence_detection(input, o->silence) == 0) {
            // AUBIO_DBG ("beginning of file is not silent, marking as
            // onset\n");
            isonset = o->delay / o->hop_size;
            o->last_onset = o->delay;
        }
    }
    onset->data[0] = isonset;
    o->total_frames += o->hop_size;
    return;
}

static uint_t aubio_onset_get_last(aubio_onset_t *o) {
    return o->last_onset - o->delay;
}

};  // anonymous namespace

// ================================================================================================
// Main function.

void FindOnsets(const float *samples, int samplerate, int numFrames,
                int numThreads, Vector<Onset> &out) {
    static const int windowlen = 256;
    static const int bufsize = windowlen * 4;
    static const char *method = "complex";

    auto onset = new_aubio_onset(method, bufsize, windowlen, samplerate);
    fvec_t *samplevec = new_fvec(windowlen), *beatvec = new_fvec(2);
    for (int i = 0; i <= numFrames - windowlen; i += windowlen) {
        memcpy(samplevec->data, samples + i, sizeof(float) * windowlen);
        aubio_onset_do(onset, samplevec, beatvec);
        if (beatvec->data[0] > 0) {
            int pos = aubio_onset_get_last(onset);
            if (pos >= 0) out.push_back({pos, 1.0});
        }
    }
    del_fvec(samplevec);
    del_fvec(beatvec);
    del_aubio_onset(onset);
}

};  // namespace Vortex