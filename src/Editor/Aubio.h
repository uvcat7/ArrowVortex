#pragma once

#include <Core/Core.h>

namespace Vortex {

/* Config */
#define HAVE_WIN_HACKS 1
#define HAVE_STDLIB_H 1
#define HAVE_STDIO_H 1
#define HAVE_MATH_H 1
#define HAVE_STRING_H 1
#define HAVE_LIMITS_H 1
#define HAVE_C99_VARARGS_MACROS 1
#define HAVE_AUBIO_DOUBLE 0
#define HAVE_WAVREAD 1
#define HAVE_WAVWRITE 1
#define HAVE_MEMCPY_HACKS 1

/* Types */

#if !HAVE_AUBIO_DOUBLE
typedef float smpl_t;
#define AUBIO_SMPL_FMT "%f"
#else
typedef double smpl_t;
#define AUBIO_SMPL_FMT "%lf"
#endif

#if !HAVE_AUBIO_DOUBLE
typedef double lsmp_t;
#define AUBIO_LSMP_FMT "%lf"
#else
typedef long double lsmp_t;
#define AUBIO_LSMP_FMT "%Lf"
#endif

typedef unsigned int uint_t;
typedef int sint_t;
typedef char char_t;

/* fvec */

typedef struct { uint_t length; smpl_t *data; } fvec_t;

fvec_t * new_fvec(uint_t length);
void del_fvec(fvec_t *s);
smpl_t fvec_get_sample(fvec_t *s, uint_t position);
void  fvec_set_sample(fvec_t *s, smpl_t data, uint_t position);
smpl_t * fvec_get_data(fvec_t *s);
void fvec_print(fvec_t *s);
void fvec_set_all(fvec_t *s, smpl_t val);
void fvec_zeros(fvec_t *s);
void fvec_ones(fvec_t *s);
void fvec_rev(fvec_t *s);
void fvec_copy(fvec_t *s, fvec_t *t);

/* cvec */

typedef struct {
	uint_t length;  /**< length of buffer = (requested length)/2 + 1 */
	smpl_t *norm;   /**< norm array of size ::cvec_t.length */
	smpl_t *phas;   /**< phase array of size ::cvec_t.length */
} cvec_t;

cvec_t * new_cvec(uint_t length);
void del_cvec(cvec_t *s);
void cvec_norm_set_sample(cvec_t *s, smpl_t val, uint_t position);
void cvec_phas_set_sample(cvec_t *s, smpl_t val, uint_t position);
smpl_t cvec_norm_get_sample(cvec_t *s, uint_t position);
smpl_t cvec_phas_get_sample(cvec_t *s, uint_t position);
smpl_t * cvec_norm_get_data(cvec_t *s);
smpl_t * cvec_phas_get_data(cvec_t *s);
void cvec_print(cvec_t *s);
void cvec_copy(cvec_t *s, cvec_t *t);
void cvec_norm_set_all(cvec_t *s, smpl_t val);
void cvec_norm_zeros(cvec_t *s);
void cvec_norm_ones(cvec_t *s);
void cvec_phas_set_all(cvec_t *s, smpl_t val);
void cvec_phas_zeros(cvec_t *s);
void cvec_phas_ones(cvec_t *s);
void cvec_zeros(cvec_t *s);

/* lvec */

typedef struct {
	uint_t length; /**< length of buffer */
	lsmp_t *data;  /**< data array of size [length] */
} lvec_t;

lvec_t * new_lvec(uint_t length);
void del_lvec(lvec_t *s);
lsmp_t lvec_get_sample(lvec_t *s, uint_t position);
void  lvec_set_sample(lvec_t *s, lsmp_t data, uint_t position);
lsmp_t * lvec_get_data(lvec_t *s);
void lvec_print(lvec_t *s);
void lvec_set_all(lvec_t *s, smpl_t val);
void lvec_zeros(lvec_t *s);
void lvec_ones(lvec_t *s);

/* Memory management */
#define AUBIO_MALLOC(_n)             malloc(_n)
#define AUBIO_REALLOC(_p,_n)         realloc(_p,_n)
#define AUBIO_NEW(_t)                (_t*)calloc(sizeof(_t), 1)
#define AUBIO_ARRAY(_t,_n)           (_t*)calloc((_n)*sizeof(_t), 1)
#define AUBIO_MEMCPY(_dst,_src,_n)   memcpy(_dst,_src,_n)
#define AUBIO_MEMSET(_dst,_src,_t)   memset(_dst,_src,_t)
#define AUBIO_FREE(_p)               free(_p)

#define AUBIO_ERR(...)
#define AUBIO_ERROR   AUBIO_ERR
#define UNUSED

/* Libc shortcuts */
#define PI         (3.14159265358979323846)
#define TWO_PI     (PI*2.)

/* aliases to math.h functions */
#define EXP        exp
#define COS        cos
#define SIN        sin
#define ABS        fabs
#define POW        pow
#define SQRT       sqrt
#define LOG10      log10
#define LOG        log
#define FLOOR      floor
#define CEIL       ceil
#define ATAN2      atan2
#define ROUND(x)   FLOOR(x+.5)

/* handy shortcuts */
#define DB2LIN(g) (POW(10.0,(g)*0.05f))
#define LIN2DB(v) (20.0*LOG10(v))
#define SQR(_a)   (_a*_a)

#define MAX(a,b)  ( a > b ? a : b)
#define MIN(a,b)  ( a < b ? a : b)

#define ELEM_SWAP(a,b) { smpl_t t=(a);(a)=(b);(b)=t; }

#define VERY_SMALL_NUMBER 2.e-42 //1.e-37

/** if ABS(f) < VERY_SMALL_NUMBER, returns 1, else 0 */
#define IS_DENORMAL(f) ABS(f) < VERY_SMALL_NUMBER

/** if ABS(f) < VERY_SMALL_NUMBER, returns 0., else f */
#define KILL_DENORMAL(f)  IS_DENORMAL(f) ? 0. : f

/** if f > VERY_SMALL_NUMBER, returns f, else returns VERY_SMALL_NUMBER */
#define CEIL_DENORMAL(f)  f < VERY_SMALL_NUMBER ? VERY_SMALL_NUMBER : f

#define SAFE_LOG10(f) LOG10(CEIL_DENORMAL(f))
#define SAFE_LOG(f)   LOG(CEIL_DENORMAL(f))

/* Error reporting */
typedef enum {
	AUBIO_OK = 0,
	AUBIO_FAIL = 1
} aubio_status;

/** System types */
typedef enum
{
	aubio_win_rectangle,
	aubio_win_hamming,
	aubio_win_hanning,
	aubio_win_hanningz,
	aubio_win_blackman,
	aubio_win_blackman_harris,
	aubio_win_gaussian,
	aubio_win_welch,
	aubio_win_parzen,
	aubio_win_default = aubio_win_hanningz,
} aubio_window_type;

/** Utility functions */

fvec_t * new_aubio_window(aubio_window_type window_type, uint_t length);

smpl_t fvec_median(fvec_t * input);
uint_t fvec_peakpick(fvec_t * onset, uint_t pos);

smpl_t aubio_unwrap2pi(smpl_t phase);

smpl_t fvec_max(fvec_t * s);
smpl_t fvec_min(fvec_t * s);
void fvec_shift(fvec_t * s);
smpl_t fvec_mean(fvec_t * s);

smpl_t fvec_quadratic_peak_pos(fvec_t * x, uint_t pos);

uint_t aubio_silence_detection(fvec_t * o, smpl_t threshold);

/** Vector functions */

}; // namespace Vortex